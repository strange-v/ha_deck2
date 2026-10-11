# HA Deck configuration reference

This document covers the root `ha_deck` component, themes, screens, widgets, and actions. The executable source of truth is the validation schema in `components/ha_deck/__init__.py`. An option marked **templatable** accepts either a fixed value or an ESPHome lambda.

## Root component

```yaml
ha_deck:
  id: deck
  lvgl_id: main_lvgl
  default_theme: material_dark
  default_screen: scr_main
  screen_timeout: 30s
  themes:
    - id: material_dark
  screens:
    - id: scr_main
      widgets: []
```

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | No | Generated | ESPHome ID for the component. |
| `lvgl_id` | Yes | — | Existing ESPHome LVGL component rendered by HA Deck. |
| `default_theme` | Yes | — | Theme activated during setup. |
| `default_screen` | Yes | — | Screen shown during setup and retained during navigation. |
| `update_interval` | No | `100ms` | Reevaluation interval for the active screen. |
| `screen_timeout` | No | `30s` | Inactivity delay before a non-persistent screen returns to `default_screen`. Touch input resets the timer. A widget can require a longer delay for its screen, for example `camera.timeout`. |
| `weather_icon_size` | No | `fonts.icon_medium` size | Shared compiled weather-image size, from 8 to 128 px. |
| `weather_icon_directory` | No | Bundled Meteocons | Complete replacement directory for the 16 weather SVGs. Relative paths resolve from the main ESPHome YAML. |
| `themes` | Yes | — | Non-empty list of theme definitions. |
| `screens` | Yes | — | Non-empty list of logical screens. |

## Themes

A theme is semantic: widgets consume its color roles, font tokens, metrics, and named accents instead of defining a separate global palette.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | Practically | Generated | Define explicitly when referenced by `default_theme` or an action. |
| `base` | No | `dark` | Selects the built-in `dark` or `light` palette. The ID does not select a palette. |
| `background_color` | No | Base palette | Default screen background. |
| `surface_color` | No | Base palette | Main surface role. |
| `surface_container_color` | No | Base palette | Elevated/container surface role. |
| `primary_color`, `on_primary_color` | No | Base palette | Primary color and content shown on it. |
| `primary_container_color`, `on_primary_container_color` | No | Base palette | Primary container roles. |
| `error_color`, `on_error_color` | No | Base palette | Error roles. |
| `on_surface_color` | No | Base palette | Default text/icon color. |
| `outline_color` | No | Base palette | Default outline role. |
| `body_font` | No | — | Legacy general fallback. Prefer tokenized `fonts`. |
| `fonts` | No | Empty | Font-token mapping described below. |
| `metrics` | No | Built in | Layout-token mapping described below. |
| `radius` | No | `5` | Default control radius in pixels. |
| `disabled_opacity` | No | About `38%` | Default opacity for disabled controls. |
| `accents` | No | Built in | Custom `{name, color, on_color}` entries. Same-name entries replace built-ins. |

Font tokens are `text_small`, `text_medium`, `text_large`, `icon_small`, and `icon_medium`. Text sizes fall back toward `body_font`; icon sizes fall back to one another. HA Deck intentionally provides no icon font, so every glyph must resolve through a theme token or a widget-local font.

| Metric | Default |
| --- | --- |
| `spacing_small` | `8` |
| `spacing_medium` | `16` |
| `touch_target` | `56` |
| `control_height` | `88` |

Built-in accents are `light`, `climate`, `heat`, and `dry`. The semantic `neutral` accent resolves dynamically to `on_surface_color` on `background_color`, so it follows light/dark bases and color overrides.

## Screens

Screens are mounted lazily. The default screen remains mounted while transient screens are removed when left.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | Practically | Generated | Define explicitly when referenced by navigation or `default_screen`. |
| `widgets` | Yes | — | Widget list; it may be empty. |
| `background_color` | No | Active theme background | Per-screen background override. |
| `background_image` | No | — | ESPHome image drawn over the background color. |
| `persistent` | No | `false` | Prevents automatic return to `default_screen`. The default screen is always persistent. |

## Widgets

Widgets are added under a screen's `widgets` list. Except where noted, positions and sizes use display pixels.

### Shared options

Most widgets use these layout options:

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | No | Generated | ESPHome ID for the widget. |
| `x`, `y` | Yes | — | Position on the screen. |
| `width`, `height` | Yes | — | Widget size. Values must be positive. |
| `visible` | No | `true` | **Templatable.** Hides the widget and disables input. |

An icon can use an ESPHome image or a font glyph:

```yaml
icon: image_id

icon:
  glyph: "\U000F0335"
  font: material_icons_48
```

