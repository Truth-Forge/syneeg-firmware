# SynEEG Firmware Architecture

## 1. Purpose and authority

This document defines the firmware architecture for one active EEG SynPod Top: one ADS1299, one qualified STM32-family controller, eight EEG channels, a local acquisition clock, and one SynBus/CAN-FD interface.

The product authority is [`SynEEG_SPEC.md`](/Users/jeremyserna/truth_forge/docs/brands/synaptic_silhouette/products/syneeg/SynEEG_SPEC.md). If this document conflicts with that specification, the product specification governs. In particular, this architecture preserves these requirements:

- one ADS1299 and eight EEG channels per active SynPod Top (`INV-001`, `INV-002`);
- 250 samples per second per channel as the mandatory base rate (`POD-ELEC-005`);
- deterministic local acquisition that transport, status, storage, radio, or host behavior cannot block (`POD-ELEC-006`);
- one elected acquisition epoch for all active Tops on a cap (`INV-011`);
- a temporary timing coordinator, with a local sample counter and clock-quality information in every Top (`BUS-004`);
- explicit stop or segmentation on coordinator loss or topology change (`BUS-005`, `DATA-003`);
- complete signed 24-bit samples and ADS1299 status in the native data path (`DATA-002`); and
- SynDock as the off-head power and computer gateway, not the authoritative sampler or permanent timing master (`BND-003`, `DOCK-003`).

The portable architecture does not depend on an exact STM32 suffix or pin map. The current concrete Rev A candidates are selected separately in [`REV_A_REFERENCE_DESIGN.md`](REV_A_REFERENCE_DESIGN.md); board integration must record any evidence-backed change without leaking target-specific details into this portable layer.

## 2. Architectural invariants

1. **Every active SynPod samples locally.** No coordinator, SynDock, host, or other SynPod reads another Pod's ADS1299.
2. **The acquisition path is bounded and non-blocking.** A `DRDY` edge may start a preconfigured SPI DMA transfer; it may not allocate memory, wait for a queue, format USB data, or wait for CAN transmission.
3. **A sample becomes immutable before publication.** After a DMA target is sealed as a `SampleRecord`, no producer or consumer may alter it.
4. **Transport consumes samples; it does not own acquisition timing.** Congestion may produce an explicit gap or fault, but it may not delay the next ADS1299 read.
5. **Sequence and segment identity are monotonic within their scopes.** Reset, reconfiguration, coordinator change, membership change, or timing discontinuity cannot be hidden by continuing an old sequence as if nothing changed.
6. **Solo and multi-Pod acquisition use the same state machine.** A one-Pod cap elects that Pod as coordinator; there is no separate reduced-fidelity acquisition mode.
7. **SynDock translates protocols but does not reinterpret samples.** Native SynBus records remain the lossless source. OpenBCI compatibility is a host-facing projection.
8. **No dynamic allocation occurs after acquisition is armed.** DMA buffers, immutable sample slots, queues, and CAN transmit slots are fixed-size pools established during initialization.

## 3. Layer model

```text
Application policy
  acquisition session, configuration, contact-check state, fault policy
        |
Cap epoch and topology
  discovery, election, membership snapshot, clock quality, segmentation
        |
Native SynBus protocol
  CAN-FD identifiers, sample frames, descriptors, control and fault events
        |
Portable acquisition core
  ADS1299 state, DRDY admission, DMA ownership, immutable records, queues
        |
Platform contract
  monotonic timer, SPI DMA, GPIO actions, CAN-FD, NVM, watchdog, wakeups
        |
Generated board integration (later)
  STM32 startup, clocks, GPIO, DMA, SPI, FDCAN, NVIC, linker and HAL glue
```

Dependencies point downward only. The portable core must not include STM32 HAL headers or expose HAL handles in its public types.

## 4. Deterministic acquisition pipeline

The only normal sample path is:

```text
ADS1299 DRDY falling edge
        -> latch local monotonic capture tick
        -> reserve preallocated DMA slot
        -> assert ADS chip select and start 27-byte SPI DMA read
        -> DMA completion validates transfer and releases chip select
        -> seal immutable SampleRecord
        -> publish slot index to bounded acquisition queue
        -> SynBus worker encodes one native CAN-FD sample frame
        -> asynchronous CAN-FD transmit
        -> release slot to free pool after transport ownership ends
```

