# Hardware notes

## PTAC unit

Applied Comfort Products, model `DMQB09K34S7A20`, 230/208 V, cooling 9700/9600 BTU/h, heating 11900/9700 BTU/h (3.4/2.8 kW heater), 19 A min circuit.

Do not touch the unit's mains-side wiring. The robot only turns the front-panel knobs mechanically.

## The knob

- Fan/mode rotary knob: OFF (0) at top, HEAT 3/2/1 sweeping left, COOL 3/2/1 sweeping right. Total travel is roughly 250 degrees.
- A separate Temperature knob sits to the right and is not automated.
- The knob holds position on its own and turns easily by hand. Do not remove the knob or touch the shaft. "Holds by itself" is assumed, not yet tested with the servo released; see `test-plan.md` (24-hour hold test).
- The UI only uses OFF, HEAT3, HEAT2, COOL3, COOL2 (about +/-90 degrees from OFF). That is the most an MG996R can reach. HEAT1 / COOL1 are out of reach without a 270-degree servo or gearing; not needed for now.
- The firmware and UI currently allow 500-2500 us (0-180 degrees). The MG996R's real usable range is probably smaller, and driving it into its end stop stalls it. Find the real range on the bench and narrow `SERVO_MIN_US` / `SERVO_MAX_US` and the UI mapping together.

## Current rig

- ESP32 dev board (ELEGOO ESP-32, 30-pin, USB-C, CP2102), jumper wires (ELEGOO Dupont set).
- Servo: Miuzei MG996R metal gear. Two purchased; the first is dead, the spare is the one to use. 4.8-7.2 V, about 15 kg-cm stall at 6 V, up to ~2.5 A stalled.
- Servo is screwed to a wooden block on the shelf above the unit. The servo arm is duct-taped to the knob and currently removed from the servo.
- ESP32 and wiring sit on top of the PTAC discharge grille (warm air in heat mode).

### Power (two plugs, both to wall outlets)

Owner-reported: one plug for the servo, one for the ESP32 board (which is which is not confirmed).

- **Black cable with a black wall adapter** plugged into a wall outlet near the floor on the left. This is the Arkare 5V 2A adapter (`B09W96X88K`, 5.5x2.5 mm barrel, center positive). Its thin black cable runs along the baseboard under the unit and up to the top.
- **Beige/cream cable** from a multi-outlet strip beside it, run along the baseboard and up behind the unit. At the top its cores fan out and go into a small white block, from which thin wires lead to the ESP32.

Not confirmed (answer these in the repo when known):
1. Which supply feeds the servo and which feeds the ESP32.
2. What the white block is and its printed ratings. If the beige cable is a mains cord and the white block is an AC-DC module, then mains-voltage conductors are spliced and taped on top of the heater unit. That must be replaced with a proper enclosed power adapter before further use.
3. Whether the servo supply ground is connected to an ESP32 GND pin. With two separate supplies this is likely missing; without it the servo signal has no reference and the servo can twitch.
4. Rating of the servo supply (a 5V 2A adapter is undersized for an MG996R, which can pull ~2.5 A stalled).

## Known problems

1. **Servo overheated and died.** The old firmware attached the servo at boot and never detached, so it held torque against the knob continuously. The new firmware only powers the servo for a move plus 1 s, caps continuous on-time at 6 s with a 6 s cooldown, and never moves at boot. The spare must be bench tested before use (`test-plan.md`).
2. **Power.** Plan: servo supply 5-6 V and 3 A or more, servo ground tied to ESP32 GND, 470-1000 uF capacitor across the servo supply, 10k pull-down on the signal line. Optionally switch servo power with a MOSFET or relay.
3. **Mounting.** Duct tape on the knob melts and loosens. Plan: 3D-printed (PETG/ASA, not PLA) cap that fits over the knob, plus a servo bracket fixed to the panel or unit, coaxial with the knob or offset with a printed gear pair. Printing is done by a family member. Needs measurements first: knob diameter and height, clearance above the panel, mounting surface. Owner chose to keep the MG996R (no lower-torque servo).
4. **Electronics placement.** Move the ESP32 and wiring off the discharge grille into a small box out of the airflow, before the soak test.
5. **Public MQTT login.** The web page login is public by design. Its HiveMQ permissions must be restricted (see README, "Broker setup"); this has not been verified.
6. **Credentials.** The old firmware sketch contained live credentials. Rotate anything that was ever pushed or published (see README, "Broker setup").

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
- The ESP32 board sits on top of the unit at the grille edge, with a bundle of jumper wires running left along the top to the small white block (see Power above).
- The black Arkare adapter is plugged into the wall near the floor; the beige cable goes to a multi-outlet strip next to it. Both run along the baseboard and up to the top of the unit.
- At the time of the photos the servo arm was detached from the servo but still taped to the knob.