`font` is optional for a glyph when the active theme provides the required icon font.

### Button

A `button` needs `text`, `icon`, or both. If both are present, the icon is shown above the text.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `text` | No | — | Button label. |
| `icon` | No | — | ESPHome image, or an `image`/`glyph` icon object. |
| `font` | No | Theme font | Font for `text`. |
| `variant` | No | `glass` | `glass`, `filled`, or `icon`. |
| `accent` | No | Theme primary | **Templatable.** Named theme accent. |
| `accent_icon` | No | `false` | **Templatable.** Uses the accent color for a glass button icon. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `disabled` | No | `false` | **Templatable.** Disables input. |
| `toggle` | No | `false` | Makes the button checkable. |
| `checked` | No | Local state | **Templatable.** External checked state. |
| `background_color` | No | Theme value | Local background override. |
| `text_color` | No | Theme value | Local foreground override. |
| `border_color` | No | Theme value | Local border override. |
| `border_width` | No | Theme value | Local border width in pixels. |
| `radius` | No | Theme value | Local corner radius. |
| `disabled_opacity` | No | Theme value | Opacity percentage while disabled. |
| `on_click` | No | — | Runs after a short click. |
| `on_press` | No | — | Runs when pressed. |
| `on_release` | No | — | Runs when released. |
| `on_long_press` | No | — | Runs after LVGL's configured long-press time. |
| `on_turn_on` | No | — | Runs when a toggle turns on. Requires `toggle: true`. |
| `on_turn_off` | No | — | Runs when a toggle turns off. Requires `toggle: true`. |

After a long press, click and toggle actions are skipped for that gesture.

### Sensor value

`sensor_value` displays a numeric value with optional text, units, and an icon. A `NaN` value is shown as `unavailable_text`.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `value` | Yes | — | **Templatable** numeric value. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `format` | No | `%.1f` | Float format with exactly one conversion. Use `%%` for `%`. |
| `units` | No | Empty | Text shown next to the value. |
| `unavailable_text` | No | `−` | Text shown for `NaN`. |
| `top_text` | No | Empty | Supporting text above the value. |
| `bottom_text` | No | Empty | Supporting text below the value. |
| `icon` | No | — | ESPHome image or glyph icon. |
| `value_font` | No | `text_medium` | Local value font. |
| `text_font` | No | `text_small` | Local font for units and supporting text. |
| `accent` | No | `neutral` | Named theme accent. The default follows the theme's normal foreground (`on_surface_color`). |

### Weather

`weather` uses the sensor-value layout and selects an icon from a Home Assistant weather condition. It uses the bundled Meteocons unless the root component sets `weather_icon_directory`.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `temperature` | Yes | — | **Templatable** numeric temperature. |
| `condition` | Yes | — | **Templatable** Home Assistant condition string. |
| `is_night` | No | `false` | **Templatable.** Selects night variants for clear and partly cloudy weather. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `format` | No | `%.1f` | Temperature float format. |
| `units` | No | Empty | Temperature units. |
| `unavailable_text` | No | `−` | Text shown for `NaN`. |
| `top_text`, `bottom_text` | No | Empty | Supporting text. |
| `value_font`, `text_font` | No | Theme fonts | Local font overrides. |
| `accent` | No | `neutral` | Named theme accent. The default follows the theme's normal foreground (`on_surface_color`). |

Weather images use `fonts.icon_medium` for their size. If themes use different icon sizes, set root option `weather_icon_size` to a value from 8 to 128. To use your own set, point `weather_icon_directory` at a directory containing all files listed in `components/ha_deck/assets/weather/README.md`. Missing files are reported during configuration validation.

### Slider

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `value` | No | `0` | **Templatable** current value. |
| `min_value`, `max_value` | No | `0`, `100` | Range; minimum must be lower than maximum. |
| `orientation` | No | `vertical` | `vertical` or `horizontal`. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `disabled` | No | `false` | **Templatable.** Disables input. |
| `format` | No | `%.0f%%` | Float format shown while dragging. |
| `label` | No | Empty | Text in the action area. Cannot be combined with `icon`. |
| `font` | No | `text_small` | Local label font. |
| `icon` | No | — | Glyph and optional font for the action area. |
| `action_height` | No | `56` | Action-area size along the main axis. |
| `accent` | No | Theme primary | Named theme accent. |
| `optimistic` | No | `true` | Keeps the selected value while external state catches up. |
| `optimistic_timeout` | No | `2s` | Maximum wait for external state confirmation. |
| `on_value` | No | — | Runs while the value changes; value is available as `x`. |
| `on_release` | No | — | Runs after dragging; value is available as `x`. |
| `on_click` | No | — | Runs when the action area is clicked. Requires a label or icon. |

