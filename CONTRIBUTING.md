# Contributing

## Local component integration

Do not copy component files into the WSL ESPHome installation. Point the test configuration directly at this repository:

```yaml
external_components:
  - source:
      type: local
      path: /path/to/ha_deck2/components
    components: [ha_deck]
```

Repository example files use `../components`, which resolves correctly when run from their own directory context.

## Toolchain

Use the `esphome` executable from the active virtual environment.

Keep PlatformIO and ESPHome caches on the Linux filesystem rather than the mounted Windows drive:

```bash
cd /path/to/ha_deck2
source "$HOME/esphome-venv/bin/activate"
export PLATFORMIO_CORE_DIR=/tmp/ha_deck2-platformio
export ESPHOME_CACHE_DIR=/tmp/ha_deck2-cache
esphome compile tests/validate.yaml
```

Large-layout regression:

```bash
esphome compile tests/validate_720.yaml
```

This fixture starts on the climate screen at 720x720 and uses larger theme metrics and weather assets so responsive composite layout paths are exercised.

The YAML files also set `esphome.build_path` below `/tmp/esphome`.

## Run the visual dashboard

```bash
cd /path/to/ha_deck2
source "$HOME/esphome-venv/bin/activate"
export PLATFORMIO_CORE_DIR=/tmp/ha_deck2-platformio
export ESPHOME_CACHE_DIR=/tmp/ha_deck2-cache
set -a
source .env
set +a
esphome run examples/dashboard.yaml
```

The SDL window is 480x480 and uses the SDL touchscreen as LVGL input. ESPHome's `lvgl.widgets` list is intentionally empty: HA Deck creates the custom LVGL objects on its logical screens.

`examples/dashboard.yaml` subscribes to real Home Assistant entities configured through top-level substitutions. It exposes the encrypted ESPHome native API for local testing. Copy `.env.example` to `.env`, replace the example API key, and load it into the environment before running the dashboard. Adjust entity substitutions and network/API settings for the current environment.

Use `examples/buttons.yaml` for a compact visual matrix of button states and accent colors. Its SDL canvas is 680x480.

## Validation targets

Fast schema tests:

```bash
python -m unittest tests.test_schema
```

Native/SDL regression:

```bash
esphome compile tests/validate.yaml
```

Embedded compile check:

```bash
esphome compile tests/validate_esp32.yaml
```

The ESP32/ESP-IDF clean build can take substantially longer than native SDL. Allow a generous timeout. For UI-only changes, native compile plus interactive SDL inspection is the fast loop, but changes touching LVGL APIs, image formats, or embedded compilation should also reach a successful ESP32 build.

## Cache and interrupted-build recovery

ESPHome may reuse generated external-component sources after C++ header changes. If a build behaves as though it has stale code, remove only the exact project build directory under `/tmp/esphome/<node-name>` and rebuild. First resolve and print the absolute path; never delete `/tmp/esphome` or another broad directory blindly.

An interrupted ESP-IDF dependency step can leave a managed component directory half-created and cause `FileExistsError` on the next attempt. Use the same precise project build-directory cleanup; do not delete a shared cache root broadly.

## Verification by change type

- Schema-only change: run native compile and add the option to `tests/validate.yaml`.
- Visual/style change: compile and inspect the relevant SDL example at runtime.
- Screen lifecycle/navigation change: test default to transient, transient to transient, back navigation, and animated plus non-animated transitions.
- Templatable state change: test entity lag, unavailable/NaN state, screen re-entry, and forced first update.
- New icon/font usage: ensure glyphs are included in every relevant ESPHome font declaration; missing glyphs can look like layout bugs.
- New LVGL widget type: register its LVGL feature in `_register_lvgl_resources()` and verify both native and ESP32 compilation.
