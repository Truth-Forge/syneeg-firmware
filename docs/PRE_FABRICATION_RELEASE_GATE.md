# Pre-fabrication release gate

No SynEEG board package is sent for fabrication until the exact revision passes this checklist. “Printed” in this document means PCB fabrication and assembly, not the earlier non-powered enclosure fit print.

## Design authority

- [ ] Product revision and prototype purpose are named.
- [ ] Every schematic requirement traces to the current SynEEG specification.
- [ ] Exact SynPod or SynDock board outline, height zones, connectors, enclosure, and service sequence are frozen for this prototype.
- [ ] Every change requested by the independent reviewer is closed, rejected with evidence, or recorded as an accepted prototype limitation.

## Schematic

- [ ] Every symbol pin number and function has been checked against the current manufacturer datasheet.
- [ ] ADS1299 supplies, reference, VCAP capacitors, clock, reset/start, SPI, REF, BIAS, lead-off, exposed pad, and input common-mode paths have a sheet-level audit.
- [ ] STM32 power, VCAP, reset, boot, SWD, oscillators, decoupling, and alternate-function pins have a sheet-level audit.
- [ ] CAN-FD, M-LVDS clock distribution, termination, clock mux defaults, and driver contention states have a sheet-level audit.
- [ ] Every regulator and protection circuit has startup, shutdown, brownout, reverse, short, and fault behavior documented.
- [ ] USB power and USB data domains in SynDock cannot unintentionally bypass isolation or back-power the worn system.
- [ ] ERC has zero unexplained errors or warnings.

## Parts and footprints

- [ ] BOM contains exact manufacturer part numbers, value, tolerance, voltage rating, package, lifecycle, supplier link, and DNP state.
- [ ] Every footprint, exposed pad, pin-one mark, polarity, connector orientation, courtyard, and manufacturer land pattern has been checked.
- [ ] Symbols/footprints downloaded from a library have been rechecked against the manufacturer document.
- [ ] Approved alternates are electrically, mechanically, thermally, and firmware compatible; other substitutions are prohibited.
- [ ] Stock and lifecycle are checked immediately before order.

## Layout

- [ ] Fabricator stackup, material, finished thickness, copper weight, via process, drill limits, and controlled-impedance rules are entered into the PCB source.
- [ ] ADS1299 input, REF, BIAS, reference, VCAP, and supply loops are reviewed at component-and-via level.
- [ ] LM27762 charge-pump loop and switching return remain outside the sensitive input-current paths.
- [ ] Clock, SPI, CAN, M-LVDS, LED, and MCU return currents have continuous controlled paths.
- [ ] CAN and M-LVDS topology, termination, stubs, branch count, and common-mode limits match the released SynLink configuration.
- [ ] Test points remain accessible after assembly for every rail, ground, clock, `DRDY`, SPI, CAN, M-LVDS, reset, boot, SWD, and fault output.
- [ ] PCB and solder joints carry no enclosure or SynLink structural load.
- [ ] DRC and schematic-layout parity have zero unexplained errors or warnings.

## Analysis

- [ ] Power budget includes typical, startup, maximum, five-node, short-circuit, and brownout cases.
- [ ] SynLink voltage drop is calculated at minimum source voltage and maximum qualified length/topology.
- [ ] Analog filter, input protection leakage, resistor noise, channel mismatch, bandwidth, and overload recovery are calculated at tolerance corners.
- [ ] CAN-FD bit timing and oscillator tolerance are calculated for the released bus length and node count.
- [ ] M-LVDS clock termination and signal integrity are simulated or otherwise justified for the released topology.
- [ ] Rail simulations use traceable models and include startup and load transients where applicable.
- [ ] Thermal estimates fit the closed enclosure and expected wearer-facing temperature limit.

## Manufacturing package

- [ ] Native KiCad source is tagged with the exact revision.
- [ ] Gerber or IPC-2581/ODB++, NC drill/slots, fab drawing, stackup, assembly drawing, BOM, and CPL are generated from that tag.
- [ ] IPC-356 netlist is generated when supported.
- [ ] Reference designators agree across schematic, PCB, BOM, CPL, and assembly drawing.
- [ ] Paste, mask, thermal relief, via tenting/filling, and panelization assumptions are explicit.
- [ ] Outputs have been opened in an independent viewer and compared with the native design.
- [ ] The fabrication house’s automated DFM report passes.
- [ ] Any proposed CAM edit returns to the source design and is regenerated; the fab may not silently change the board.

## First article plan

- [ ] Prototype quantity is intentionally small.
- [ ] Bring-up uses a current-limited source and defined stop conditions.
- [ ] Programming and recovery image are built from the tagged firmware revision.
- [ ] Test fixtures and expected waveforms/register values are ready before boards arrive.
- [ ] Sequence covers unpowered inspection, resistance checks, rail-by-rail power, clock, reset, ADS ID, register readback, internal test signal, shorted-input noise, CAN, USB, and multi-node timing.
- [ ] Raw instrument data, firmware logs, photos, board serial, rework, and deviations have a recording location.
- [ ] A first article is not worn until the specification’s bench electrical, fault-current, isolation, thermal, material, and mechanical-release gates are complete.

## Release signatures

| Role | Name | Exact revision | Date | Disposition |
|---|---|---|---|---|
| Product owner |  |  |  |  |
| AI-assisted design package audit |  |  |  |  |
| Independent electrical reviewer |  |  |  |  |
| Fabrication/assembly DFM |  |  |  |  |