### Climate

`climate` combines a temperature arc, target controls, mode buttons, power, and an optional menu for secondary controls.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `current_temperature` | Yes | — | **Templatable** current temperature. |
| `target_temperature` | Yes | — | **Templatable** target temperature. |
| `primary_controls` | Yes | — | One to three climate option objects. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `disabled` | No | `false` | **Templatable.** Disables input. |
| `min_value`, `max_value` | No | `5`, `35` | Target temperature range. |
| `step` | No | `0.5` | Target adjustment step; must be positive. |
| `format` | No | `%.1f` | Temperature float format. |
| `units` | No | `°C` | Temperature units. |
| `accent` | No | `climate` | Cooling accent. |
| `heating_accent` | No | `heat` | Heating accent. |
| `drying_accent` | No | `dry` | Drying accent. |
| `arc_mode` | No | `cooling` | **Templatable:** `cooling`, `heating`, or `drying`. |
| `optimistic_timeout` | No | `2s` | Holds a new target while external state catches up. |
| `padding` | No | Theme metric | Inner padding. |
| `gap` | No | Theme metric | Space between controls. |
| `controls_height` | No | Theme metric | Height of the bottom controls. |
| `power` | No | Defaults below | Power-control object. |
| `additional_controls` | No | Empty | Groups shown in the menu overlay. |
| `menu_icon` | No | Menu glyph | Glyph for the menu button. |
| `close_icon` | No | Close glyph | Glyph for the overlay close button. |
| `on_target_change` | No | — | Runs after a target change; value is available as `x`. |

#### Climate options

Items in `primary_controls` and each additional group's `options` support:

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | No | Generated | ESPHome ID. |
| `label` | No | Empty | Option label. |
| `icon` | No | — | ESPHome image or glyph icon. |
| `active` | No | `false` | **Templatable.** Active state. |
| `accent` | No | Parent accent | Named theme accent. |
| `on_select` | No | — | Runs when selected. |

Each item in `additional_controls` requires a `label` and a non-empty `options` list.

The `power` object supports:

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `visible` | No | `true` | **Templatable.** Power-button visibility. |
| `state` | No | `false` | **Templatable.** Current power state. |
| `icon` | No | Power glyph | Font glyph. |
| `accent` | No | Current arc accent | Named theme accent. |
| `on_turn_on`, `on_turn_off` | No | — | Power automations. |

### Navigation button

`navigation_button` uses either aligned or absolute placement. It must use exactly one navigation mode: `back: true` or `target`.

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | No | Generated | ESPHome ID. |
| `x`, `y` | No | — | Absolute position. Both must be set together. |
| `align` | No | `top_left` | `top_left`, `top_right`, `bottom_left`, `bottom_right`, or `center`. Cannot be combined with `x`/`y`. |
| `width`, `height` | No | Theme touch target | Button size. |
| `margin` | No | Theme small spacing | Margin used with alignment. |
| `visible` | No | `true` | **Templatable.** Widget visibility. |
| `back` | Sometimes | `false` | Uses screen history, then the default screen. |
| `target` | Sometimes | — | Opens a fixed screen. |
| `glyph` | No | Back glyph | Font glyph shown by the button. |

### Camera

