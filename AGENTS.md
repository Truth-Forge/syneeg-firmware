# SynEEG Firmware — Agent Contract

**Universal coding policy:**
`/Users/jeremyserna/truth_forge/docs/ecosystem/BUILD_PROCESS_PORTABLE.html`.
Read it before creating or materially changing software. `CODE_POLICY_BINDING.json`
is this repository's machine-readable pointer to that policy, not a policy copy.

Portable firmware behavior for the distributed SynEEG EEG/fNIRS system. **This repo owns executable protocol and acquisition behavior** — the other SynEEG repos may cite that behavior but must not create a competing protocol authority.

## System invariants (from `README.md` — binding)

- Each active SynPod captures one ADS1299 locally through a non-authoritative RP2040-class node; transports identified records as an addressed half-duplex RS-485 client under SynDock-controlled transmit slots.
- **SynDock is the permanent system authority** (STM32G474): discovers/addresses Pods, owns topology generations and segment transitions, distributes the 2.048 MHz clock, collects records, exposes the native wired stream.
- The ESP32-C6 and display are operational/connectivity surfaces; **neither can become acquisition authority.**
- Raw optical ADC samples remain local to the optical board.

## Authority chain

1. Truth Forge's `SynEEG_SPEC.md` owns product intent.
2. This repo owns protocol + firmware behavior (CMake build, `src/`, `include/`, `platform/`, `tests/`).
3. `syneeg-hardware` owns hardware implementation; `syneeg-pipeline` decodes this repo's committed 64-byte frames — a frame-format change here is a cross-repo contract change; surface it, don't slip it in.

## Conventions (all agents)

- Dirty tree is inheritance, not an error. Commit by explicit paths only.
- No medical framing. Product decisions are encoded, not reopened.
