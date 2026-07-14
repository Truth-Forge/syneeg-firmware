# SynDock STM32 integration boundary

The portable core does not commit generated STM32Cube code. The STM32G474CEU6 is centralized in SynDock and is the permanent acquisition authority; SynPods use non-authoritative RP2040/reference controllers with external MCP2518FD-class CAN-FD controllers.

The generated SynDock target must provide:

1. CAN-FD/BRS coordination for up to eight addressed Pods, with hardware timestamps, filters, bus-off recovery, and explicit loss/topology state.
2. The shared 2.048 MHz ADS1299 clock plus synchronized `START` and `RESET_N` control distributed to Pods.
3. Monotonic segment/epoch timing and fixed queues for native EEG and reduced optical frames.
4. Native isolated USB acquisition that remains independent of ESP32-C6 radio, update, and display workload.
5. Deterministic framed transfer to the ESP32-C6 with ready/interrupt handshake.
6. SWD production programming, immutable authority identity, watchdog, brownout handling, fault segmentation, and recovery.

Generated code should live under a target-specific directory and call the portable APIs. ADS1299 SPI/DMA belongs to the SynPod RP2040 adapter, not this STM32 target. No platform layer may duplicate packet definitions or move acquisition authority into the ESP32-C6 or a Pod.
