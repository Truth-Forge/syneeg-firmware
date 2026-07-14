# Firmware requirements matrix

`Implemented` means portable behavior has host tests. `Target contract` requires board integration. `Verification gate` requires physical or multi-node evidence; host tests never close it.

| Product requirement | Firmware interpretation | Current artifact | Status | Remaining evidence |
|---|---|---|---|---|
| `INV-001`, `INV-002` | One ADS1299/eight EEG channels per active Pod; no authoritative Pod STM32 | `ads1299.*`, `sample.h` | Implemented portable core | ADS ID, DMA and eight-channel target trace |
| `POD-ELEC-005` | Selectable 250/500/1000 SPS; 1000 primary; 2000 qualification target | rate metadata/profile tests | Implemented portable core | rate, load, noise, timing and endurance per topology |
| `POD-ELEC-006`, `009` | Local RP2040/reference node owns deterministic ADS capture and external MCP2518FD-class CAN transport | architecture §2–3 | Target contract | RP2040 DMA/CAN load and recovery traces |
| `POD-ELEC-010` | Hybrid optical reduced results pass through the common Pod CAN node | optical semantic/frame types | Implemented contract | inter-processor and CAN integration test |
| `POD-OPT-001` | Optical RP2040 owns OpenfNIRS-derived local acquisition and accumulation | optical result boundary | Implemented contract | hardware/firmware provenance and optical bench test |
| `BUS-004` | SynDock STM32 is permanent coordinator; Pods are addressed clients and never elect | `coordinator.*` | Implemented portable state | CAN discovery/address/control integration |
| `BUS-005` | Topology or authority/timing fault enters stop-required, emits STOP for the active topology, then requires a strictly higher segment ID | coordinator/node transition tests | Implemented foundation | injected join/loss/timing-fault target test |
| `BUS-006` | Start alignment, skew, drift and recovery are measured | shared timestamp/clock-quality fields | Verification gate | qualified 1–8 Pod synchronization report |
| `BUS-008` | One-Pod and multi-Pod carriers use the same CAN authority path | solo/multi coordinator tests | Implemented portable state | one/two/eight-Pod hardware test |
| `DOCK-008` | SynDock discovers/configures Pods, distributes clock/control, owns epochs and collects frames | coordinator and architecture | Partial | STM32G474 CAN/clock/USB integration |
| `DOCK-009` | ESP32-C6/display cannot interrupt or replace wired acquisition authority | processor boundary | Target contract | reboot/congestion/display-load fault injection |
| `DATA-001`–`004` | EEG retains node, segment, sequence, time, status, 24-bit values, topology/config generations and gaps | 64-byte EEG frame | Implemented | descriptors and full host integration |
| `DATA-005` | Optical retains Pod/source/detectors, wavelength/state, accumulation, quality, gaps and shared time | 64-byte optical frame | Implemented | OpenfNIRS board integration and golden target vectors |
| `OPEN-002` | BrainFlow operates over wired SynDock USB as a lossy EEG projection | OpenBCI encoder | Partial | discovery/rate/timing/contact/recovery suite |
| `OPEN-003`, `010` | Native descriptors cover EEG plus optical groups; optical exports through native/SNIRF path | versioned frame types | Target contract | descriptors, recorder and SNIRF bridge |

## Portable baseline definition of done

The portable baseline requires clean configure/build/test runs and golden round trips for every implemented wire frame. The RP2040 and STM32 targets require independent timing, topology, fault, transport, analog, optical, power, thermal, safety, and long-duration evidence before product qualification.