### 4.1 Interrupt responsibilities

The `DRDY` interrupt handler does only the work needed to preserve capture time and start the read:

1. latch the free-running local timer;
2. advance the attempted-sample sequence;
3. reserve a free DMA slot with bounded operations;
4. record the current segment, configuration generation, sequence, tick, and accumulated gap count in that writable slot; and
5. invoke `spi_dma_start()` for exactly 27 bytes: three ADS1299 status bytes followed by eight three-byte channel values.

If DMA is already active or no slot is available, the handler increments an acquisition-overrun counter, records the missed sequence, raises a deferred fault event, and returns. It never spins or overwrites a sealed record.

The SPI DMA completion callback:

1. records success, short transfer, timeout, or peripheral failure;
2. releases chip select through the platform adapter;
3. rejects an incomplete transfer rather than publishing partial channel data;
4. seals a successful slot; and
5. publishes only the sealed slot index to the single-producer acquisition queue.

Parsing, sign extension, CAN encoding, logging, LED changes, configuration commands, and host traffic occur outside both interrupt paths.

### 4.2 Fixed ownership states

Each preallocated sample slot has one owner and one state:

```text
FREE -> DMA_WRITING -> SEALED -> QUEUED -> TRANSPORT_OWNED -> FREE
                   \-> REJECTED ---------------------------> FREE
```

- Only DMA may write the 27 raw ADS1299 bytes while the slot is `DMA_WRITING`.
- The acquisition core may fill capture metadata before sealing.
- `SEALED` and later states are immutable.
- State transitions use a short critical section or lock-free primitive supplied by the platform contract.
- A debug build traps an illegal transition. A release build increments a persistent fault counter, stops the affected segment, and enters recovery policy.

### 4.3 Portable sample record

The in-memory record is a semantic type, not a packed wire structure:

```c
typedef struct {
    uint32_t source_node_id;
    uint32_t segment_id;
    uint32_t sample_sequence;
    uint64_t capture_tick;
    uint32_t membership_generation;
    uint16_t configuration_generation;
    uint16_t gaps_before;
    uint8_t  clock_quality;
    uint8_t  ads_status[3];
    uint8_t  channel_be24[8][3];
} SampleRecord;
```

`channel_be24` preserves the ADS1299 byte order and two's-complement value exactly. Conversion to signed 32-bit counts or microvolts is a consumer operation derived from released gain, reference, calibration, and coding metadata. The wire encoder serializes fields explicitly; it must not transmit the compiler's struct layout.

### 4.4 Queue and backpressure policy

The acquisition queue is a bounded single-producer/single-consumer ring of sample-slot indices. Its capacity is selected from measured worst-case CAN service latency and qualified with margin.

Queue full is a data-integrity event, not an invitation to block. The new record is rejected, its sequence remains consumed, the cumulative gap counter advances, and the next published sample or a dedicated `GAP` control frame reports the discontinuity. Repeated overruns beyond a configured threshold stop and segment acquisition rather than allowing an apparently continuous but unreliable stream.

## 5. ADS1299 service

The portable ADS1299 service owns commands, register definitions, startup sequencing, configuration images, readback verification, and conversion-state transitions. Hardware actions are callbacks supplied by the platform adapter.

Required states are:

```text
OFF -> RESETTING -> CONFIGURING -> VERIFIED -> ARMED -> STREAMING
                         |             |          |          |
                         +-----------> FAULT <----+----------+
```

The service supports at least:

- reset and device-ID readback;
- `SDATAC`, register configuration, verified readback, `START`, `RDATAC`, `STOP`, and recovery;
- normal electrode input, internally shorted input, and internal test signal;
- released gain and 250 SPS configuration;
- explicit reference, bias, SRB, and channel-enable configuration;
- lead-off/contact-check configuration whose active state is visible in stream metadata; and
- preservation of all three ADS1299 status bytes in every native sample.

Reconfiguration that changes sampling, gain, channel state, reference, bias, lead-off excitation, or test-signal state closes the current segment before the new configuration becomes active. Register writes are not interleaved with continuous sample reads.

