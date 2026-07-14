# SynEEG Firmware Architecture

## 1. Authority and scope

This repository implements portable behavior beneath the current [SynEEG product specification](/Users/jeremyserna/truth_forge/docs/brands/synaptic_silhouette/products/syneeg/SynEEG_SPEC.md) and [board design specification](/Users/jeremyserna/truth_forge/docs/brands/synaptic_silhouette/products/syneeg/SynEEG_BOARD_DESIGN_SPEC.md). The released hardware contract is `/Users/jeremyserna/apps_dev/syneeg-hardware/requirements/design_contract.json`.

The architecture has one permanent authority: the STM32G474 in SynDock. SynPods never elect a master and their physical order never determines identity or timing.

## 2. Processor ownership

| Processor | Owns | Does not own |
|---|---|---|
| SynPod RP2040/reference node | ADS1299 SPI/DMA, local sequence/gap/fault evidence, optical-result intake, addressed CAN transport, identity and recovery | Global epochs, shared clock, host or wireless UI |
| Optical RP2040 | OpenfNIRS-derived dual-wavelength source control, AD7380 PIO/DMA acquisition, local accumulation, reduced optical results | SynBus timing, USB, radio, global topology |
| SynDock STM32G474 | Pod discovery/addressing, topology generations, clock and START/RESET control, unified EEG/optical epochs, collection, timestamps, fault segmentation and USB acquisition | Display rendering or wireless application work |
| SynDock ESP32-C6 | Wireless/application transport, updates, and display-state hosting | Acquisition clock, timing authority, ADS1299 or fNIRS acquisition |

The portable C11 layer has no RP2040 SDK, STM32 HAL, ESP-IDF, or packed-struct dependency.

## 3. Deterministic local acquisition

Each Pod reads its own ADS1299. A normal local path is:

```text
ADS1299 DRDY
  -> latch local sequence and shared-carrier time relationship
  -> reserve a fixed DMA slot
  -> read 27 bytes by SPI DMA
  -> seal ADS status + eight 24-bit channels + gap/fault evidence
  -> queue a fixed native frame for the external CAN-FD controller
```

Transport never delays the next conversion. Queue exhaustion or DMA failure consumes the attempted sequence and becomes explicit gap/fault evidence. No dynamic allocation is required after acquisition is armed.

The ADS1299 profile selector preserves the common channel/reference/bias configuration and changes only CONFIG1 `DR[2:0]`:

| Rate | DR bits | CONFIG1 | Product meaning |
|---:|:---:|:---:|---|
| 250 SPS | `110` | `0x96` | selectable product profile |
| 500 SPS | `101` | `0x95` | selectable product profile |
| 1,000 SPS | `100` | `0x94` | primary full-performance profile |
| 2,000 SPS | `011` | `0x93` | supported driver profile; full-system qualification target only |

The register mapping follows the TI ADS1299 CONFIG1 definition. Software support does not claim board or full-topology qualification.

## 4. SynDock coordinator and Pod clients

SynDock maintains stable electronic node IDs, assigned bus addresses, liveness, one pending topology generation, the topology generation of any active segment, a serially increasing segment ID, and a serially increasing control-command sequence. The table holds at most eight active Pods; expired entries are reusable by replacement identities rather than consuming lifetime capacity.

```text
Pod join/leave/address change or timing/acquisition fault
  -> SynDock marks STOP_REQUIRED while preserving the active topology generation
  -> SynDock emits broadcast STOP for the active segment/topology
  -> running Pods accept STOP and become idle
  -> SynDock marks SEGMENT_REQUIRED
  -> SynDock publishes the current topology generation
  -> addressed Pods accept it only from their configured SynDock authority
  -> SynDock begins a nonzero segment whose ID is strictly greater than every prior ID
  -> Pods accept START only for that topology generation
```

SynDock cannot publish a replacement topology or begin another segment while `STOP_REQUIRED`. Pods accept `TOPOLOGY` only while unbound or idle, `START` only while idle for a nonzero current topology and newer nonzero segment, and `STOP` only while running for the exact active topology and segment. A topology update can never substitute for STOP.

