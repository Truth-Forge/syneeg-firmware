# STM32 integration boundary

The portable core does not commit generated STM32Cube code because the exact schematic must first freeze the MCU suffix, pins, oscillators, DMA requests, FDCAN instance, debug connector, and clock-distribution circuit.

The selected Rev A candidate is STM32G474CEU6. The generated target must provide:

1. ADS1299 SPI mode 1 with hardware-controlled timing and a nonblocking DMA transfer path.
2. Falling-edge `DRDY` external interrupt that starts a 27-byte SPI DMA read without waiting on transport.
3. Fixed sample buffers and a bounded single-producer/single-consumer queue.
4. FDCAN with CAN-FD/BRS support, hardware receive timestamps, explicit bus-off recovery, and filters for control versus sample frames.
5. A monotonic 64-bit epoch tick derived from a hardware timer.
6. GPIO for ADS1299 `CS`, `RESET`, `START`, and any released power-down control.
7. GPIO for clock-source mux selection and M-LVDS driver enable; changes are allowed only while acquisition is stopped.
8. SWD production programming, immutable unit identity, calibration storage, watchdog, brownout handling, and a recovery path.

Generated code should live under a target-specific directory and call the portable APIs. It must not duplicate ADS1299 register policy or packet definitions.