## 6. Cap topology, timing, and coordinator election

### 6.1 Coordinator role

The coordinator is a temporary control-plane role for one SynCap clock domain. It:

- proposes the membership snapshot for the next segment;
- coordinates a future epoch start;
- publishes clock-sync observations and the accepted timing-quality state;
- closes a segment when it can; and
- triggers deterministic re-election when membership changes.

It does **not**:

- read or buffer another Pod's ADS1299 data;
- relay all sample frames;
- assign another Pod's sample sequence;
- define the montage;
- become SynDock's sampler; or
- make local capture dependent on a per-sample coordinator message.

Every Pod retains its own ADS1299, `DRDY`, DMA path, sample sequence, monotonic timer, timing estimator, and sample frames.

### 6.2 Discovery and deterministic election

Each active Pod periodically announces immutable node identity, boot identity, capabilities, current state, and timing quality. A node is eligible to coordinate only after its identity is valid, its local clock is usable, and it is not in a capture or hardware fault state.

For a stable membership snapshot, all nodes select the same eligible winner by deterministic ordering of stable node identity. The election rule must be identical on every Pod and collision detection must reject duplicate identities. Election has a bounded settling interval; samples are not appended to an old segment while membership is unsettled.

With one active Pod, that Pod observes a stable one-member set, elects itself, and starts a one-member segment through the same proposal and start path used by a larger cap. SynDock remains a gateway even in solo operation.

### 6.3 Epoch start

The coordinator proposes:

- the new segment identifier;
- the exact ordered membership snapshot;
- the membership and configuration generations;
- the intended future coordinator-domain start tick; and
- the minimum timing-quality acceptance state.

Each member maps the proposed start into its local monotonic timer, acknowledges only if the uncertainty is acceptable, arms locally, and starts its own ADS1299 at the mapped boundary. The exact clock-estimation algorithm, synchronization signaling, acceptance limits, and measured skew/drift budget remain qualification work under `BUS-006`; this architecture does not claim a timing accuracy before that evidence exists.

No node enters `STREAMING` until the segment descriptor and its local start decision are durable enough to make subsequent samples unambiguous.

### 6.4 Topology-change segmentation

The following events require a segment boundary:

- node join, departure, replacement, reboot, or identity collision;
- coordinator timeout, resignation, or fault;
- duplicate or incompatible segment proposal;
- timing uncertainty outside the released limit;
- ADS1299 reset or acquisition-path recovery;
- sampling, gain, reference, bias, channel, lead-off, or test-signal configuration change; or
- a sustained acquisition or transport overrun that violates the released continuity limit.

Normal transition:

```text
STREAMING
  -> stop admitting old-segment DRDY records
  -> drain or explicitly discard old-segment queued records
  -> publish SEGMENT_END(reason, final_sequence, counters)
  -> DISCOVERING / ELECTING
  -> publish new membership and configuration descriptors
  -> coordinated future start
  -> STREAMING in a new segment
```

If the coordinator disappears, each surviving Pod locally closes the old segment as `ABORTED_COORDINATOR_LOSS`; it does not wait indefinitely for an end frame that cannot arrive. SynDock records the independently observed aborts, prevents cross-segment concatenation, and waits for the newly elected coordinator's segment descriptor.

Physical position and electronic node identity remain separate descriptor fields. A replacement or moved Top produces a new mapping and segment; channel numbers are never silently reassigned within a live segment.

## 7. Native SynBus CAN-FD protocol

### 7.1 General rules

- Native SynBus uses CAN FD with extended identifiers.
- CAN arbitration and data-phase rates are board/topology qualification parameters, not portable-core constants.
- Multi-byte integers use network byte order.
- CAN's link CRC is retained; native records additionally carry an end-to-end CRC-32C so corruption can be detected after gateway, buffering, or file transport.
- A short bus address is assigned only for arbitration. The payload and segment descriptors preserve stable electronic identity.
- Unknown protocol versions or message types are rejected or forwarded as opaque diagnostics; they are not guessed.

The 29-bit extended identifier is partitioned as follows:

```text
bits 28..26  priority class       0 is highest
bits 25..21  message class
bits 20..13  source short address
bits 12..5   destination address  0xFF means broadcast
bits 4..0    subtype
```