`camera` shows a live MJPEG stream (`multipart/x-mixed-replace` over plain HTTP). It is intended for RTSP/H.264/H.265 cameras behind a server that transcodes to MJPEG on demand, such as [go2rtc](https://github.com/AlexxIT/go2rtc). The stream is opened only while the widget's screen is active and the widget is visible, so the server stops transcoding when nobody is watching.

```yaml
- camera:
    x: 0
    y: 0
    width: 480
    height: 480
    url: http://192.168.1.10:1984/api/stream.mjpeg?src=entrance_480
    on_click:
      - lambda: id(deck).go_back();
```

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `x`, `y`, `width`, `height` | Yes | — | Position and size. |
| `url` | Yes | — | `http://` URL of the MJPEG stream. HTTPS and credentials are not supported. |
| `timeout` | No | `5min` | Inactivity time after which the stream is paused. While the widget is on a screen, it also raises that screen's `screen_timeout` to at least this value. |
| `visible` | No | `true` | **Templatable.** Hiding the widget closes the stream. |
| `connecting_text` | No | `Connecting` | Shown until the first frame arrives. |
| `error_text` | No | `No signal` | Shown while reconnecting; the last frame stays dimmed behind it. |
| `paused_text` | No | `Paused` | Shown after `timeout`. Tapping the widget resumes the stream. |
| `on_click` | No | — | Runs when the live view is tapped. |

Status texts use the theme's `text_small` font, so their glyphs must be included in that font.

Frames are centered: smaller frames are letterboxed in black, larger frames are cropped. For the best frame rate, let the server produce exactly the widget size, so the device only decodes and copies. The widget keeps reading the stream while it decodes and always decodes the newest frame, so the picture does not fall behind when the server sends more frames than the device can show; the extra frames are dropped.

The widget needs about three RGB565 frame buffers (`width × height × 2` bytes each) plus three 128 KiB JPEG buffers, so PSRAM is required for full-screen use on ESP32. It is supported on ESP32 and the host (SDL) platform.

Performance notes:

- On an ESP32-S3 with a 480x480 RGB panel, decoding a full-screen frame takes about 50 ms, which shows about 10 fps from a 12 fps camera. The decoder time depends mostly on the frame size in pixels, not on JPEG quality.
- Set `power_save_mode: none` under `wifi`. With Wi-Fi power saving, frames arrive in bursts and more of them are dropped.
- Frame statistics (decoded, received, and dropped frames per second and decode time) are logged every 10 seconds at `VERBOSE` level under the `ha_deck.camera` tag.
- On ESP32 the widget uses the [jpegdec-esphome](https://github.com/strange-v/jpegdec-esphome) ESP-IDF component, a build of JPEGDEC with ESP32-S3 SIMD enabled. Another component that links its own copy of JPEGDEC, such as ESPHome's `online_image` with JPEG support, may conflict with it.

An example go2rtc configuration that uses Intel VA-API for decoding and scaling:

```yaml
api:
  listen: ":1984"

ffmpeg:
  rtsp_vaapi: >-
    -hwaccel vaapi -hwaccel_device /dev/dri/renderD128 -hwaccel_output_format vaapi
    -fflags nobuffer+discardcorrupt -flags low_delay -rtsp_transport tcp -i {input}
  # Fill 480x480 by cropping the center. Use crop=480:480:0:0 for the left edge.
  mjpeg_480_cover: >-
    -vf "scale_vaapi=w=-2:h=480,hwdownload,format=nv12,crop=480:480,format=yuvj420p,setpts=N/FRAME_RATE/TB"
    -fps_mode passthrough -c:v mjpeg -q:v 5 -huffman 0 -force_duplicated_matrix 1
  # Fit the width and add black bars. RTP/JPEG needs sizes divisible by 8.
  mjpeg_480_contain: >-
    -vf "scale_vaapi=w=480:h=272,hwdownload,format=nv12,pad=480:480:0:(oh-ih)/2:black,format=yuvj420p,setpts=N/FRAME_RATE/TB"
    -fps_mode passthrough -c:v mjpeg -q:v 5 -huffman 0 -force_duplicated_matrix 1

streams:
  entrance_480: ffmpeg:rtsp://127.0.0.1:8554/entrance#input=rtsp_vaapi#video=mjpeg_480_cover
```

`-huffman 0 -force_duplicated_matrix 1` are required because go2rtc carries ffmpeg's output over RTP/JPEG, which only accepts standard Huffman tables.

## Actions

`ha_deck.screen.show` changes the logical screen:

| Option | Required | Default | Description |
| --- | --- | --- | --- |
| `id` | Yes | — | Target screen ID. |
| `animation` | No | `none` | LVGL screen-load animation. |
| `time` | No | `200ms` | Animation duration. |

`ha_deck.theme.set` requires the target theme `id` and reapplies it to the active screen immediately.

```yaml
- ha_deck.screen.show:
    id: another_screen
    animation: move_left
    time: 220ms

- ha_deck.theme.set: another_theme
```

Screen animations are `none`, `over_left`, `over_right`, `over_top`, `over_bottom`, `move_left`, `move_right`, `move_top`, `move_bottom`, `fade_in`, `fade_out`, `out_left`, `out_right`, `out_top`, and `out_bottom`.

## Home Assistant state helpers

HA Deck provides helpers for safely using Home Assistant `text_sensor` states in lambdas. A state is considered
available after it has been received and when it is neither empty, `unknown`, nor `unavailable`.

```yaml
visible: !lambda |-
  return ha_is_available(id(ha_alarm_state));
```

Use `ha_state_in` and `ha_state_not_in` to compare against any number of states. Both return `false` when the entity
is unavailable, preventing an unavailable entity from accidentally passing a negative comparison.

```yaml
visible: !lambda |-
  return ha_state_not_in(
    id(ha_blinds_state),
    "opening",
    "closing"
  );
```
