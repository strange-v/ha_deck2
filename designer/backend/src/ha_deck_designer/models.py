from typing import Literal

from pydantic import BaseModel, Field


class FontResource(BaseModel):
    id: str
    size: int
    file: str | None = None


class AccentResource(BaseModel):
    color: str
    on_color: str


class ThemeFonts(BaseModel):
    id: str
    base: str = "dark"
    body_font: FontResource | None = None
    fonts: dict[str, FontResource] = Field(default_factory=dict)
    colors: dict[str, str] = Field(default_factory=dict)
    accents: dict[str, AccentResource] = Field(default_factory=dict)


class WidgetGeometry(BaseModel):
    x: int | None = None
    y: int | None = None
    width: int | None = None
    height: int | None = None


class WidgetPreview(BaseModel):
    text: str | None = None
    label: str | None = None
    glyph: str | None = None
    variant: str | None = None
    accent: str | None = None
    background_color: str | None = None
    text_color: str | None = None
    border_color: str | None = None
    border_width: int | None = None
    radius: int | None = None
    orientation: str | None = None
    align: str | None = None
    margin: int | None = None
    units: str | None = None
    format: str | None = None
    action_height: int | None = None
    power_glyph: str | None = None
    explicit_fonts: dict[str, FontResource] = Field(default_factory=dict)


class LayoutWidget(BaseModel):
    key: str
    type: str
    id: str | None = None
    geometry: WidgetGeometry
    movable: bool
    resizable: bool
    read_only_reason: str | None = None
    preview: WidgetPreview


class LayoutScreen(BaseModel):
    id: str
    background_color: str | None = None
    background_image: str | None = None
    background_image_file: str | None = None
    widgets: list[LayoutWidget]


class Viewport(BaseModel):
    width: int
    height: int
    display_id: str


class LayoutDocument(BaseModel):
    path: str
    revision: str
    deck_id: str
    default_screen: str
    default_theme: str
    viewport: Viewport
    themes: list[ThemeFonts]
    screens: list[LayoutScreen]
    warnings: list[str] = Field(default_factory=list)


class ConfigurationSummary(BaseModel):
    path: str
    has_ha_deck: bool
    error: str | None = None


class CoordinateChange(BaseModel):
    screen_id: str
    widget_id: str
    x: int | None = None
    y: int | None = None
    width: int | None = Field(default=None, gt=0)
    height: int | None = Field(default=None, gt=0)


class PatchLayoutRequest(BaseModel):
    path: str
    revision: str
    changes: list[CoordinateChange] = Field(min_length=1)


class PatchLayoutResponse(BaseModel):
    revision: str
    diff: str


class ErrorDetail(BaseModel):
    code: Literal[
        "invalid_path",
        "invalid_yaml",
        "layout_not_found",
        "dimensions_unresolved",
        "font_unresolved",
        "revision_conflict",
        "widget_not_found",
        "coordinate_not_editable",
    ]
    message: str
