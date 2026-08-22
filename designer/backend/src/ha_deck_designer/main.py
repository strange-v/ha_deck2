import os
from functools import lru_cache
from pathlib import Path

from fastapi import Depends, FastAPI, HTTPException, Query
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from .files import (
    InvalidConfigPathError,
    atomic_write_text,
    resolve_config_path,
)
from .models import (
    ConfigurationSummary,
    LayoutDocument,
    PatchLayoutRequest,
    PatchLayoutResponse,
)
from .settings import Settings
from .yaml_patcher import (
    CoordinateNotEditableError,
    RevisionConflictError,
    WidgetNotFoundError,
    patch_coordinates,
)
from .yaml_projection import (
    DimensionsUnresolvedError,
    LayoutNotFoundError,
    LayoutProjectionError,
    parse_layout,
)


@lru_cache
def get_settings() -> Settings:
    return Settings()


app = FastAPI(title="HA Deck Layout Editor", version="0.1.0")


def _http_error(exc: Exception) -> HTTPException:
    if isinstance(exc, InvalidConfigPathError):
        return HTTPException(400, detail={"code": "invalid_path", "message": str(exc)})
    if isinstance(exc, DimensionsUnresolvedError):
        return HTTPException(422, detail={"code": "dimensions_unresolved", "message": str(exc)})
    if isinstance(exc, LayoutNotFoundError):
        return HTTPException(404, detail={"code": "layout_not_found", "message": str(exc)})
    if isinstance(exc, LayoutProjectionError):
        return HTTPException(422, detail={"code": "invalid_yaml", "message": str(exc)})
    if isinstance(exc, RevisionConflictError):
        return HTTPException(409, detail={"code": "revision_conflict", "message": str(exc)})
    if isinstance(exc, WidgetNotFoundError):
        return HTTPException(404, detail={"code": "widget_not_found", "message": str(exc)})
    if isinstance(exc, CoordinateNotEditableError):
        return HTTPException(422, detail={"code": "coordinate_not_editable", "message": str(exc)})
    return HTTPException(500, detail={"code": "internal_error", "message": str(exc)})


@app.get("/api/health")
async def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/api/configurations", response_model=list[ConfigurationSummary])
async def configurations(settings: Settings = Depends(get_settings)) -> list[ConfigurationSummary]:
    result: list[ConfigurationSummary] = []
    paths: list[Path] = []
    for directory, names, files in os.walk(settings.config_root):
        names[:] = [
            name
            for name in names
            if not name.startswith(".") and name not in {"node_modules", "__pycache__"}
        ]
        paths.extend(
            Path(directory) / name
            for name in files
            if Path(name).suffix.lower() in {".yaml", ".yml"}
        )
    paths.sort()
    for path in paths:
        if not path.is_file():
            continue
        try:
            source = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError):
            continue
        relative_path = path.relative_to(settings.config_root).as_posix()
        has_ha_deck = "ha_deck:" in source
        error = None
        if has_ha_deck:
            try:
                parse_layout(source, relative_path)
            except LayoutProjectionError as exc:
                error = str(exc)
        result.append(
            ConfigurationSummary(path=relative_path, has_ha_deck=has_ha_deck, error=error)
        )
    return result


@app.get("/api/layout", response_model=LayoutDocument)
async def layout(
    path: str = Query(min_length=1), settings: Settings = Depends(get_settings)
) -> LayoutDocument:
    try:
        config_path = resolve_config_path(settings.config_root, path)
        source = config_path.read_text(encoding="utf-8")
        return parse_layout(source, path).document
    except (ValueError, OSError, UnicodeError) as exc:
        raise _http_error(exc) from exc


@app.get("/api/config-asset", response_class=FileResponse)
async def config_asset(
    configuration: str = Query(min_length=1),
    asset: str = Query(min_length=1),
    settings: Settings = Depends(get_settings),
) -> FileResponse:
    try:
        config_path = resolve_config_path(settings.config_root, configuration)
        asset_path = (config_path.parent / asset).resolve(strict=True)
        asset_path.relative_to(settings.config_root)
        if not asset_path.is_file():
            raise InvalidConfigPathError("Asset path is not a file")
        return FileResponse(asset_path)
    except (ValueError, OSError) as exc:
        raise _http_error(
            exc if isinstance(exc, InvalidConfigPathError) else InvalidConfigPathError(str(exc))
        ) from exc


@app.patch("/api/layout", response_model=PatchLayoutResponse)
async def patch_layout(
    request: PatchLayoutRequest, settings: Settings = Depends(get_settings)
) -> PatchLayoutResponse:
    try:
        config_path = resolve_config_path(settings.config_root, request.path)
        source = config_path.read_text(encoding="utf-8")
        result = patch_coordinates(
            source,
            path=request.path,
            expected_revision=request.revision,
            changes=request.changes,
        )
        atomic_write_text(config_path, result.source)
        return PatchLayoutResponse(revision=result.revision, diff=result.diff)
    except (ValueError, OSError, UnicodeError) as exc:
        raise _http_error(exc) from exc


frontend_dist = Path(__file__).parents[3] / "frontend" / "dist"
if frontend_dist.is_dir():
    app.mount("/assets", StaticFiles(directory=frontend_dist / "assets"), name="assets")

    @app.get("/{path:path}", include_in_schema=False)
    async def frontend(path: str) -> FileResponse:
        requested = frontend_dist / path
        return FileResponse(requested if requested.is_file() else frontend_dist / "index.html")
