# Rev A firmware-facing reference selections

This page summarizes the processor and interface choices the portable firmware must support. Detailed circuits, parts, connector mapping, and release evidence belong to `/Users/jeremyserna/apps_dev/syneeg-hardware/requirements/design_contract.json` and the Truth Forge board design specification.

| Boundary | Current Rev A reference |
|---|---|
| SynPod EEG AFE | one ADS1299IPAG, eight simultaneous 24-bit channels |
| SynPod local node | RP2040/reference controller; non-authoritative |
| SynPod CAN | external MCP2518FD-class controller plus MCP2562FD transceiver |
| SynPod timing | receives SynDock's differential 2.048 MHz clock and synchronized START/RESET |
| Hybrid optical board | separate OpenfNIRS-derived RP2040 board; one 735/850 nm source, four detectors, local PIO/DMA accumulation |
| SynTrode | keyed 20-contact interface: ten EEG plus ten physically separated optical contacts |
| SynLink | sixteen contacts carrying paired power/return, CAN FD, differential clock, synchronized control, trigger/fault, and reserved expansion |
| SynDock authority | STM32G474CEU6 owning discovery, topology, epochs, timing, collection and USB acquisition |
| SynDock connectivity | ESP32-C6 for wireless/application/update/display-state work without acquisition authority |
| SynDock display | premium integrated status display on a separate non-acquisition boundary |

The supported portable ADS1299 profiles are 250, 500, and 1,000 SPS. A 2,000 SPS register profile is retained as a full-system qualification target and must not be represented as qualified from host tests.

OpenBCI encoding is a compatibility projection performed at SynDock or in a host bridge. Native EEG and optical carriers remain authoritative.