Control, election, segment, fault, descriptor, and sample traffic have separately assigned message classes. The registry of numeric class and subtype values is versioned in the protocol implementation; application code uses names rather than literals.

### 7.2 Native sample frame

One CAN-FD frame carries one complete eight-channel ADS1299 conversion. The payload is fixed at 64 bytes (CAN-FD DLC 15):

| Offset | Size | Field | Definition |
|---:|---:|---|---|
| 0 | 1 | `protocol_version` | Native SynBus protocol version |
| 1 | 1 | `message_type` | `SAMPLE` |
| 2 | 1 | `flags` | Segment-start proximity, contact-check/test state, recovered-gap indication |
| 3 | 1 | `header_bytes` | `24` for this version |
| 4 | 4 | `source_node_id` | Stable electronic node identity |
| 8 | 4 | `segment_id` | Segment identity scoped by the session/clock-domain descriptor |
| 12 | 4 | `sample_sequence` | Attempted conversion sequence within the node segment |
| 16 | 8 | `capture_tick` | Local monotonic timer latched at `DRDY` admission |
| 24 | 3 | `ads_status` | ADS1299 status bytes exactly as received |
| 27 | 24 | `channels` | CH1..CH8, each signed two's-complement 24-bit, MSB first |
| 51 | 1 | `clock_quality` | Versioned quality/lock state; numeric uncertainty lives in descriptors/status |
| 52 | 2 | `configuration_generation` | Configuration image used for this conversion |
| 54 | 4 | `membership_generation` | Cap membership snapshot used for this segment |
| 58 | 2 | `gaps_before` | Saturating count of locally known missing attempts since the prior published sample |
| 60 | 4 | `crc32c` | CRC-32C over bytes 0..59 |

`segment_id`, `source_node_id`, and the associated segment descriptor form the lookup key for full `DATA-001` identity: physical SynPod position, montage position, SynCap, wearer, participant, session, clock domain, gain, reference, calibration, and channel mapping. Those descriptors are repeated until acknowledged and persisted by SynDock; they are not redundantly forced into every 64-byte sample frame.

### 7.3 Required non-sample messages

The native protocol includes versioned schemas for:

- `NODE_ANNOUNCE` and capability/boot identity;
- address claim and duplicate-identity rejection;
- election proposal, vote/acknowledgement, coordinator heartbeat, and resignation;
- clock-sync observation and timing-quality status;
- segment proposal, member acknowledgement, start, end, and abort;
- node, channel, montage, calibration, configuration, session, and clock-domain descriptors;
- configuration request, accepted/rejected response, and applied generation;
- `GAP`, acquisition overrun, transport overrun, reset, brownout, watchdog, ADS fault, and bus fault; and
- contact-check/test-signal state and results.

Control messages are idempotent. Retries use stable transaction identifiers. A stale generation, stale boot identity, or old segment cannot change current acquisition state.

## 8. SynDock and OpenBCI compatibility

SynDock terminates CAN-FD and exposes the released isolated computer-facing USB data path. Its native mode forwards descriptors, samples, gaps, segment transitions, timing quality, and faults without changing their meaning.

OpenBCI compatibility exists only at SynDock or in an equivalent host bridge. SynPods never emit OpenBCI serial packets on SynBus.

For the first eight-channel compatibility profile, SynDock maps one active Pod to the conventional 33-byte OpenBCI Cyton packet:

```text
0xA0
sample_number = low 8 bits of native sample_sequence
8 x signed 24-bit channel values copied without rescaling
6 auxiliary bytes set by the declared compatibility profile
0xC0 packet footer
```

Rules for this projection:

- the native stream remains authoritative because the Cyton packet cannot carry full node, segment, timing, gap, configuration, or ADS status metadata;
- a topology or segment change terminates/restarts the compatibility stream explicitly rather than renumbering live channels;
- unsupported OpenBCI commands return an explicit unsupported result and do not mutate hidden state;
- supported gain, channel, start/stop, test, and contact-check commands are validated and translated into native generation-changing commands;
- standard channel bytes remain raw signed ADS1299 counts; microvolt conversion stays in the host descriptor/calibration layer; and
- multi-Pod acquisition uses the native SynEEG/BrainFlow path unless a separately specified compatibility profile preserves segment and source boundaries without pretending that multiple Pods are one Cyton.

