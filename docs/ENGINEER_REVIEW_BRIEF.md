# Independent electrical review brief

## Engagement model

SynEEG will provide a selected Rev A circuit architecture, exact candidate parts, editable KiCad schematic and PCB, calculations, reference documents, firmware contract, automated reports, and a findings register.

The engagement is an **independent audit and correction assignment**. The reviewer is not being asked to invent the product, design straps or enclosures, select a different EEG architecture, write the experience software, or establish whether an STM32 can communicate with an ADS1299. Those boundaries are already controlled.

The reviewer is expected to exercise ordinary professional judgment, identify defects plainly, and propose the smallest evidence-backed correction that preserves the product interfaces.

## Boards in scope

1. **SynPod Rev A:** one ADS1299, one non-authoritative RP2040/reference node with external MCP2518FD-class CAN controller, eight EEG channels, separate optional OpenfNIRS-derived optical board, 20-contact Hybrid SynTrode, 16-contact SynLink, received common clock/control, clean local rails, identity storage, SWD, and recovery.
2. **SynDock Rev A:** centralized STM32G474 authority, CAN-FD collection, common clock/control, ESP32-C6, integrated display, off-head 5 V power entry, separate isolated USB data connection, protection, and explicit topology/status/fault behavior.

SynLink strap mechanics, SynPad, SynCap, enclosure industrial design, and electrode manufacture are separate work. This review covers the electrical interfaces they must satisfy, not their final mechanical construction.

## Required review passes

### Pass 1 — architecture and schematic

For every sheet, return annotated source/PDF findings and a table containing:

- finding ID and severity;
- exact sheet, net, reference designator, and pin;
- violated datasheet section, calculation, or requirement;
- consequence;
- proposed correction;
- whether the correction changes firmware, mechanics, BOM, cost, or verification;
- reviewer disposition after correction.

Review at minimum:

- ADS1299 supplies, reference, VCAP, clock, reset/start, SPI, REF, BIAS, lead-off, common mode, input protection/filtering, exposed pad, and test modes;
- SynPod RP2040, external CAN controller, ADS1299 DMA, optical-result intake, debug, and recovery;
- centralized SynDock STM32 supplies, VCAP, reset, boot, clocks, SWD, FDCAN, USB, timing authority, and recovery;
- power startup/shutdown, ±2.5 V generation, ripple/noise, dissipation, brownout, short, reverse and sequencing;
- CAN-FD and M-LVDS physical layers, contention defaults, termination, ESD, topology and connector behavior;
- SynDock USB isolation and prevention of data-port back-powering;
- exact parts, packages, symbols, footprints and sourcing.

### Pass 2 — placement, stackup and constraints

Before routing, approve or correct:

- board outline and height/keepout inputs;
- layer stackup and reference planes;
- analog, input, reference, bias, power-switching, digital, CAN, clock, debug and connector zones;
- placement of every ADS1299 support component;
- switching-current and signal-return paths;
- critical net classes, differential-pair rules, vias, clearances, impedance and test access.

### Pass 3 — routed PCB

Review the actual native PCB at trace/via/plane level. Run independent ERC, DRC and schematic-layout parity. Inspect all critical nets and all rule exclusions. Return corrected native sources or precise change instructions, followed by a clean rerun.

### Pass 4 — fabrication release

Review the exact tagged manufacturing package, BOM/CPL agreement, drawings, stackup, mask/paste, polarity/orientation, DFM report, and independent viewer render. Sign only the exact release hash/revision reviewed.

### Pass 5 — first article support

Review measured deviations during controlled bench bring-up and help isolate schematic, layout, assembly, firmware, or fixture defects. No wearer testing is part of first power-up.

## Deliverables

- annotated schematic and PCB review files;
- findings register with closure status;
- corrected native KiCad sources when corrections are included in the contract;
- independent ERC/DRC/parity reports;
- reviewed calculations and simulation comments;
- fabrication-release signoff tied to an exact source revision;
- first-article findings addendum if that gate is contracted.

Screenshots, PDFs, and verbal approval do not replace editable source and the findings register.

## Acceptance

A pass is complete when every finding has a reproducible location, evidence, disposition, and closure result. “Looks good,” “standard practice,” or a clean DRC by itself is not signoff. Conversely, a preference is not a defect: a proposed redesign must identify the failed requirement or measurable advantage that justifies it.

## AI use

Responsible AI use is welcomed. The reviewer may use AI to search manufacturer documents, compare reference implementations, generate calculations, inspect files, draft tests, or challenge assumptions. AI-assisted work must remain traceable to the exact source and must be checked against the native design and deterministic tools. Undisclosed manual copying from unlicensed projects is prohibited regardless of whether AI was involved.
