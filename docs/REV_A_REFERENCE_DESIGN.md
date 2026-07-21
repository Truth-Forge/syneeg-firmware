# Rev A firmware-facing reference selections

This page summarizes the processor and interface choices the portable firmware must support. Detailed circuits, parts, connector mapping, and release evidence belong to the hardware repository's `requirements/design_contract.json`; product intent remains in the single Truth Forge `SynEEG_SPEC.md`.

| Boundary | Current Rev A reference |
|---|---|
| SynPod EEG AFE | one ADS1299IPAG, eight simultaneous 24-bit channels |
| SynPod local node | RP2040/reference controller; non-authoritative |
| SynPod transport | addressed half-duplex RS-485 client; RP2040-class local controller plus at least 10 Mbit/s-capable transceiver |
| SynPod timing | receives SynDock's differential 2.048 MHz clock and synchronized START/RESET |
| Hybrid optical board | separate OpenfNIRS-derived RP2040 board; one 735/850 nm source, four detectors, local PIO/DMA accumulation |
| SynTrode | separate keyed services: round ten-contact EEG plus 24-position optical with O1-O20 assigned and O21-O24 reserved |
| SynLink | sixteen contacts carrying paired power/return, RS485_A/B, differential clock, synchronized control, trigger/fault, and reserved expansion |
| SynDock authority | STM32G474CEU6 owning discovery, topology, epochs, timing, collection and USB acquisition |
| SynDock connectivity | ESP32-C6 for wireless/application/update/display-state work without acquisition authority |
| SynDock display | premium integrated status display on a separate non-acquisition boundary |

The supported portable ADS1299 profiles are 250, 500, and 1,000 SPS. A 2,000 SPS register profile is retained as a full-system qualification target and must not be represented as qualified from host tests.

OpenBCI encoding is a compatibility projection performed at SynDock or in a host bridge. Native EEG and optical carriers remain authoritative.