SynDock may expose native and compatibility interfaces concurrently only when buffering and USB scheduling have been proven not to cause native loss. Compatibility traffic is always discardable before native acquisition data.

## 9. Portable platform contract

The portable core depends on a narrow C ABI implemented once per board port. Representative operations are:

```c
typedef struct {
    uint64_t (*monotonic_ticks)(void);
    uint32_t (*ticks_per_second)(void);

    bool (*ads_spi_dma_start)(uint8_t *dst, size_t len);
    void (*ads_chip_select)(bool asserted);
    void (*ads_reset)(bool asserted);
    void (*ads_start)(bool asserted);
    void (*ads_power_down)(bool asserted);

    bool (*canfd_tx_async)(uint32_t extended_id,
                           const uint8_t *payload,
                           size_t len,
                           uint32_t token);

    bool (*nvm_read)(uint32_t key, void *dst, size_t len);
    bool (*nvm_write)(uint32_t key, const void *src, size_t len);
    void (*watchdog_kick)(void);
    void (*request_worker_wakeup)(uint32_t events);
    void (*enter_critical)(void);
    void (*exit_critical)(void);
} SynPlatform;
```

The concrete API may split these interfaces by concern, but the ownership boundary remains. Platform code calls portable entry points for `DRDY`, SPI DMA completion/error, CAN receive, CAN transmit completion, timer events, and reset reason. Portable code does not implement interrupt vectors.

The platform adapter guarantees:

- timer reads are monotonic across a segment and document wrap behavior;
- SPI DMA reads exactly the requested bytes in ADS1299-compatible mode;
- chip-select timing and DMA completion ordering meet the ADS1299 requirements;
- callbacks cannot refer to a buffer after ownership is returned;
- CAN transmit completion identifies the original transmit token;
- reset reason and persistent fault counters survive the documented reset classes; and
- critical sections are bounded and safe from both worker and interrupt context.

## 10. Repository implementation boundary

### 10.1 Portable baseline implemented now

The current repository implements and host-tests:

- ADS1299 reset, device-family/channel identification, commands, register access with command-decode timing, one verified 250-SPS register profile, start/stop/recovery, and raw-frame parsing;
- signed 24-bit range preservation and conversion-frame validation;
- one fixed 64-byte native sample-payload codec with CRC-32C and generation/gap fields;
- one 33-byte OpenBCI Cyton-compatible eight-channel projection;
- a minimum-stable-node election primitive with observed-membership revision and required-segment state; and
- deterministic ADS bus mocks plus sample, CRC, range, election, membership-change, and encode/decode tests.

The current coordinator is a primitive, not the complete distributed epoch protocol described in §6. It does not yet implement boot identity, duplicate-identity detection, stable membership proposals, acknowledgements, future start ticks, M-LVDS clock-presence qualification, or multi-node commit messages. Likewise, the extended CAN identifier registry, descriptors, control/fault frames, immutable DMA slot pool, bounded acquisition queue, and timer/DMA/CAN fault-injection harness remain planned portable-core work.

Host tests run on a workstation with no STM32 SDK. They prove only the implemented state and protocol behavior; they do not prove electrical timing, interrupt latency, DMA coherency, CAN signal integrity, synchronization, or ADS1299 noise performance.

### 10.2 Generated STM32 HAL integration added later

The board-specific integration begins only after an exact MCU, package, clock source, pin map, CAN transceiver, PCB, and recovery/debug strategy are selected. Its generated project owns:

- startup, linker script, clock tree, cache and memory-region policy;
- GPIO assignments and electrical modes;
- SPI instance, mode, prescaler, DMA request, DMA buffers, and cache coherency;
- `DRDY` EXTI routing and measured interrupt priority;
- FDCAN instance, filters, message RAM, bit timing, FD/BRS policy, and transceiver controls;
- monotonic timer and capture implementation;
- watchdog, reset reason, bootloader/recovery, NVM, SWD and production-test hooks; and
- the adapter that translates HAL callbacks into the portable entry points.

