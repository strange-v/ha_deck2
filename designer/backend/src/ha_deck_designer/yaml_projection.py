import re
from dataclasses import dataclass
from typing import Any

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq
from ruamel.yaml.error import YAMLError

from .files import calculate_revision
from .models import (
    AccentResource,
    FontResource,
    LayoutDocument,
    LayoutScreen,
    LayoutWidget,
    ThemeFonts,
    Viewport,
    WidgetGeometry,
    WidgetPreview,
)


class LayoutProjectionError(ValueError):
    code = "invalid_yaml"


class LayoutNotFoundError(LayoutProjectionError):
    code = "layout_not_found"


class DimensionsUnresolvedError(LayoutProjectionError):
    code = "dimensions_unresolved"


@dataclass(frozen=True)
class EditableCoordinate:
    start: int
    end: int


@dataclass
class ParsedLayout:
    document: LayoutDocument
    coordinates: dict[tuple[str, str, str], EditableCoordinate]


_DIMENSIONS = re.compile(r"^\s*(\d+)\s*[xX]\s*(\d+)\s*$")
_INTEGER = re.compile(r"[-+]?\d+")
_THEME_FONT_KEYS = {
    "text_small",
    "text_medium",
    "text_large",
    "icon_small",
    "icon_medium",
}
_COLOR_KEYS = {
    "surface_color",
    "surface_container_color",
    "primary_color",
    "on_primary_color",
    "primary_container_color",
    "on_primary_container_color",
    "error_color",
    "on_error_color",
    "on_surface_color",
    "outline_color",
    "background_color",
}

_DARK_COLORS = {
    "background_color": "#111318",
    "surface_color": "#111318",
    "surface_container_color": "#1d2026",
    "primary_color": "#a8c7fa",
    "on_primary_color": "#062e6f",
    "primary_container_color": "#284777",
    "on_primary_container_color": "#d7e3ff",
    "error_color": "#ffb4ab",
    "on_error_color": "#690005",
    "on_surface_color": "#e2e2e9",
    "outline_color": "#8c9199",
}
_LIGHT_COLORS = {
    "background_color": "#f9f9ff",
    "surface_color": "#f9f9ff",
    "surface_container_color": "#ededf4",
    "primary_color": "#0b57d0",
    "on_primary_color": "#ffffff",
    "primary_container_color": "#d7e3ff",
    "on_primary_container_color": "#001b3f",
    "error_color": "#ba1a1a",
    "on_error_color": "#ffffff",
    "on_surface_color": "#1a1b20",
    "outline_color": "#74777f",
}
_DEFAULT_ACCENTS = {
    "light": AccentResource(color="#fcd663", on_color="#211a0a"),
    "climate": AccentResource(color="#2196f3", on_color="#001d36"),
    "heat": AccentResource(color="#ff6f22", on_color="#3d1200"),
    "dry": AccentResource(color="#26a69a", on_color="#00201c"),
}


def _yaml() -> YAML:
    parser = YAML(typ="rt")
    parser.preserve_quotes = True
    return parser


def _mapping(value: Any, description: str) -> CommentedMap:
    if not isinstance(value, CommentedMap):
        raise LayoutProjectionError(f"{description} must be a mapping")
    return value


def _instances(value: Any, description: str) -> list[CommentedMap]:
    if isinstance(value, CommentedMap):
        return [value]
    if isinstance(value, CommentedSeq) and all(isinstance(item, CommentedMap) for item in value):
        return list(value)
    raise LayoutProjectionError(f"{description} must be a mapping or list of mappings")


def _literal_string(value: Any) -> str | None:
    return str(value) if isinstance(value, str) else None


def _literal_int(value: Any) -> int | None:
    return int(value) if isinstance(value, int) and not isinstance(value, bool) else None


def _required_id(mapping: CommentedMap, description: str) -> str:
    value = _literal_string(mapping.get("id"))
    if not value:
        raise LayoutProjectionError(f"{description} requires a literal id")
    return value


def _format_color(value: Any) -> str | None:
    if isinstance(value, int) and not isinstance(value, bool):
        return f"#{value & 0xFFFFFF:06x}"
    return _literal_string(value)


def _parse_dimensions(value: Any, display_id: str) -> tuple[int, int]:
    literal = _literal_string(value)
    if literal:
        match = _DIMENSIONS.fullmatch(literal)
        if match:
            width, height = (int(part) for part in match.groups())
            if width > 0 and height > 0:
                return width, height
    if isinstance(value, CommentedMap):
        width = _literal_int(value.get("width"))
        height = _literal_int(value.get("height"))
        if width and height and width > 0 and height > 0:
            return width, height
    raise DimensionsUnresolvedError(
        f"Display '{display_id}' dimensions must be literal WIDTHxHEIGHT or width/height values"
    )


