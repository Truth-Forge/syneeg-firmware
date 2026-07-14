# Rev A reference design selections

Status: **selected design target for drafting and independent review**. These are no longer open-ended choices for a contractor. A reviewer may change one only by identifying the failed requirement, the evidence for the failure, and a compatible replacement.

This document turns the product-level capability slots into a concrete SynPod and SynDock starting point. It is not a fabrication release. Exact passives, footprints, and pins become release-controlled in the KiCad hardware repository.

## 1. SynPod core

| Function | Rev A selection | Reason |
|---|---|---|
| EEG AFE | **ADS1299IPAG**, one per active SynPod | Product invariant; eight 24-bit simultaneous channels, internal reference, bias drive, lead-off, test signals |
| Controller | **STM32G474CEU6**, UFQFPN48 | Compact 7 × 7 mm package; three FDCAN controllers, USB device capability, SPI, DMA, timers, CRC, sufficient memory and debug resources |
| CAN-FD physical layer | **MCP2562FD-E/MF**, 3 × 3 mm DFN8 | CAN FD, separate 3.3 V VIO, 5 V bus supply, widely implemented physical layer |
| Conversion-clock distribution | **SN65MLVD206BDR** M-LVDS transceiver | Every active SynPod can receive the same elected 2.048 MHz clock; only the elected coordinator enables its driver |
| Local conversion oscillator | **SG-8018CG 2.0480M-TJHSA0**, 3.3 V CMOS | Active exact 2.048 MHz orderable part, compact 2.5 × 2.0 mm package, direct ADS1299 clock frequency |
| Clock selection | **74LVC1G157GW,125** | Small digital 2:1 mux selects local or received clock while acquisition is stopped |
| Unit/calibration storage | **M24C64-RMC6TG**, UFDFPN8 | 64-Kbit I²C EEPROM for identity, board revision, calibration, production results, and write-protected release data |
| Debug/programming | **Tag-Connect TC2030-IDC-NL footprint plus SWD pads** | No permanent bulky connector; supports production and recovery access |