Command sequence, topology generation, and segment ID are 32-bit serial numbers using RFC1982-style half-range ordering. Zero is reserved and increment skips from `UINT32_MAX` to `1`. A candidate is newer only when its unsigned forward distance is from 1 through `0x7fffffff`; equality, zero, reverse values, and the ambiguous half-range distance are stale. Exact command duplicates are therefore rejected rather than replayed idempotently. A control retry uses a fresh SynDock command sequence while preserving the intended type, topology, and segment.

Pods also reject commands with the wrong authority ID or destination. A one-Pod prefrontal carrier and a multi-Pod SynCap use the same coordinator/client path. Neither configuration elects a Pod.

The portable coordinator establishes deterministic policy. Hardware adapters still must implement discovery retries, authenticated/recoverable identity provisioning, bounded control acknowledgement, CAN filtering, clock-presence qualification, and physical fault handling.

## 5. Native carriers

All multibyte values use network byte order. CAN-FD link integrity is supplemented by end-to-end CRC-32C so records remain verifiable after gateway buffering or file transport.

### 5.1 EEG sample, version 1, 64 bytes

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | protocol version |
| 1 | 1 | EEG sample type |
| 2 | 1 | flags |
| 3 | 1 | header size (`24`) |
| 4 | 4 | source node ID |
| 8 | 4 | segment ID |
| 12 | 4 | sample sequence |
| 16 | 8 | shared carrier timestamp relationship |
| 24 | 3 | ADS1299 status bytes |
| 27 | 24 | eight signed 24-bit channel counts |
| 51 | 1 | clock quality |
| 52 | 2 | configuration generation |
| 54 | 4 | topology generation |
| 58 | 2 | gaps before |
| 60 | 4 | CRC-32C over bytes 0–59 |

This preserves the verified 64-byte EEG format. The field formerly described as membership generation is now named topology generation without changing its wire location or meaning.

### 5.2 Optical result, version 1, 64 bytes

| Offset | Size | Field |
|---:|---:|---|
| 0 | 1 | optical protocol version |
| 1 | 1 | optical result type |
| 2 | 1 | reserved flags |
| 3 | 1 | detector count (`4`) |
| 4 | 4 | source Pod node ID |
| 8 | 4 | segment ID |
| 12 | 4 | optical sequence |
| 16 | 8 | shared carrier timestamp relationship |
| 24 | 1 | optical source ID |
| 25 | 1 | source disabled/active/fault state |
| 26 | 2 | wavelength in nanometers (`735` or `850`) |
| 28 | 4 | local accumulation count |
| 32 | 4 | four detector IDs |
| 36 | 16 | four signed accumulated raw results |
| 52 | 1 | detector saturation mask |
| 53 | 1 | detector coupling mask |
| 54 | 1 | fault evidence |
| 55 | 1 | clock quality |
| 56 | 2 | optical configuration generation |
| 58 | 2 | gaps before |
| 60 | 4 | CRC-32C over bytes 0–59 |

This is a reduced local result, not a transport for raw optical ADC samples. Status and descriptor frames carry slower-changing physical position, montage, calibration, capabilities, ambient/coupling detail, and fault context without inflating every sample.

## 6. Interoperability boundary

The native stream is lossless authority. SynDock or a host bridge may project one Pod's eight EEG channels into a 33-byte OpenBCI Cyton packet. That projection truncates sequence identity and omits segment, topology, timing-quality, gap, calibration, and optical information; it can never replace or round-trip the native record.

BrainFlow is the immediate EEG compatibility path. Optical results remain available through the native protocol and a future SNIRF-compatible exporter; the firmware does not extend OpenBCI or BrainFlow packets with private optical fields.

## 7. Platform contracts and pending evidence

The SynPod adapter must provide deterministic ADS1299 SPI/DMA, `DRDY`, timers, RP2040-to-MCP2518FD transport, fixed buffers, identity storage, watchdog and recovery. The optical adapter must preserve the OpenfNIRS PIO/DMA and accumulation boundary. The SynDock adapter must provide STM32G474 CAN-FD, common clock/control, hardware timestamps, USB, topology/fault state, and the isolated ESP32-C6 handoff.

Host tests verify register images, state transitions, addressing, stale/authority rejection, signed decoding, field round trips, gaps, and CRC. They do not prove interrupt latency, CAN loading, clock skew/drift, analog noise, optical performance, power, thermal behavior, isolation, wearer safety, or long-duration reliability. Those remain physical release gates.
