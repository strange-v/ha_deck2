# Architecture

## Ownership boundary

ESPHome initializes and owns the physical or SDL display, touchscreen, LVGL instance, fonts, images, API connection, and Home Assistant state objects. The external component receives an existing `lvgl_id` and renders into it. Do not create a parallel display or input stack inside HA Deck.

The Python integration in `components/ha_deck/__init__.py` validates YAML, declares generated objects, registers required LVGL features, packages weather images, and wires automations. Runtime behavior lives in `components/ha_deck/*.h` and `*.cpp`.

## Runtime object graph

`HaDeck` is a `PollingComponent` and owns references to themes and logical screens. Each `HaDeckScreen` owns a list of `HaDeckWidget` instances. A widget may create several LVGL objects, but exposes one root object for visibility and lifecycle management.

The common widget lifecycle is:

1. `mount(parent, theme)` creates LVGL objects.
2. `apply_theme(theme)` applies semantic colors, fonts, and metrics.
3. `update(force)` evaluates visibility and widget-specific templatable state.
4. `unmount()` clears pointers after LVGL deletes the screen object tree.

`HaDeckWidget::update()` handles the shared templatable `visible` property before calling the widget-specific update method. New widget types should follow this contract.

## Screen lifetime and navigation

Screens are logical definitions and are mounted lazily. The configured default screen is special: it stays mounted while a secondary screen is active. A secondary screen is deleted when navigating away from it. This keeps the common home screen responsive while preventing ten configured screens from retaining ten complete LVGL trees.

Only the active screen is updated by `HaDeck::update()`. The retained default screen does not poll templatable widget state in the background. Before a screen becomes visible, `switch_screen()` mounts it if needed, reapplies the current theme, and performs a forced update.

Navigation history is stored as logical screen pointers. `go_back()` pops the history and falls back to the default screen. Navigation is available through:

- `ha_deck.screen.show` in ESPHome automations;
- `id(deck).switch_screen("screen_name")` in a lambda;
- `navigation_button` with either `back: true` or `target: screen_id`.

Animated loads let LVGL delete the previous transient screen after the animation. Non-animated loads delete it immediately. When changing this code, test both paths because LVGL owns the actual object deletion while `unmount()` only clears HA Deck's pointers.

## Themes and styling

Themes are semantic rather than widget-specific. They contain Material-inspired light/dark color roles, reusable font tokens, layout metrics, radius, disabled opacity, and named accents.

Font tokens:

- `text_small`
- `text_medium`
- `text_large`
- `icon_small`
- `icon_medium`

Metric tokens:

- `spacing_small` (default 8 px)
- `spacing_medium` (default 16 px)
- `touch_target` (default 56 px)
- `control_height` (default 88 px)
- `radius` (default 5 px)

The built-in accents in `theme.h` are `light`, `climate`, `heat`, and `dry`. `neutral` is semantic and resolves to the theme's `on_surface` color on its background. A YAML accent with the same name replaces the built-in behavior. Unknown or empty accent names fall back to the theme primary color.

Shared glass button styling lives in `button_style.h` and is used by regular buttons and climate/navigation controls. The general button adds the `filled` variant and `accent_icon` behavior. Prefer extending shared styling when a state should look consistent across controls.

## Layout strategy

Top-level widgets currently use explicit `x`, `y`, `width`, and `height` in real display pixels. There is intentionally no reference-resolution scaling: a user with a 720x720 display configures 720x720 coordinates directly.

Inside composite widgets, prefer LVGL flex layout and geometry derived from the widget's assigned bounds plus theme metrics. The climate widget is the primary example: its top area grows, its bottom control row has a themed/configurable height, and the arc is centered in the available top square. Avoid constants that assume a 480x480 screen unless the value is part of a deliberate visual primitive, such as arc stroke width.

## Composite and internal components

`HaDeckClimate` composes labels, buttons, a `HaDeckClimateArc`, and a reusable `HaDeckOverlay`. The overlay owns only backdrop/panel lifetime, theme, stacking, and open/close behavior. Callers populate `content_obj()`. Keep it generic so future selectors and dialogs can reuse it.

Weather icons are bundled SVG assets under `components/ha_deck/assets/weather/`. The Python integration registers generated ESPHome image resources automatically; users bind condition text and do not list every weather image. Their shared compile-time resize is derived from the common `icon_medium` font size; `weather_icon_size` is an explicit override for cases where those dimensions should differ. `weather_icon_directory` can replace the entire bundled set with a private directory while preserving the generated image pipeline.

Icon glyphs intentionally have no built-in font. A user must provide a local font or the relevant `icon_small`/`icon_medium` token in every theme.

## Design constraints

- HA Deck remains one ESPHome external component; widgets are isolated C++ classes, not separately installed components.
- Coordinates are native display pixels. There is no hidden reference-resolution conversion.
- Named accents describe domain semantics. Widget-local color overrides remain escape hatches rather than a parallel styling system.
- Climate modes, presets, fan modes, and swing modes are configured explicitly; HA Deck does not discover Home Assistant capability lists at runtime.
- The reusable overlay is internal infrastructure, not a public top-level widget.
- Screen persistence is intentionally absent until a concrete restart/resume requirement justifies it.

## Important files

- `components/ha_deck/__init__.py`: schema, validation, code generation, actions, automatic LVGL/font/image resource registration.
- `ha_deck.{h,cpp}`: root controller, polling, themes, navigation, history.
- `screen.{h,cpp}` and `widget.h`: screen and widget lifecycles.
- `theme.h` and `button_style.h`: semantic styling and shared control states.
- `button`, `slider`, `value_display`, `weather`, `navigation_button`: widgets.
- `climate`, `climate_arc`, `overlay`: climate composition and internals.
