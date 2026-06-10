# pebbleface-perlin

Perlin is a Pebble watchface with randomly rotating perlin-noise backgrounds.

## Supported platforms

- Pebble Time / Time Steel (basalt)
- Pebble Time Round (chalk)
- Pebble Time 2 (emery) — new in v1.3.0

## Features

- Time and date over rotating perlin-noise backgrounds (changes hourly, or can be fixed)
- Optional date display
- Optional battery percentage display
- Bluetooth disconnect vibration (optional)
- Hourly vibration (optional)
- Steps progress bar (new in v1.3.0): a horizontal bar between the time and the
  date that grows outward from the center as you approach your daily step goal.
  Uses the Pebble Health API. Configurable from the settings page:
  - `showsteps` — show/hide the bar
  - `maxsteps` — daily step goal the bar is scaled against (default 10000)

## v1.3.0 changes

- Added Pebble Time 2 (emery) support with larger time/date layout frames
  optimized for the bigger 200x228 screen. The existing LECO fonts are kept;
  only the layer frames were enlarged/repositioned.
- Added the configurable steps progress bar (Health API, with graceful
  fallback to zero steps on platforms without health support).
- Configuration page URL switched to HTTPS.

## Building

Build with the Pebble SDK:

```
pebble build
pebble install --emulator emery
```

Note: emery uses the basalt background artwork (centered on the larger
screen). Layout offsets may need minor tweaks after testing on a real
device or emulator.
