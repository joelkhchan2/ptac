# Hardware notes

## PTAC unit

Applied Comfort Products, model `DMQB09K34S7A20`, 230/208 V, cooling 9700/9600 BTU/h, heating 11900/9700 BTU/h (3.4/2.8 kW heater), 19 A min circuit.

Do not touch the unit's mains-side wiring. The robot only turns the front-panel knobs mechanically.

## The knob

- Fan/mode rotary knob: OFF (0) at top, HEAT 3/2/1 sweeping left, COOL 3/2/1 sweeping right. Total travel is roughly 250 degrees.
- A separate Temperature knob sits to the right and is not automated.
- The knob holds position on its own and turns easily by hand. Do not remove the knob or touch the shaft.
- The UI only uses OFF, HEAT3, HEAT2, COOL3, COOL2 (about +/-90 degrees from OFF). That is the most an MG996R can reach. HEAT1 / COOL1 are out of reach without a 270-degree servo or gearing; not needed for now.

## Current rig

- ESP32 dev board (ELEGOO ESP-32, 30-pin, USB-C, CP2102), jumper wires (ELEGOO Dupont set).
- Servo: Miuzei MG996R metal gear, 2 purchased (one in use, one spare). 4.8-7.2 V, about 15 kg-cm stall at 6 V, up to ~2.5 A stalled.
- Power: Arkare 5V 2A adapter (`B09W96X88K`, 5.5x2.5 mm barrel, center positive), one wall plug for everything, shared by ESP32 and servo.
- Servo is screwed to a wooden block on the shelf above the unit. The servo arm is duct-taped to the knob and currently removed from the servo.
- ESP32 and wiring sit on top of the PTAC discharge grille (warm air in heat mode).

## Known problems

1. **Servo overheated and stopped.** The old firmware attached the servo at boot and never detached, so it held torque against the knob 24/7, on a shared 5V 2A supply. Firmware now detaches 1 s after each move. Need to confirm the servo is not permanently damaged: cool down, turn horn by hand (should be smooth), bench test unloaded with the ESP32Servo `Sweep` example.
2. **Power.** 5V 2A shared between ESP32 and an MG996R is undersized. Plan: dedicated servo supply (5-6 V, 3 A+), common ground with the ESP32, 470-1000 uF capacitor across the servo supply. Optionally switch servo power with a MOSFET or relay.
3. **Overkill servo.** The knob takes little torque. A lower-torque servo (ideally 270 degrees) or a 28BYJ-48 stepper may run cooler. Measure the torque needed first.
4. **Mounting.** Duct tape on the knob melts and loosens. Plan: 3D-printed (PETG/ASA, not PLA) cap that fits over the knob, plus a servo bracket fixed to the panel or unit, coaxial with the knob or offset with a printed gear pair. Printing is done by a family member. Needs measurements first: knob diameter and height, clearance above the panel, mounting surface.
5. **Electronics placement.** Move the ESP32 and wiring off the discharge grille into a small box out of the airflow.
6. **TLS** in the firmware is `setInsecure()` (no certificate check). Switch to `setCACert()` when convenient.

## Parts list

| Item | Notes |
|---|---|
| ELEGOO ESP-32 dev board (2 pack, USB-C, 30 pin) | ASIN B0D8T7Z1P5 |
| Miuzei MG996R servo (2 pack) | ASIN B0BZ4QMSM2 |
| Arkare 5V 2A adapter with 8 tips | ASIN B09W96X88K |
| ELEGOO 120 pcs Dupont jumper wires | ASIN B01EV70C78 |

## Photo-derived reference

Photos are kept locally, not in this repo. What they showed (2026-10-01):

**Nameplate.** Applied Comfort Products Inc., Cambridge ON. Model `DMQB09K34S7A20`. 230-208 V (min 197, max 254), 60 Hz, 1 phase. Cooling 9700/9600 BTU/h, 4.6/5.0 A, 970/960 W, EER 10.0. Heating 11900/9700 BTU/h, 15.3/13.9 A, heater 3.4/2.8 kW. Indoor motor 0.45 FLA (1/20 HP), outdoor motor 0.75 FLA (1/10 HP), compressor RLA 3.8, LRA 26. Min circuit 19 A, 20 A time-delay fuse. Refrigerant HCFC22, 20.0 oz. Used with sleeve DAX99SG, DAX99AA or DAX99WE.

**Control panel (top-right of the unit, facing up under a black louvered cover).** Two knobs on a tan label, "Applied Comfort" below:
- Left, FAN SPEED: black round knob with a recessed center. Positions around the dial: 0 at top, heat 3 / 2 / 1 on the left (red arc), cool 3 / 2 / 1 on the right (blue arc). Heat 1 and cool 1 are at the lower left and lower right. This is the knob the robot turns. It currently has duct tape residue across its face.
- Right, TEMPERATURE: black knob with a pointer, WARMER (red, left) to COOLER (blue, right). Not automated.
- A sticker above the panel warns that a low ambient control prevents cooling when outdoor temperature is below 13 C (55 F).

**Rig layout.**
- Unit is wall-mounted low in a corner next to a window with a curtain, with a shelf/ledge above it. A black louvered discharge grille is on the top right and a screened intake on the top left (mesh taped over).
- Servo (Miuzei MG996R, label reads "996 Servo All-metal") is mounted on a wooden block wrapped in grey duct tape, sitting on the ledge to the right above the unit. Its orange/red/brown lead runs down to a 3-pin Dupont plug that was lying on the discharge grille.
- The ESP32 board sits on top of the unit at the grille edge, with a bundle of jumper wires running left along the top to a small white block. The white block is not part of the design; ignore it.
- Power: the Arkare adapter is plugged into a wall outlet near the floor on the left; its thin black cable runs along the baseboard and up to the unit top. The PTAC's own cream power cord goes to a separate multi-outlet strip next to it.
- At the time of the photos the servo arm was detached from the servo but still taped to the knob.