Primary sources: [ADS1299 datasheet](https://www.ti.com/lit/ds/symlink/ads1299.pdf), [STM32G474CE product page](https://www.st.com/en/microcontrollers-microprocessors/stm32g474ce.html), [MCP2562FD datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP2561-2FD-High-Speed-CAN-Flexible-Data-Rate-Transceiver-DS20005284.pdf), [SN65MLVD206B datasheet](https://www.ti.com/lit/ds/symlink/sn65mlvd206b.pdf), [SG-8018CG product page](https://www.epsondevice.com/crystal/en/products/crystal-oscillator/sg8018cg.html), [74LVC1G157 product page](https://www.nexperia.com/products/analog-logic-ics/logic/decoders-and-demultiplexers-digital-multiplexers/serie/74lvc1g157/), and [M24C64 datasheet](https://www.st.com/resource/en/datasheet/m24c64-r.pdf).

## 2. SynPod power tree

Rev A uses a **regulated 5.0 V SynBus**. The first release does not boost the head bus to 9 V or 12 V. This avoids placing a high-frequency buck converter beside every ADS1299 and keeps a five-node system within the ordinary capability of a powered USB battery source and appropriately sized SynLink conductors.

```text
SynLink 5V_BUS
    |
    +-- input fuse/current limit + reverse/ESD protection
    |
    +-- MCP2562FD VDD (5 V)
    |
    +-- TPS7A2033PDBVR --> 3V3_DIG --> STM32, VIO, M-LVDS, EEPROM, oscillator
    |
    +-- LM27762DSSR --> +2V5_A and -2V5_A --> ADS1299 AVDD and AVSS

ADS1299 DVDD <-- filtered 3V3_DIG branch
ADS1299 reference <-- internal 4.5 V reference, decoupled per TI reference circuit
```

Selected power devices:

- **TPS7A2033PDBVR** for the 3.3 V rail. It is an active 300 mA, 7 µV RMS, high-PSRR LDO in SOT-23-5. The first prototype uses the serviceable SOT-23 package instead of wafer-level packaging.
- **LM27762DSSR** for regulated +2.5 V and -2.5 V analog rails from the 5 V bus. It combines the inverting charge pump and positive/negative LDOs in 3 × 2 mm. Its 2 MHz switching node, flying capacitor, and return loop form a strict analog-layout exclusion zone and require measured ripple/noise verification.

Primary sources: [TPS7A20 product page](https://www.ti.com/product/TPS7A20) and [LM27762 datasheet](https://www.ti.com/lit/ds/symlink/lm27762.pdf).

Power calculations that the KiCad design package must freeze:

1. maximum and acquisition-typical current for every rail;
2. five-active-node SynBus current, conductor loss, connector loss, and minimum node voltage;
3. LM27762 output-divider values and tolerance corners for ±2.5 V;
4. startup sequencing and ADS1299 absolute-maximum compliance;
5. regulator dissipation and enclosure temperature rise;
6. analog ripple at the ADS1299 supply pins and its measured appearance in shorted-input data.

## 3. Ten-path SynTrode circuit

The electrical contract is exactly ten wearer-facing conductors:

1. eight channel electrodes;
2. one common REF electrode; and
3. one BIAS electrode.

Rev A follows the established OpenBCI-style common-reference configuration:

- each channel electrode routes to its dedicated ADS1299 channel input through the released protection/filter cell;
- REF routes to SRB2;
- each `CHnSET` closes the SRB2 switch, uses normal electrode input, and defaults to gain 24;
- BIAS routes from `BIASOUT` through its separately stabilized, current-limited protection cell;
- both positive and negative channel inputs participate in the internal bias derivation by default;
- lead-off sense uses those same paths and adds no eleventh conductor.

The current firmware profile therefore programs `CONFIG1=0x96`, `CONFIG2=0xC0`, `CONFIG3=0xEC`, `CH1SET..CH8SET=0x68`, `BIAS_SENSP=0xFF`, `BIAS_SENSN=0xFF`, `MISC1=0x00`, and `CONFIG4=0x00`. Lead-off sense bits remain off during ordinary acquisition. These bytes must be decoded into a schematic-review table rather than accepted as unexplained constants.

For the protection/filter cell, Rev A starts from the TI ADS1299EEG-FE and OpenBCI Cyton implementation, then performs a leakage/noise/fault-current audit before values are frozen. The design may not substitute ordinary high-leakage TVS parts on an EEG input merely because they pass ESD. Each selected element must have maximum leakage, capacitance, noise contribution, tolerance, voltage rating, and recovery documented. [ADS1299EEG-FE user guide](https://www.ti.com/lit/ug/slau443b/slau443b.pdf) and [OpenBCI V3 hardware files](https://github.com/OpenBCI/V3_Hardware_Design_Files).

## 4. Elected common sample clock

CAN FD remains the control and sample-data network. CAN election alone does not force independent oscillators to remain phase-aligned, so Rev A adds one differential M-LVDS clock pair to the SynBus.

The resulting electrically enabled SynLink has six conductors:

1. `5V_BUS`;
2. `GND_BUS`;
3. `CAN_H`;
4. `CAN_L`;
5. `MCLK_P`; and
6. `MCLK_N`.

Every active SynPod contains the same local oscillator, clock mux, and M-LVDS transceiver:

- solo node: local 2.048 MHz oscillator selected, M-LVDS driver disabled;
- elected coordinator: local oscillator selected, M-LVDS driver enabled;
- follower: received M-LVDS clock selected, driver disabled;
- topology or coordinator change: stop acquisition, disable all drivers, elect, select clocks, verify clock presence, and begin a new epoch.

This is decentralized: there is no permanent master or additional timing module. One ordinary SynPod temporarily supplies the shared physical conversion clock. The selected M-LVDS part is designed for multipoint clock/data distribution. Exact topology, termination, stub length, and connector pin assignment must be proven against the released SynLink geometry before fabrication.

Rev A also selects **CAN FD with bit-rate switching**, initially targeting 1 Mbit/s arbitration and 4 Mbit/s data. One native sample is one 64-byte CAN-FD frame, or 16 kB/s of payload per active Pod at 250 SPS. Five active Pods produce 80 kB/s; eight produce 128 kB/s. The exact on-wire load, including arbitration, control traffic, bit stuffing, retries, and margin, must be calculated from the final FDCAN timing and verified under injected errors. Classical CAN remains a fallback for control and recovery, not the normal multi-Pod sample transport.

## 5. SynDock Rev A

SynDock is a gateway and power boundary, not the EEG sampler or permanent timekeeper.

| Function | Rev A selection |
|---|---|
| Gateway MCU | **STM32G474CEU6**, shared toolchain and protocol code with SynPod |
| SynBus CAN-FD | **MCP2562FD-E/MF** |
| Data-port isolation | **ADuM3165** full-/low-speed USB digital isolator |
| Power input | dedicated USB-C receptacle configured as a 5 V sink from a floating battery pack |
| Data output | separate USB-C USB 2.0 device port, operating simultaneously with power input |
| Worn-side power protection | current limit/eFuse, reverse blocking, ESD, short-circuit and fault reporting selected in the SynDock schematic |

The earlier USB2512B assumption is removed from Rev A. A two-port USB hub is unnecessary when one connector is deliberately power-only and the other is one USB device data link. The STM32 is the USB-to-SynBus gateway. Removing the hub reduces size, firmware ambiguity, power, and BOM count without changing the two-connector consumer behavior.

The data-port host side and worn/battery side receive separate power domains around the USB isolator. The USB data connector must not become an unintended power source for the worn system. [ADuM3165 product page](https://www.analog.com/en/products/adum3165.html).

## 6. Board form and layout direction

Rev A is one compact double-sided rigid PCB, not M.2, rigid-flex, or a removable internal SynCard. The footprint parallel to the head is minimized; thickness can be used to accommodate enclosure load paths and stacked component-height zones.

Initial layout rules:

- ADS1299, reference/VCAP capacitors, input filters, REF, and BIAS remain in one quiet analog zone adjacent to the SynTrode interface.
- LM27762 switching loop is compact and physically separated from electrode traces; no electrode trace crosses its switching-current return area.
- STM32, CAN, M-LVDS, LEDs, SWD, and EEPROM occupy the digital/SynLink side.
- continuous reference planes and controlled return paths are preferred over an arbitrary split-ground doctrine; any partition or bridge is justified by current flow.
- no structural load enters the PCB, connector solder joints, ADS1299, or SynTrode contacts.
- test access exists for every rail, ADS clock, `DRDY`, SPI, CAN, M-LVDS, reset, boot, and SWD.

## 7. What remains for audit rather than invention

The reviewing engineer receives these selections and is asked to close:

- exact pin assignment and DMA/peripheral mapping;
- exact protection/filter cell values after leakage, noise, bandwidth, and fault calculations;
- exact power-input protection/eFuse circuit;
- exact CAN and M-LVDS terminations and released topology;
- exact decoupling values and placement from manufacturer guidance;
- exact stackup, impedance rules, spacing, via sizes, and assembly limits;
- simulations and measured acceptance limits;
- any necessary part replacement supported by a written equivalence analysis.

That is a bounded design audit. It is not permission to reopen the one-ADS1299/eight-channel SynPod, the separate SynDock, the SynTrode contract, or the accessible open-system intent.
