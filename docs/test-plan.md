# Bench test plan (before the servo touches the knob)

Do these in order. Stop at the first failure. No multimeter needed.

## Gate 0: check the existing rig
- [ ] Photograph and write down the printed ratings of both power supplies (volts, amps, polarity, connector). Record them in `hardware.md`.
- [ ] Identify the white block and the beige cable. If they are mains-voltage conductors spliced and taped, replace with a proper enclosed adapter first.
- [ ] Unplug the dead servo. Check the ESP32 and the adapter are not hot, discolored or smelling burnt.
- [ ] Confirm the ESP32 still uploads a sketch.

## Gate 1: servo alone, off the PTAC
- [ ] Horn removed from the knob, servo free on the bench. Power it from the supply that will feed it in service, with its ground tied to an ESP32 GND pin.
- [ ] Flash the new firmware (do not erase flash, to keep saved presets).
- [ ] Send `cmd/us` values from the web page in Manual mode. Servo moves, goes silent about a second later, and feels cool at rest.
- [ ] Servo does not twitch at boot or when the ESP32 resets.

## Gate 2: find the real range
- [ ] Step the servo in small increments toward each end. Note where it starts buzzing or stops moving (end stop).
- [ ] Set `SERVO_MIN_US` / `SERVO_MAX_US` and the UI mapping a little inside those limits. Record the values here.

## Gate 3: on the knob
- [ ] Mount the horn (coupler) on the knob. Calibrate each preset in small steps with Manual on.
- [ ] Confirm OFF, HEAT3, HEAT2, COOL3, COOL2 land on the marks, and from the phone too.

## Gate 4: soak and hold
- [ ] Cycle through all presets for 30 minutes. After, the servo, supply and wiring are only warm at most.
- [ ] 24-hour hold test: set a preset, note the knob position, check again after 24 hours with the compressor/fan running. If it drifted, add a detent or a periodic re-assert in firmware.
- [ ] Confirm the "Robot" indicator on the page goes offline within about a minute of unplugging the ESP32, and back online when plugged in.