def _display_references(value: Any) -> list[str]:
    values = list(value) if isinstance(value, CommentedSeq) else [value]
    references: list[str] = []
    for item in values:
        if literal := _literal_string(item):
            references.append(literal)
        elif isinstance(item, CommentedMap):
            reference = _literal_string(item.get("display_id"))
            if reference:
                references.append(reference)
    return references


def _resolve_viewport(root: CommentedMap, deck: CommentedMap) -> Viewport:
    lvgl_id = _literal_string(deck.get("lvgl_id"))
    if not lvgl_id:
        raise DimensionsUnresolvedError("ha_deck.lvgl_id must be a literal ID")
    if "lvgl" not in root:
        raise DimensionsUnresolvedError("The YAML document has no lvgl section")
    lvgl_instances = _instances(root["lvgl"], "lvgl")
    lvgl_matches = [item for item in lvgl_instances if _literal_string(item.get("id")) == lvgl_id]
    if len(lvgl_matches) != 1:
        raise DimensionsUnresolvedError(
            f"Expected exactly one lvgl instance with id '{lvgl_id}', found {len(lvgl_matches)}"
        )
    display_ids = _display_references(lvgl_matches[0].get("displays"))
    if len(display_ids) != 1:
        raise DimensionsUnresolvedError(
            f"LVGL '{lvgl_id}' must reference exactly one literal display, found {len(display_ids)}"
        )
    display_id = display_ids[0]
    if "display" not in root:
        raise DimensionsUnresolvedError("The YAML document has no display section")
    displays = _instances(root["display"], "display")
    matches = [item for item in displays if _literal_string(item.get("id")) == display_id]
    if len(matches) != 1:
        raise DimensionsUnresolvedError(
            f"Expected exactly one display with id '{display_id}', found {len(matches)}"
        )
    if "dimensions" not in matches[0]:
        raise DimensionsUnresolvedError(f"Display '{display_id}' has no dimensions property")
    width, height = _parse_dimensions(matches[0]["dimensions"], display_id)
    return Viewport(width=width, height=height, display_id=display_id)


def _font_catalog(root: CommentedMap) -> dict[str, FontResource]:
    if "font" not in root:
        return {}
    catalog: dict[str, FontResource] = {}
    for font in _instances(root["font"], "font"):
        font_id = _literal_string(font.get("id"))
        size = _literal_int(font.get("size"))
        if not font_id or size is None or size <= 0:
            continue
        file_value = _literal_string(font.get("file"))
        catalog[font_id] = FontResource(id=font_id, size=size, file=file_value)
    return catalog


def _image_catalog(root: CommentedMap) -> dict[str, str]:
    if "image" not in root:
        return {}
    catalog: dict[str, str] = {}
    for image in _instances(root["image"], "image"):
        image_id = _literal_string(image.get("id"))
        file_value = _literal_string(image.get("file"))
        if image_id and file_value:
            catalog[image_id] = file_value
    return catalog


def _resolve_font(reference: Any, catalog: dict[str, FontResource]) -> FontResource | None:
    font_id = _literal_string(reference)
    return catalog.get(font_id) if font_id else None


