# Second Movement Customizations

This repository customizes Movement for a consistent three-button interface
and a small set of purpose-built watch faces. This file describes implemented
behavior; it is not a roadmap or design notebook.

## Navigation

Faces are divided into three top-level menus:

```text
CLOCK -> TOOLS -> CNFG -> CLOCK
```

- Short Mode advances within the current menu.
- Long Mode advances to the next menu.
- A transient title face identifies the newly selected menu, then disappears
  from rotation.
- The Config `TITLE` setting controls the title duration.
- Memory is nested under Config through a launcher rather than occupying a
  top-level menu.

The general control convention is:

- Mode navigates.
- Light illuminates, except where an editor requires a third input.
- Short Alarm performs the face's primary action.
- Long Alarm performs its related alternate or reset action.

## Clock and solar display

### Clock

- Short Alarm toggles the hourly signal.
- Long Alarm toggles and persists the global 12/24-hour preference.

### `my_sunrise_face`

- Initially shows whichever next event—sunrise or sunset—occurs first.
- Short Alarm toggles sunrise/sunset.
- Long Alarm toggles the shared 12/24-hour preference.
- Labels use `RISE` and `SET`; the bottom row shows time and day.
- Coordinates are edited by a separate Config face using the original
  Sunrise/Sunset coordinate editor.
- Missing coordinates default to Montreal (`45.50 N`, `73.57 W`).
- Timezone remains an independent setting; Montreal/Toronto use the New York /
  Eastern timezone entry.

Date, time, timezone, and location are separate Config faces.

## Timers

### `my_countdown_face`

- Presets are 1, 3, 5, 10, 15, 20, and 30 minutes.
- Short Alarm starts or pauses; long Alarm resets.
- While stopped, the first accelerometer tap rounds the remaining time upward
  to the nearest preset. Later taps cycle presets.
- A running countdown continues through its background alarm.
- Light remains ordinary illumination.

### `my_stopwatch_face`

- Uses 128 Hz timing and displays hundredths and an hours counter.
- Short Alarm starts or stops; long Alarm resets.
- Lap mode and Light-controlled refresh behavior were removed.

## Memory

`my_databank_face` displays build-time name/value entries:

- The name remains on top while the value scrolls below.
- Short Alarm selects the next entry; long Alarm selects the previous entry.
- Short Mode returns to the Memory launcher; long Mode returns to Clock.
- Personal entries live in the ignored
  `private/my_databank_entries.inc` file and are compiled into local firmware.
- `private/my_databank_entries.example.inc` documents the tracked format.

## Motion and rest report

`time_since_motion_face` samples the LIS2DW Still/Active signal once per minute
and stores a volatile 1,440-bit rolling history (24 hours). History resets on
reboot.

- `INACT` displays time since sampled activity.
- Short Alarm cycles the cached report:
  `INACT -> REST -> START -> END -> STILL -> WAKE -> INACT`.
- Long Alarm returns to `INACT`.
- `REST` is the strongest detected rest candidate.
- `START` and `END` show its local boundaries.
- `STILL` is the raw inactive-sample percentage inside it.
- `WAKE` counts active runs of at least five minutes that remain inside the
  selected candidate.

The analyzer uses an 80%-still centered 20-minute neighborhood, joins analyzed
gaps up to 10 minutes, rejects candidates shorter than 60 minutes, and scores
remaining candidates using duration plus raw still minutes. These are motion
estimates, not medical sleep measurements.

## Settings and persistence

- Menu title duration is stored separately in `title_length.u8`.
- Accelerometer motion threshold is stored in `motion_threshold.u8`.
- `THRSH` offers 0.125, 0.25, 0.5, 0.75, 1.0, and 1.5 g; default is 1.0 g.
- The original 32-bit settings word remains full.

## Low-energy behavior

The experimental display-only sleep-Light state machine was removed after
unreliable hardware behavior.

- Light wakes the watch and uses the normal illumination path.
- Alarm wakes the watch without forwarding an Alarm face event or sound.
- Normal inactivity timing returns the watch to low-energy mode.
- `MOVEMENT_DEBUG_MODE` can enable the 60-second development timeout override;
  it is disabled for ordinary use.

## Build

Create the ignored Databank configuration before the first build:

```sh
cp private/my_databank_entries.example.inc private/my_databank_entries.inc
```

Build Sensor Watch Pro with the custom LCD:

```sh
make clean
make BOARD=sensorwatch_pro DISPLAY=custom
```

To clean, build, number, and copy firmware to the mounted watch volume:

```sh
./scripts/export_firmware_to_usb.sh
```

The repository also contains the proposed Chirpy motion-export wire contract in
`docs/CHIRPY_MOTION_PROTOCOL.md`; its receiver application is maintained in the
separate sibling `sensor-watch-motion-rx` repository.
