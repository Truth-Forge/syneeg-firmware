# SynEEG Firmware

Portable firmware behavior for the distributed SynEEG EEG/fNIRS system.

Each active SynPod captures one ADS1299 locally through a non-authoritative RP2040-class node and transports identified records through an external MCP2518FD-class CAN-FD controller and MCP2562FD transceiver. A Hybrid SynPod also accepts reduced results from its separate OpenfNIRS-derived optical RP2040 board. Raw optical ADC samples remain local.

SynDock is the permanent system authority. Its STM32G474 discovers and addresses Pods, owns topology generations and segment transitions, distributes the 2.048 MHz clock plus synchronized control, collects EEG and optical records, and exposes the native wired stream. The ESP32-C6 and display are operational and connectivity surfaces; neither can become acquisition authority.

## Implemented portable behavior

- ADS1299 command/register transport, verified configuration, and signed 24-bit conversion decoding;
- selectable 250, 500, and 1,000 SPS product profiles;
- a 2,000 SPS driver profile marked explicitly as a full-system qualification target, not a qualified product profile;
- a permanent SynDock coordinator and addressed non-authoritative Pod node state machine;
- topology-generation changes that enter an explicit stop-required state, emit a broadcast abort for the active topology, and require a strictly higher new segment ID, plus authority, address, and stale-command rejection;
- a lossless 64-byte EEG frame with source, segment, sequence, shared timestamp, ADS status, eight signed 24-bit channels, configuration/topology generations, gaps, and CRC-32C;
- a separate versioned 64-byte optical result frame with source/detector identity, wavelength and source state, four accumulated detector results, saturation/coupling/fault evidence, timestamp, gaps, and CRC-32C;
- OpenBCI Cyton-compatible projection for use only at the SynDock/host boundary; and
- host-side tests independent of RP2040 or STM32 SDKs.

## Processor boundaries

- SynPod RP2040/reference controller: ADS1299 SPI/DMA, sequence/gap/fault evidence, optical-result intake, and CAN-node transport.
- Optical RP2040: OpenfNIRS-derived source execution, ADC/PIO/DMA acquisition, accumulation, and reduced optical results.
- SynDock STM32G474: discovery, addressing, common clock/control, unified epochs, timestamps, topology, fault segmentation, CAN coordination, and USB acquisition.
- SynDock ESP32-C6: wireless/application transport, updates, and display-state hosting without acquisition authority.

The C core is processor-agnostic. Board adapters provide deterministic SPI/DMA, CAN-FD, timers, GPIO, storage, and recovery without leaking vendor HAL types into the portable interfaces.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Design and evidence map

- [Firmware architecture](docs/ARCHITECTURE.md)
- [Rev A processor and interface selections](docs/REV_A_REFERENCE_DESIGN.md)
- [Reference implementation and license ledger](docs/REFERENCE_IMPLEMENTATIONS.md)
- [Firmware requirements matrix](docs/FIRMWARE_REQUIREMENTS_MATRIX.md)
- [AI-assisted electrical engineering method](docs/AI_ASSISTED_ELECTRICAL_ENGINEERING.md)
- [Independent engineer review brief](docs/ENGINEER_REVIEW_BRIEF.md)
- [Pre-fabrication release gate](docs/PRE_FABRICATION_RELEASE_GATE.md)

## Authority and qualification

The product authority is `SynEEG_SPEC.md` in Truth Forge, with `SynEEG_BOARD_DESIGN_SPEC.md` and the hardware repository's `requirements/design_contract.json` controlling the current board implementation. Passing host tests proves portable logic only. Rate, synchronization, CAN load, optical quality, noise, thermal, safety, and long-duration claims remain pending the specified physical qualification.

New SynEEG firmware and documentation in this repository are Apache-2.0. Third-party projects in the evidence ledger are references only unless an import is recorded explicitly with its provenance and license.
