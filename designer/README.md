# HA Deck Layout Editor

The Designer is intentionally a layout-only editor. Existing ESPHome YAML remains the source of truth. The editor reads screens, widget geometry, themes, and font declarations, then writes only literal `x`, `y`, `width`, and `height` values.

See [Layout Editor Architecture](../docs/designer-architecture.md) for component boundaries, read/save flows, preview constraints, security, and important files.

## Current scope

- Resolves the viewport through `ha_deck.lvgl_id -> lvgl.displays -> display.dimensions`.
- Treats missing, dynamic, ambiguous, or unsupported display dimensions as a blocking error.
- Resolves theme font tokens and explicit widget fonts to their literal ESPHome `font.size` and `font.file` declarations.
- Displays all top-level widgets; unknown types receive a generic preview.
- Supports single selection, Ctrl/Cmd multi-selection, marquee selection, mouse drag, arrow-key movement, and undo/redo.
- Preserves comments, tags, lambdas, quoting, ordering, and formatting by patching only geometry scalar spans.
- Rejects stale saves using a SHA-256 source revision.

The first version does not expand ESPHome `packages` or `!include`, create widgets, edit general widget properties, or execute ESPHome validation. A YAML file must directly contain the related `display`, `lvgl`, `font`, and `ha_deck` sections that the preview needs.

## Local development

Requirements:

- `uv`
- Node.js with npm

Backend:

```powershell
cd designer/backend
uv sync
$env:HA_DECK_CONFIG_ROOT = (Resolve-Path ../..).Path
uv run uvicorn ha_deck_designer.main:app --reload --port 8099
```

Frontend, in a second terminal:

```powershell
cd designer/frontend
npm install
npm run dev
```

Open `http://127.0.0.1:5173`. Vite proxies `/api` to the backend.

Tests and production build:

```powershell
cd designer/backend
uv run pytest -p no:cacheprovider
uv run ruff check --no-cache .

cd ../frontend
npm test
npm run build
```

## Docker Compose

The included development compose file mounts `examples/` as the configuration root:

```powershell
docker compose -f compose.designer.yaml up --build
```

For a real ESPHome directory, replace the volume source:

```yaml
volumes:
  - /path/to/esphome:/config
```

Then open `http://localhost:8099`.

## Published Docker image

Publishing a GitHub Release builds and pushes the Designer for `linux/amd64` and
`linux/arm64`. Configure the repository before the first release:

- Variable `DOCKERHUB_REPO`, for example `strangev/ha-deck-designer`.
- Secret `DOCKERHUB_USERNAME`.
- Secret `DOCKERHUB_TOKEN` with Docker Hub write permission.

A stable release such as `v1.2.3` publishes `1.2.3`, `1.2`, `1`, and `latest`.
Prereleases containing `-beta` or `-rc` update their corresponding rolling channel.

```yaml
services:
  ha-deck-designer:
    image: strangev/ha-deck-designer:latest
    restart: unless-stopped
    ports:
      - "8099:8099"
    volumes:
      - /path/to/esphome:/config
```

## Keyboard controls

- Arrow keys: move selected widgets by 1 px.
- Shift + Arrow: move by 8 px.
- Ctrl/Cmd + click: toggle a widget in the selection.
- Ctrl/Cmd + Z: undo.
- Ctrl/Cmd + Shift + Z: redo.

Aligned navigation buttons and widgets with dynamic coordinates are displayed but locked. Add literal `x` and `y` values in YAML before moving them in the editor.
