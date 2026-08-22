from pathlib import Path

import pytest

from ha_deck_designer.yaml_projection import DimensionsUnresolvedError, parse_layout

REPOSITORY_ROOT = Path(__file__).parents[3]


def test_projects_real_dashboard_with_dimensions_and_fonts():
    path = REPOSITORY_ROOT / "examples" / "dashboard.yaml"
    document = parse_layout(path.read_text(encoding="utf-8"), "examples/dashboard.yaml").document

    assert (document.viewport.width, document.viewport.height) == (480, 480)
    assert document.viewport.display_id == "dashboard_display"
    assert document.screens[0].background_color is None
    assert document.screens[0].background_image == "dashboard_background"
    assert document.screens[0].background_image_file == "assets/480x480_trees.jpg"
    assert [screen.id for screen in document.screens] == [
        "scr_main",
        "scr_heating",
        "scr_ac",
        "scr_tv",
    ]
    theme = document.themes[0]
    assert theme.fonts["text_small"].size == 16
    assert theme.fonts["icon_medium"].size == 48
    navigation = next(
        widget
        for screen in document.screens
        for widget in screen.widgets
        if widget.type == "navigation_button"
    )
    assert navigation.preview.glyph == "\U000f004d"
    assert document.warnings == []


def test_dimensions_are_required_and_have_no_fallback():
    source = """
display:
  - id: display_1
lvgl:
  id: lvgl_1
  displays: display_1
ha_deck:
  id: deck
  lvgl_id: lvgl_1
  default_theme: theme
  default_screen: screen
  themes:
    - id: theme
  screens:
    - id: screen
      widgets: []
"""

    with pytest.raises(DimensionsUnresolvedError, match="has no dimensions"):
        parse_layout(source, "dashboard.yaml")


def test_unresolved_font_is_reported_without_blocking_geometry():
    source = """
display:
  - id: display_1
    dimensions: 320x240
lvgl:
  id: lvgl_1
  displays: display_1
ha_deck:
  id: deck
  lvgl_id: lvgl_1
  default_theme: theme
  default_screen: screen
  themes:
    - id: theme
      fonts:
        text_small: missing_font
  screens:
    - id: screen
      widgets: []
"""

    document = parse_layout(source, "dashboard.yaml").document

    assert document.viewport.width == 320
    assert document.themes[0].fonts == {}
    assert "missing_font" in document.warnings[0]


def test_dynamic_coordinates_are_read_only():
    source = """
display:
  - id: display_1
    dimensions: 320x240
lvgl:
  id: lvgl_1
  displays: display_1
ha_deck:
  id: deck
  lvgl_id: lvgl_1
  default_theme: theme
  default_screen: screen
  themes:
    - id: theme
  screens:
    - id: screen
      widgets:
        - button:
            id: dynamic_button
            x: ${button_x}
            y: 8
            width: 64
            height: 64
            text: Test
"""

    widget = parse_layout(source, "dashboard.yaml").document.screens[0].widgets[0]

    assert widget.movable is False
    assert widget.geometry.x is None
    assert "literal integers" in widget.read_only_reason


def test_duplicate_widget_ids_are_visible_but_not_editable():
    source = """
display:
  - id: display_1
    dimensions: 320x240
lvgl:
  id: lvgl_1
  displays: display_1
ha_deck:
  id: deck
  lvgl_id: lvgl_1
  default_theme: theme
  default_screen: screen
  themes:
    - id: theme
  screens:
    - id: screen
      widgets:
        - button: {id: duplicate, x: 0, y: 0, width: 10, height: 10}
        - button: {id: duplicate, x: 20, y: 20, width: 10, height: 10}
"""

    parsed = parse_layout(source, "dashboard.yaml")

    assert all(not widget.movable for widget in parsed.document.screens[0].widgets)
    assert parsed.coordinates == {}
    assert "duplicate widget id" in parsed.document.warnings[0]
