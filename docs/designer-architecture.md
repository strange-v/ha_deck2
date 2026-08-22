# Layout Editor Architecture

## Purpose and ownership boundary

The Layout Editor is a companion application, not part of the ESPHome runtime. ESPHome YAML
remains the source of truth. The editor does not generate widgets, evaluate lambdas, resolve Home
Assistant state, or replace ESPHome validation. It reads enough static configuration to reproduce
screen geometry and approximate HA Deck styling, and writes only literal widget geometry values.

The application is designed to run beside an ESPHome configuration directory. In Docker, that
directory is mounted at `/config`. The backend deliberately restricts configuration and asset access
to this configured root.

## Application structure

The Designer is a small two-part web application:

- `designer/backend`: Python 3.12, FastAPI, Pydantic, and ruamel.yaml.
- `designer/frontend`: React, TypeScript, Vite, and Zustand.

The production Docker image builds the frontend first and copies its static output into the Python
image. FastAPI then serves both `/api/*` and the single-page application on port `8099`. During local
development, Vite runs separately and proxies `/api` to FastAPI.

## Read path

The backend discovers YAML files below `HA_DECK_CONFIG_ROOT`. Loading a layout performs these steps:

1. Parse YAML in round-trip mode so tags, comments, quoting, and source locations remain available.
2. Resolve the physical viewport through `ha_deck.lvgl_id -> lvgl.displays -> display.dimensions`.
3. Resolve font IDs, theme font tokens, effective light/dark colors, accents, and screen backgrounds.
4. Project screens and widgets into a small JSON model containing geometry and preview metadata.
5. Record exact source spans for literal `x`, `y`, `width`, and `height` scalars.

Display dimensions are mandatory. If they are missing, dynamic, or ambiguous, the editor refuses to
open the layout instead of guessing. ESPHome `packages` and `!include` expansion are intentionally
out of scope for the initial implementation.

## Preview model

The frontend draws widgets at native display-pixel coordinates. Preview components reproduce the
layout primitives that matter for composition: bounds, theme colors, configured fonts, icon slots,
button variants, value/unit baselines, sliders, and climate controls. Dynamic values use stable dummy
data because ESPHome lambdas and Home Assistant entities are not evaluated by the editor.

The preview is approximate by design. The C++/LVGL implementation remains authoritative for exact
rendering and runtime state. When widget styling changes, update `WidgetPreview.tsx` alongside the
component and reuse semantic theme rules rather than adding unrelated CSS colors.

## Editing and save path

Zustand stores the loaded document, current geometry, selection, and undo/redo snapshots. Movement,
multi-selection, keyboard nudging, and Inspector edits update only browser state until Save is used.
Numeric Inspector fields accept simple arithmetic, but only their evaluated integer result enters the
state.

Save sends changed geometry together with the SHA-256 revision of the loaded source. The backend:

1. rejects the request if the file revision changed;
2. verifies the screen, widget ID, and editable scalar spans;
3. replaces only the affected integer substrings;
4. reparses the result before writing;
5. performs an atomic file replacement and returns a unified diff.

This span-based patching is intentionally narrower than serializing YAML. It preserves lambdas,
custom tags, comments, ordering, and formatting that the editor does not own. Duplicate widget IDs
and dynamic geometry are read-only because they cannot be patched unambiguously.

## Security and deployment

The editor has no built-in authentication and can modify files in its mounted configuration root. Do
not expose it directly to the public internet. Place it behind an authenticated reverse proxy when it
is reachable outside a trusted network, and mount only the intended ESPHome directory.

GitHub Releases build `designer/Dockerfile` for `linux/amd64` and `linux/arm64` and publish semantic
version, stable, beta, and RC Docker Hub tags. Repository credentials are consumed only through
GitHub Actions secrets and are never embedded in the image.

## Important files

- `designer/backend/src/ha_deck_designer/yaml_projection.py`: YAML-to-layout projection and source spans.
- `designer/backend/src/ha_deck_designer/yaml_patcher.py`: revision-safe geometry patching.
- `designer/backend/src/ha_deck_designer/main.py`: discovery, layout, asset, save, and static-file APIs.
- `designer/frontend/src/store.ts`: editor state, selection, history, and pending changes.
- `designer/frontend/src/Canvas.tsx`: native-pixel canvas and pointer interactions.
- `designer/frontend/src/WidgetPreview.tsx`: approximate widget rendering.
- `designer/Dockerfile`: production frontend/backend image.
- `.github/workflows/release-designer-docker.yml`: multi-architecture image publication.

Local setup, tests, Docker Compose, and keyboard controls are documented in
[`designer/README.md`](../designer/README.md).