def _resolve_themes(
    deck: CommentedMap, catalog: dict[str, FontResource], warnings: list[str]
) -> list[ThemeFonts]:
    themes_value = deck.get("themes")
    if not isinstance(themes_value, CommentedSeq):
        raise LayoutProjectionError("ha_deck.themes must be a list")
    result: list[ThemeFonts] = []
    for index, value in enumerate(themes_value):
        theme = _mapping(value, f"ha_deck.themes[{index}]")
        theme_id = _required_id(theme, f"ha_deck.themes[{index}]")
        base = _literal_string(theme.get("base")) or "dark"
        resolved = ThemeFonts(
            id=theme_id,
            base=base,
            colors=dict(_LIGHT_COLORS if base == "light" else _DARK_COLORS),
            accents={name: value.model_copy() for name, value in _DEFAULT_ACCENTS.items()},
        )
        body_font_id = _literal_string(theme.get("body_font"))
        if body_font_id:
            resolved.body_font = catalog.get(body_font_id)
            if resolved.body_font is None:
                warnings.append(
                    f"Theme '{theme_id}' references unresolved body font '{body_font_id}'"
                )
        fonts = theme.get("fonts")
        if isinstance(fonts, CommentedMap):
            for token in _THEME_FONT_KEYS:
                if token not in fonts:
                    continue
                font_id = _literal_string(fonts[token])
                resource = catalog.get(font_id) if font_id else None
                if resource:
                    resolved.fonts[token] = resource
                else:
                    warnings.append(
                        f"Theme '{theme_id}' font token '{token}' references "
                        f"'{font_id or '<dynamic>'}', which cannot be resolved "
                        "to a literal font size"
                    )
        for color in _COLOR_KEYS:
            formatted = _format_color(theme.get(color))
            if formatted:
                resolved.colors[color] = formatted
        accents = theme.get("accents")
        if isinstance(accents, CommentedSeq):
            for accent_value in accents:
                if not isinstance(accent_value, CommentedMap):
                    continue
                name = _literal_string(accent_value.get("name"))
                color = _format_color(accent_value.get("color"))
                on_color = _format_color(accent_value.get("on_color"))
                if name and color and on_color:
                    resolved.accents[name] = AccentResource(color=color, on_color=on_color)
        result.append(resolved)
    return result


def _preview(widget: CommentedMap, catalog: dict[str, FontResource]) -> WidgetPreview:
    icon = widget.get("icon")
    icon_map = icon if isinstance(icon, CommentedMap) else CommentedMap()
    power = widget.get("power")
    power_map = power if isinstance(power, CommentedMap) else CommentedMap()
    preview = WidgetPreview(
        text=_literal_string(widget.get("text")),
        label=_literal_string(widget.get("label")),
        glyph=_literal_string(icon_map.get("glyph")) or _literal_string(widget.get("glyph")),
        variant=_literal_string(widget.get("variant")),
        accent=_literal_string(widget.get("accent")),
        background_color=_format_color(widget.get("background_color")),
        text_color=_format_color(widget.get("text_color")),
        border_color=_format_color(widget.get("border_color")),
        border_width=_literal_int(widget.get("border_width")),
        radius=_literal_int(widget.get("radius")),
        orientation=_literal_string(widget.get("orientation")),
        align=_literal_string(widget.get("align")),
        margin=_literal_int(widget.get("margin")),
        units=_literal_string(widget.get("units")),
        format=_literal_string(widget.get("format")),
        action_height=_literal_int(widget.get("action_height")),
        power_glyph=_literal_string(power_map.get("icon")),
    )
    for role, reference in (
        ("font", widget.get("font")),
        ("value_font", widget.get("value_font")),
        ("text_font", widget.get("text_font")),
        ("icon_font", icon_map.get("font")),
    ):
        resource = _resolve_font(reference, catalog)
        if resource:
            preview.explicit_fonts[role] = resource
    return preview


def _widget_preview(
    widget_type: str, widget: CommentedMap, catalog: dict[str, FontResource]
) -> WidgetPreview:
    preview = _preview(widget, catalog)
    if widget_type == "navigation_button" and preview.glyph is None:
        preview.glyph = "\U000f004d"
    if widget_type == "climate" and preview.power_glyph is None:
        preview.power_glyph = "\U000f0425"
    return preview


def _source_span(source: str, mapping: CommentedMap, key: str) -> EditableCoordinate | None:
    value = _literal_int(mapping.get(key))
    if value is None:
        return None
    line, column = mapping.lc.value(key)
    lines = source.splitlines(keepends=True)
    if line < 0 or line >= len(lines):
        return None
    line_start = sum(len(part) for part in lines[:line])
    tail = lines[line][column:]
    match = _INTEGER.match(tail)
    if not match:
        return None
    return EditableCoordinate(
        start=line_start + column + match.start(),
        end=line_start + column + match.end(),
    )


