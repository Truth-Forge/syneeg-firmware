# SynEEG Firmware

Open firmware foundation for the SynEEG eight-channel SynPod and its SynDock gateway.

The first hardware target is one ADS1299 and one STM32-family controller per active SynPod. Each SynPod acquires eight signed 24-bit channels at 250 samples per second, preserves the ADS1299 status word, and can operate alone or participate in one coordinated cap epoch. SynDock carries the wired host boundary and translates native SynEEG traffic for host software such as BrainFlow.

This repository deliberately separates three kinds of truth:

- `src/` and `include/` are portable, host-tested firmware behavior.
- `platform/stm32/` is the hardware integration contract that a generated STM32Cube project must satisfy.
- `docs/` records selected parts, evidence, open engineering decisions, verification gates, and the boundary between automated checks and physical validation.

## Why this repository exists

SynEEG is being developed on the premise that serious instrumentation can be made more accessible without hiding it behind expert-only workflows. Reference designs, manufacturer documentation, AI-assisted drafting, automated checking, editable sources, inexpensive fabrication, and focused expert review can turn an open question into an auditable design.

Expert review remains valuable, but it is applied to concrete claims and artifacts. The reviewer receives a selected circuit, a selected bill of materials, calculations, machine-check results, and explicit questions—not an invitation to redefine the product or charge for rediscovering standard patterns.

## Current scope

Implemented now:

- ADS1299 command and register transport with readback verification;
- a conservative eight-channel, 250-SPS configuration profile;
- 27-byte conversion-frame decoding and signed 24-bit expansion;
- a fixed native sample frame that fits one CAN-FD payload;
- OpenBCI Cyton-compatible eight-channel packet encoding at the SynDock boundary;
- a tested minimum-stable-node election primitive that preserves a required segment boundary when observed membership changes;
- host-side tests that run without a board.

Reserved for the board integration stage:

- STM32Cube-generated startup and peripheral initialization;
- `DRDY` interrupt to SPI DMA wiring;
- FDCAN filters, queues, timestamps, and recovery;
- full discovery, membership snapshot, proposal/acknowledgement, duplicate-identity, and coordinated-start protocol messages;
- production identity/calibration storage;
- bootloader and signed recovery image;
- physical clock-source switching and elected clock distribution;
- hardware-in-loop and analog performance qualification.

## Build and test

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Design and evidence map

- [Firmware architecture](docs/ARCHITECTURE.md)
- [Rev A selected hardware stack](docs/REV_A_REFERENCE_DESIGN.md)
- [Reference implementation and license ledger](docs/REFERENCE_IMPLEMENTATIONS.md)
- [Firmware requirements matrix](docs/FIRMWARE_REQUIREMENTS_MATRIX.md)
- [AI-assisted electrical engineering method](docs/AI_ASSISTED_ELECTRICAL_ENGINEERING.md)
- [Independent engineer review brief](docs/ENGINEER_REVIEW_BRIEF.md)
- [Pre-fabrication release gate](docs/PRE_FABRICATION_RELEASE_GATE.md)

## Authority

The product definition remains the SynEEG system specification in Truth Forge. This repository implements that specification; it does not silently replace it. Exact hardware choices proposed here are Rev A recommendations until they are incorporated into a released schematic and the governing specification.

## Licensing

New SynEEG firmware and documentation in this repository are released under Apache-2.0. The permissive license and explicit patent grant support community use while preserving source provenance. Third-party projects listed in the evidence ledger are references only and are not vendored here. The future hardware repository will make a separate explicit open-hardware license selection appropriate to editable PCB sources.
