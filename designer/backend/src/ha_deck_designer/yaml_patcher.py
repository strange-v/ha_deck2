import difflib
from dataclasses import dataclass

from .files import calculate_revision
from .models import CoordinateChange
from .yaml_projection import LayoutProjectionError, parse_layout


class RevisionConflictError(ValueError):
    pass


class WidgetNotFoundError(ValueError):
    pass


class CoordinateNotEditableError(ValueError):
    pass


@dataclass(frozen=True)
class PatchResult:
    source: str
    revision: str
    diff: str


@dataclass(frozen=True)
class _Patch:
    start: int
    end: int
    replacement: str


def patch_coordinates(
    source: str,
    *,
    path: str,
    expected_revision: str,
    changes: list[CoordinateChange],
) -> PatchResult:
    actual_revision = calculate_revision(source)
    if actual_revision != expected_revision:
        raise RevisionConflictError(
            "The configuration changed after it was loaded; reload before saving"
        )
    parsed = parse_layout(source, path)
    widget_keys = {
        (screen.id, widget.id)
        for screen in parsed.document.screens
        for widget in screen.widgets
        if widget.id
    }
    patches: list[_Patch] = []
    seen: set[tuple[str, str]] = set()
    for change in changes:
        widget_key = (change.screen_id, change.widget_id)
        if widget_key in seen:
            raise CoordinateNotEditableError(
                f"Widget '{change.widget_id}' in screen "
                f"'{change.screen_id}' occurs twice in changes"
            )
        seen.add(widget_key)
        if widget_key not in widget_keys:
            raise WidgetNotFoundError(
                f"Widget '{change.widget_id}' was not found in screen '{change.screen_id}'"
            )
        values = (
            ("x", change.x),
            ("y", change.y),
            ("width", change.width),
            ("height", change.height),
        )
        if not any(value is not None for _, value in values):
            raise CoordinateNotEditableError(f"Widget '{change.widget_id}' change is empty")
        for coordinate, value in values:
            if value is None:
                continue
            span = parsed.coordinates.get((*widget_key, coordinate))
            if span is None:
                raise CoordinateNotEditableError(
                    f"Widget '{change.widget_id}' {coordinate} coordinate is not a literal integer"
                )
            patches.append(_Patch(span.start, span.end, str(value)))
    updated = source
    for patch in sorted(patches, key=lambda item: item.start, reverse=True):
        updated = f"{updated[: patch.start]}{patch.replacement}{updated[patch.end :]}"
    try:
        parse_layout(updated, path)
    except LayoutProjectionError as exc:
        raise CoordinateNotEditableError(f"Patched YAML failed validation: {exc}") from exc
    diff = "".join(
        difflib.unified_diff(
            source.splitlines(keepends=True),
            updated.splitlines(keepends=True),
            fromfile=path,
            tofile=path,
        )
    )
    return PatchResult(source=updated, revision=calculate_revision(updated), diff=diff)