def parse_layout(source: str, path: str) -> ParsedLayout:
    try:
        loaded = _yaml().load(source)
    except YAMLError as exc:
        raise LayoutProjectionError(f"Invalid YAML: {exc}") from exc
    root = _mapping(loaded, "YAML document")
    if "ha_deck" not in root:
        raise LayoutNotFoundError("The YAML document has no ha_deck section")
    decks = _instances(root["ha_deck"], "ha_deck")
    if len(decks) != 1:
        raise LayoutNotFoundError(
            f"The first version supports exactly one ha_deck instance, found {len(decks)}"
        )
    deck = decks[0]
    deck_id = _required_id(deck, "ha_deck")
    viewport = _resolve_viewport(root, deck)
    font_catalog = _font_catalog(root)
    image_catalog = _image_catalog(root)
    warnings: list[str] = []
    themes = _resolve_themes(deck, font_catalog, warnings)
    default_theme = _literal_string(deck.get("default_theme"))
    default_screen = _literal_string(deck.get("default_screen"))
    if not default_theme or not default_screen:
        raise LayoutProjectionError("ha_deck default_theme and default_screen must be literal IDs")
    screens_value = deck.get("screens")
    if not isinstance(screens_value, CommentedSeq):
        raise LayoutProjectionError("ha_deck.screens must be a list")
    screens: list[LayoutScreen] = []
    coordinates: dict[tuple[str, str, str], EditableCoordinate] = {}
    for screen_index, screen_value in enumerate(screens_value):
        screen = _mapping(screen_value, f"ha_deck.screens[{screen_index}]")
        screen_id = _required_id(screen, f"ha_deck.screens[{screen_index}]")
        widgets_value = screen.get("widgets")
        if not isinstance(widgets_value, CommentedSeq):
            raise LayoutProjectionError(f"Screen '{screen_id}' widgets must be a list")
        widgets: list[LayoutWidget] = []
        seen_ids: set[str] = set()
        duplicate_ids: set[str] = set()
        for widget_index, wrapper_value in enumerate(widgets_value):
            wrapper = _mapping(wrapper_value, f"Screen '{screen_id}' widget {widget_index}")
            if len(wrapper) != 1:
                raise LayoutProjectionError(
                    f"Screen '{screen_id}' widget {widget_index} must have exactly one type key"
                )
            widget_type, widget_value = next(iter(wrapper.items()))
            widget = _mapping(widget_value, f"Widget '{widget_type}'")
            widget_id = _literal_string(widget.get("id"))
            if widget_id and widget_id in seen_ids:
                warnings.append(f"Screen '{screen_id}' has duplicate widget id '{widget_id}'")
                duplicate_ids.add(widget_id)
            if widget_id:
                seen_ids.add(widget_id)
            geometry = WidgetGeometry(
                x=_literal_int(widget.get("x")),
                y=_literal_int(widget.get("y")),
                width=_literal_int(widget.get("width")),
                height=_literal_int(widget.get("height")),
            )
            x_span = _source_span(source, widget, "x")
            y_span = _source_span(source, widget, "y")
            width_span = _source_span(source, widget, "width")
            height_span = _source_span(source, widget, "height")
            movable = bool(widget_id and x_span and y_span)
            resizable = bool(widget_id and width_span and height_span)
            reason = None
            if not widget_id:
                reason = "Widget requires a literal id before it can be moved"
            elif x_span is None or y_span is None:
                reason = "Widget x and y must be literal integers"
            if movable and widget_id:
                coordinates[(screen_id, widget_id, "x")] = x_span
                coordinates[(screen_id, widget_id, "y")] = y_span
            if resizable and widget_id:
                coordinates[(screen_id, widget_id, "width")] = width_span
                coordinates[(screen_id, widget_id, "height")] = height_span
            widgets.append(
                LayoutWidget(
                    key=f"{screen_id}/{widget_id or f'{widget_type}-{widget_index}'}",
                    type=str(widget_type),
                    id=widget_id,
                    geometry=geometry,
                    movable=movable,
                    resizable=resizable,
                    read_only_reason=reason,
                    preview=_widget_preview(str(widget_type), widget, font_catalog),
                )
            )
        if duplicate_ids:
            for widget in widgets:
                if widget.id in duplicate_ids:
                    widget.movable = False
                    widget.resizable = False
                    widget.read_only_reason = "Duplicate widget IDs cannot be edited safely"
                    for field in ("x", "y", "width", "height"):
                        coordinates.pop((screen_id, widget.id, field), None)
        background_image = _literal_string(screen.get("background_image"))
        background_image_file = image_catalog.get(background_image) if background_image else None
        if background_image and background_image_file is None:
            warnings.append(
                f"Screen '{screen_id}' references unresolved background image '{background_image}'"
            )
        screens.append(
            LayoutScreen(
                id=screen_id,
                background_color=_format_color(screen.get("background_color")),
                background_image=background_image,
                background_image_file=background_image_file,
                widgets=widgets,
            )
        )
    document = LayoutDocument(
        path=path,
        revision=calculate_revision(source),
        deck_id=deck_id,
        default_screen=default_screen,
        default_theme=default_theme,
        viewport=viewport,
        themes=themes,
        screens=screens,
        warnings=warnings,
    )
    return ParsedLayout(document=document, coordinates=coordinates)