Generated files are not copied into the portable core or edited to contain product logic. User-owned adapter files wrap generated initialization. Regeneration must leave the portable core untouched.

The STM32 port is complete only after target measurements demonstrate:

- worst-case `DRDY`-to-DMA-start and DMA-completion latency with margin at every released rate;
- no buffer or cache corruption under sustained acquisition;
- acquisition remains loss-free during maximum qualified CAN and USB load;
- reset, brownout, bus-off, unplug, coordinator loss, and topology change produce the specified segment/fault records; and
- the known-signal, contact-check, noise, timing, and long-duration tests required by P3 and the SynEEG verification matrix pass.

## 11. Fault and recovery policy

Faults are classified by whether acquisition meaning remains valid:

| Class | Examples | Required behavior |
|---|---|---|
| Record-local | CAN retry, host backpressure | Preserve native queue if bounded; report counters |
| Explicit gap | no DMA slot, queue full, isolated bad transfer | Consume sequence, report gap, continue only within released threshold |
| Segment-breaking | ADS reset, timing unlock, topology change, coordinator loss, repeated overrun | Stop old segment, report/abort, re-elect or recover, start new segment |
| Node-fatal | identity invalid, persistent ADS failure, unsafe power/reset state | Stop conversion and bus participation except bounded fault/status reporting |

Watchdog recovery never resumes an old segment. Boot identity changes, persistent reset reason is announced, and acquisition returns through discovery, election, descriptor publication, and a new coordinated start.

## 12. Verification seams

The architecture is designed to make the following evidence possible:

- byte-exact ADS1299 command and register tests;
- golden vectors for signed 24-bit preservation and native 64-byte sample frames;
- ownership-model tests proving sealed samples cannot be overwritten;
- deterministic simulated `DRDY`, DMA, queue, and CAN schedules;
- solo election and one-member segment tests;
- three-or-more-node election agreement under reordered/lost control frames;
- coordinator-loss and live join/leave tests proving no sample crosses an unmarked segment boundary;
- native-to-OpenBCI projection vectors at SynDock;
- counter and fault-injection tests for gaps, duplicates, stale generations, resets, and corrupt CRCs; and
- target timing traces correlating `DRDY`, chip select, SPI clock, DMA completion, queue publication, and CAN transmission.

Passing portable tests is necessary but not sufficient for a released SynPod. The authoritative specification requires integration evidence for acquisition accuracy, noise, clock alignment, skew, drift, topology, transport, safety, and long-duration behavior.

## 13. Reference implementations and evidence

These sources inform implementation without overriding the SynEEG specification:

- [TI ADS1299 datasheet](https://www.ti.com/lit/ds/symlink/ads1299.pdf): commands, register behavior, timing, raw data format, startup, reference, bias, and lead-off requirements.
- [TI ADS1299EEG-FE user guide](https://www.ti.com/lit/ug/slau443b/slau443b.pdf): evaluation and known-signal precedent.
- [OpenBCI Cyton firmware library](https://github.com/OpenBCI/OpenBCI_Cyton_Library): established ADS1299 configuration behavior and host-command precedent.
- [OpenBCI Cyton data format](https://docs.openbci.com/Cyton/CytonDataFormat/): compatibility packet definition used only by the SynDock projection.
- [Synaps STM32/OpenBCI port](https://github.com/smargus/Synaps): Apache-2.0 STM32 firmware precedent for ADS1299/OpenBCI behavior.
- [GIBIC ADS1299 library](https://github.com/gibic-leici/ADS1299): MIT-licensed portable driver and STM32 interrupt/DMA examples.
- [Cyton-Mod](https://github.com/flosimn/Cyton-Mod): public STM32-family + ADS1299 + USB hardware/firmware design evidence; no project-level license was identified, so it is precedent rather than a code source without permission.
- [64-channel STM32/ADS1299 firmware](https://github.com/c-q-b/gen4-64ch-eeg-acquisition-stm32): public evidence for multiple locally selected ADS1299 devices and queued USB transport; no project-level license was identified.
- [BrainFlow](https://github.com/brainflow-dev/brainflow): required wired host interoperability target.

Reference code is imported only when its license and provenance are recorded. Behavioral precedent never substitutes for SynEEG target testing.
