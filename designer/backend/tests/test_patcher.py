from ha_deck_designer.files import calculate_revision
from ha_deck_designer.models import CoordinateChange
from ha_deck_designer.yaml_patcher import RevisionConflictError, patch_coordinates

SOURCE = """# Keep this comment
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
            id: button_1
            x: 8
            y: 16
            width: 64
            height: 64
            text: Українська
            visible: !lambda return true;
""".replace("\n", "\r\n")


def test_patches_only_coordinate_scalars_and_preserves_crlf():
    result = patch_coordinates(
        SOURCE,
        path="dashboard.yaml",
        expected_revision=calculate_revision(SOURCE),
        changes=[
            CoordinateChange(
                screen_id="screen", widget_id="button_1", x=24, y=32, width=80, height=96
            )
        ],
    )

    expected = SOURCE.replace("            x: 8\r\n", "            x: 24\r\n").replace(
        "            y: 16\r\n", "            y: 32\r\n"
    ).replace("            width: 64\r\n", "            width: 80\r\n").replace(
        "            height: 64\r\n", "            height: 96\r\n"
    )
    assert result.source == expected
    assert "Українська" in result.source
    assert "!lambda return true;" in result.source


def test_rejects_stale_revision():
    try:
        patch_coordinates(
            SOURCE,
            path="dashboard.yaml",
            expected_revision="sha256:stale",
            changes=[CoordinateChange(screen_id="screen", widget_id="button_1", x=24, y=32)],
        )
    except RevisionConflictError:
        pass
    else:
        raise AssertionError("Expected revision conflict")
