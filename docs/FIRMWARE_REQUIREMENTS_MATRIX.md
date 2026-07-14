# Firmware requirements matrix

This matrix traces the first repository baseline to the governing SynEEG specification. `Implemented` means host-testable portable behavior exists. `Target contract` means the behavior is specified but depends on the released STM32 board integration. `Verification gate` means only physical or multi-node evidence can close it.

| Product requirement | Firmware interpretation | Current artifact | Status | Release evidence |
|---|---|---|---|---|
| `INV-001`, `INV-002` | One ADS1299 and eight channels per active SynPod | `ads1299.h`, `sample.h` | Implemented | ADS ID and eight decoded channels on target |
| `INV-011` | One elected acquisition epoch per cap | `coordinator.c`, architecture §6 | Partial implementation | multi-node start/skew/drift test |
| `POD-ELEC-003` | Lead-off uses existing channel paths | ADS register map and profile boundary | Target contract | contact fixture and status-bit test |
| `POD-ELEC-004` | Test/contact state is explicit; configuration changes segment | native frame flags and architecture §5–6 | Target contract | command/stream integration test |
| `POD-ELEC-005` | Base profile is 250 SPS | `syneeg_ads1299_profile_8ch_250sps()` | Implemented | `DRDY` period measured on board |
| `POD-ELEC-006` | Sample capture never waits on CAN/USB | architecture §4; STM32 platform contract | Target contract | timing trace under maximum transport load |
| `BUS-004` | Deterministic temporary coordinator | `coordinator.c` | Implemented foundation | multi-node election/failure test |
| `BUS-005` | Leader/topology change requires segment | `coordinator.c`, architecture §6.4 | Implemented foundation | injected join/loss/recovery test |
| `BUS-006` | Start, skew, drift, correction and failure are measured | Rev A shared M-LVDS clock and architecture | Verification gate | long-duration multi-node clock report |
| `BUS-007` | Separate caps retain separate clock domains | segment and node metadata | Target contract | host integration test |
| `DATA-001` | Sample identifies node, segment, sequence and generations | native 64-byte frame | Implemented | golden vectors and descriptor round trip |
| `DATA-002` | Preserve ADS status and signed raw 24-bit counts | `ads1299.c`, `protocol.c` | Implemented | TI test signal and range vectors |
| `DATA-003` | Gaps, resets and topology changes remain explicit | sequence, `gaps_before`, segment state | Implemented foundation | overrun/reset/topology fault injection |
| `DATA-004` | Electronic node identity is separate from physical position | node ID in samples; mapping in descriptors | Target contract | move/replace-node integration test |
| `OPEN-001` | Editable firmware, build, recovery and evidence | CMake, CI, docs, reference ledger | Partial | clean third-party STM32 build and flash |
| `OPEN-002` | BrainFlow works over wired SynDock USB | OpenBCI encoder and SynDock boundary | Partial | BrainFlow discovery/stream/recovery suite |
| `OPEN-003` | Descriptors cover 8 through 64 channels | one frame per eight-channel node | Target contract | synthetic 1–8 node host tests |
| `OPEN-004` | LSL/MNE bridge preserves identity and gaps | native metadata defined | Not in firmware baseline | host bridge and recorded-data examples |

## Baseline definition of done

The portable baseline is complete when all host tests pass from a clean checkout and every implemented wire format has golden vectors. The STM32 target is complete only after generated peripheral code, timing traces, fault injection, BrainFlow integration, and board measurements close the target and verification rows. Host tests do not convert target contracts into measured claims.
