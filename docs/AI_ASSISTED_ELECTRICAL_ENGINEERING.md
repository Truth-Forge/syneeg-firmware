# AI-assisted electrical engineering for SynEEG

## Product principle

SynEEG is not only an accessible EEG instrument. Its development method is part of the product story: a person does not need to begin as an electrical engineer, belong to an institution, or pay a professional to rediscover every standard circuit before they may participate.

The method is:

```text
human intent
  -> open evidence
  -> AI-assisted decisions and editable engineering artifacts
  -> deterministic machine checks
  -> focused independent expert audit
  -> physical measurements
  -> open release
```

The goal is not to remove expertise. It is to remove expertise as an all-or-nothing gate. The owner retains the product, the sources, the evidence, and the ability to understand why a decision was made.

## What AI can do in the board lifecycle now

| Stage | SynEEG use | Evidence produced |
|---|---|---|
| Requirements | Extract `shall` statements, find conflicts, build traceability and operating-state tables | requirement matrix with source line and verification method |
| Architecture | Select candidate parts and standard circuits from manufacturer references and open implementations | block diagrams, power tree, interface tables, decision ledger |
| Schematic drafting | Create the first editable KiCad schematic and symbol/footprint library | native `.kicad_sch`, PDF plots, netlist, ERC report |
| Calculation/simulation | Calculate rails, tolerances, filters, loading, fault current, CAN timing; run SPICE where valid models exist | calculation notebooks, model provenance, corner results |
| BOM | Resolve exact MPNs, lifecycle, stock, alternates, package and datasheet links | machine-readable BOM and approved-alternate rationale |
| Placement | Generate and compare floorplans; lock sensitive functional zones and mechanical keepouts | placement variants and constraint file |
| Routing | Route candidates after stackup/net classes are defined; preserve critical analog constraints | native PCB candidates, length/clearance reports |
| ERC/DRC/parity | Run deterministic checks on every change | versioned reports with every waiver explained |
| DFM | Generate fabrication data and submit it to independent fab checks | Gerbers/ODB++, drill, IPC-356, BOM/CPL, fab reports |
| Bring-up | Generate safe power-up sequences and automate scopes, logic analyzers, supplies, and data analysis | instrument logs, captures, raw EEG/test-signal data, pass/fail report |

Current tools occupy different parts of that chain. [Circuit Mind](https://www.circuitmind.io/) and [CELUS](https://www.celus.io/knowledge/design-canvas) address architecture, schematic, and BOM generation. [Flux](https://www.flux.ai/p/) and [ProtoFlow](https://www.protoflow.ai/) assist early schematic work. [Quilter](https://docs.quilter.ai/using-quilter/design-your-schematic), [DeepPCB](https://deeppcb.ai/deeppcb-kicad-plugin-ai-pcb-routing/), Cadence Allegro X AI, and Zuken AIPR generate placement/routing candidates with different levels of maturity. KiCad supplies the native editable database and deterministic ERC/DRC/export layer.

Independent public evidence shows why review is still useful: a Los Alamos evaluation of Circuit Mind found substantial time savings on supported designs while still finding manual corrections such as an unconnected USB rail, resistor values, an exposed pad, and missing Ethernet magnetics. That is the model SynEEG adopts—large drafting acceleration plus explicit audit—not blind output acceptance. [LANL evaluation](https://5659512.fs1.hubspotusercontent-na1.net/hubfs/5659512/LA-UR-25-30842_Rev1.pdf).

## What we can complete before sending anything to a designer

The designer should receive an actual candidate design package. Before engagement, AI and the product owner can complete:

1. the controlled SynEEG requirement matrix;
2. the Rev A exact-part and circuit selections;
3. operating states for off, boot, solo acquisition, coordinated acquisition, contact check, topology change, fault, update, and shutdown;
4. electrical interface-control tables for SynTrode, SynLink/SynBus, ADS1299, SynDock, USB, programming, and test;
5. a real KiCad schematic with named nets, manufacturer-linked parts, footprints, test points, and design notes;
6. a preliminary PCB outline derived from the printed enclosure envelope;
7. component floorplan and keepouts separating electrode, analog, power-switching, clock, digital, and bus zones;
8. a preliminary routed PCB or multiple route candidates;
9. rail, current, voltage-drop, clock, CAN, M-LVDS, filter, leakage, fault-current, and tolerance calculations;
10. ERC, DRC, schematic-layout parity, BOM, footprint, and pin-map reports;
11. a risk register with one requested review disposition per risk;
12. a verification plan and first-power bring-up procedure.

The designer is then contracted to audit the package in gates:

- **Gate A — circuit audit:** approve or mark corrections on every sheet and calculation;
- **Gate B — floorplan audit:** approve analog zones, return paths, power, clock, bus topology, stackup, and test access;
- **Gate C — routed-board audit:** review every critical net, plane, via transition, clearance, and footprint;
- **Gate D — fabrication release:** independently rerun checks, review outputs, and sign the exact revision;
- **Gate E — first article:** assist only with defects that appear in measured bring-up.

Payment can be tied to each completed review artifact. The engagement is sized around verification and correction rather than an undefined promise to “design an EEG system.”

## Local implementation path

KiCad is installed locally and its command-line checker is available inside the application bundle. The next hardware repository can therefore be fully source-controlled and tested locally:

```text
SynEEG_SPEC.md
  -> requirements.csv
  -> syneeg-hardware KiCad sources
  -> ERC / DRC / parity / BOM / fabrication checks
  -> engineer findings register
  -> corrected release candidate
```

Generative placement or routing tools can be trialed on copies of the same KiCad source. No cloud tool becomes the authority, and no proprietary AI output is allowed to replace the native editable files.

## What AI cannot establish without a physical board

This is a measurement boundary, not a statement that the product is inaccessible. Before a board exists, no person or AI can observe its actual:

- shorted-input noise and interference spectrum;
- rail ripple at the ADS1299 pins;
- conversion-clock integrity and multi-node skew;
- connector resistance, cable motion artifact, and SynLink reflections;
- temperature rise inside the final enclosure;
- ESD recovery, overload recovery, leakage, and first-fault behavior;
- assembly defects and component variation.

Those questions become ordinary bench measurements. The repository defines the test, expected limit, equipment, fixture, and recording format in advance so the evidence can be gathered affordably and reviewed remotely.

## Democratization acceptance criteria

The development method succeeds when a competent community member can:

- inspect the exact reason and source for a part or circuit;
- build the firmware and run its tests without proprietary software;
- open and edit the hardware in KiCad;
- reproduce every automated design check;
- order a prototype from ordinary fabrication outputs;
- follow a bounded bring-up procedure;
- see measured failures rather than hidden expert conclusions; and
- replace or improve a part without asking permission from a closed platform.
