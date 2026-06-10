# pebbleface-perlin

Perlin is a Pebble watchface with randomly rotating perlin-noise backgrounds.

## Supported platforms

- Pebble Time / Time Steel (basalt)
- Pebble Time Round (chalk)
- Pebble Time 2 (emery) — new in v1.3.0

## Features

- Time and date over rotating perlin-noise backgrounds (changes hourly)
- Optional date display
- Optional battery percentage display
- Bluetooth disconnect vibration (optional)
- Hourly vibration (optional)
- Steps progress bar (new in v1.3.0): a horizontal bar between the time and the
  date that grows outward from the center as you approach your daily step goal.
  Step counts are polled from the Health API once a minute in the tick handler
  (no HealthService event subscription).

## Configuration

Settings are configured in-app via [Clay](https://github.com/pebble/clay) —
no external configuration page is needed:

- Show date (default on)
- Show battery percentage (default on)
- Show steps progress bar (default on)
- Daily step goal the bar is scaled against (default 10000)
- Bluetooth disconnect vibration (default on)
- Hourly vibration (default off)
- Shuffle background when saving settings
- Background refresh interval: every 1, 5, 15, 30 or 60 hours (default 1,
  new in v1.4.0)

## Backgrounds

The face rotates through 25 backgrounds. BG1–BG20 are the original hand-made
themes. **BG21–BG25 are new in v1.3.0**: procedurally generated perlin-noise
artwork (rainbow arrows, teal boxes, warm flow strokes, contour bands and
pastel stitches), rendered natively for every platform size by
[`tools/generate_backgrounds.py`](tools/generate_backgrounds.py). The script
is seeded, so the shipped artwork can be regenerated exactly.

## v1.4.0 changes

- Configurable background refresh interval (1/5/15/30/60 hours) from the
  settings page; previously the background always changed hourly.
- Larger date text: the date now uses a dedicated 20pt LECO font (battery
  percentage stays at 14pt), with layout frames adjusted on every platform.

## v1.3.0 changes

- Added five new procedurally generated perlin-noise backgrounds (BG21–BG25)
  with a reproducible generator script in `tools/`.
- Added Pebble Time 2 (emery) support with larger time/date layout frames for
  the 200x228 screen, plus 200x228 upscaled background artwork (`~emery`
  resource variants).
- Added the configurable steps progress bar. Steps are polled each minute via
  `health_service_sum_today` (guarded by `PBL_HEALTH`, zero-step fallback on
  watches without health support).
- Replaced the externally hosted configuration page with Clay; settings are
  now sent as plain AppMessages (AppSync removed) and persisted on the watch.
- Project restructured to the modern SDK layout: `src/c/` + `src/pkjs/`,
  `enableMultiJS`, updated wscript.

## Building

Install the JS dependencies and build with the Pebble SDK:

```
pebble build
pebble install --emulator emery
```
