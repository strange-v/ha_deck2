import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

import esphome.config_validation as cv

from components.ha_deck import (
    CONF_ACTION_HEIGHT,
    CONF_ACCENT,
    CONF_CURRENT_TEMPERATURE,
    CONF_DRYING_ACCENT,
    CONF_FORMAT,
    CONF_FONTS,
    CONF_HEIGHT,
    CONF_HEATING_ACCENT,
    CONF_ICON_MEDIUM,
    CONF_ID,
    CONF_LABEL,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_ORIENTATION,
    CONF_PRIMARY_CONTROLS,
    CONF_TARGET_TEMPERATURE,
    CONF_WIDTH,
    CONF_SCREENS,
    CONF_THEMES,
    CONF_WEATHER,
    CONF_WEATHER_ICON_DIRECTORY,
    CONF_WEATHER_ICON_SIZE,
    CONF_WIDGETS,
    CONF_DEFAULT_THEME,
    CONF_VARIANT,
    BUTTON_SCHEMA,
    CLIMATE_SCHEMA,
    _final_validate_weather_icon_size,
    _validate_float_format,
    _validate_slider,
    _validate_weather_icon_directory,
    WEATHER_CONDITIONS,
)
from esphome.const import CONF_SIZE
from esphome.core import CORE
from esphome.final_validate import full_config


class FormatValidationTest(unittest.TestCase):
    def test_accepts_one_float_and_escaped_percent(self):
        self.assertEqual(_validate_float_format("%.1f °C"), "%.1f °C")
        self.assertEqual(_validate_float_format("%.0f%%"), "%.0f%%")

    def test_rejects_unsafe_or_ambiguous_formats(self):
        for value in ("value", "%s", "%n", "%f %f", "%*f", "%d"):
            with self.subTest(value=value), self.assertRaises(cv.Invalid):
                _validate_float_format(value)


class SliderValidationTest(unittest.TestCase):
    def _config(self, minimum=0, maximum=100):
        return {
            CONF_MIN_VALUE: minimum,
            CONF_MAX_VALUE: maximum,
            CONF_LABEL: "",
            CONF_WIDTH: 64,
            CONF_HEIGHT: 176,
            CONF_ORIENTATION: "vertical",
            CONF_ACTION_HEIGHT: 56,
            CONF_FORMAT: "%.0f%%",
        }

    def test_accepts_increasing_range(self):
        config = self._config()
        self.assertIs(_validate_slider(config), config)

    def test_rejects_empty_or_reversed_range(self):
        for minimum, maximum in ((1, 1), (10, 0)):
            with self.subTest(minimum=minimum, maximum=maximum), self.assertRaises(cv.Invalid):
                _validate_slider(self._config(minimum, maximum))


class ButtonVariantValidationTest(unittest.TestCase):
    def _config(self, variant):
        return {
            "x": 0,
            "y": 0,
            CONF_WIDTH: 64,
            CONF_HEIGHT: 64,
            "text": "Test",
            CONF_VARIANT: variant,
        }

    def test_accepts_supported_variants(self):
        for variant in ("filled", "glass", "icon"):
            with self.subTest(variant=variant):
                config = BUTTON_SCHEMA(self._config(variant))
                self.assertEqual(config[CONF_VARIANT], variant)

    def test_rejects_unknown_variant(self):
        with self.assertRaises(cv.Invalid):
            BUTTON_SCHEMA(self._config("outline"))


class ClimateDefaultsTest(unittest.TestCase):
    def test_uses_semantic_mode_accents(self):
        config = CLIMATE_SCHEMA({
            CONF_WIDTH: 480,
            CONF_HEIGHT: 480,
            "x": 0,
            "y": 0,
            CONF_CURRENT_TEMPERATURE: 20,
            CONF_TARGET_TEMPERATURE: 21,
            CONF_PRIMARY_CONTROLS: [{}],
        })

        self.assertEqual(config[CONF_ACCENT], "climate")
        self.assertEqual(config[CONF_HEATING_ACCENT], "heat")
        self.assertEqual(config[CONF_DRYING_ACCENT], "dry")


class WeatherIconSizeValidationTest(unittest.TestCase):
    class _FullConfig:
        def __init__(self, sizes):
            self.sizes = sizes

        def get_path_for_id(self, font_id):
            return ["font", font_id, CONF_ID]

        def get_config_for_path(self, path):
            return {CONF_SIZE: self.sizes[path[1]]}

    def _config(self, dark_size="icons_dark", light_size="icons_light"):
        return {
            CONF_DEFAULT_THEME: "dark",
            CONF_SCREENS: [{CONF_WIDGETS: [{CONF_WEATHER: {}}]}],
            CONF_THEMES: [
                {CONF_ID: "dark", CONF_FONTS: {CONF_ICON_MEDIUM: dark_size}},
                {CONF_ID: "light", CONF_FONTS: {CONF_ICON_MEDIUM: light_size}},
            ],
        }

    def test_derives_common_icon_medium_size(self):
        config = self._config()
        token = full_config.set(self._FullConfig({"icons_dark": 48, "icons_light": 48}))
        try:
            self.assertIs(_final_validate_weather_icon_size(config), config)
        finally:
            full_config.reset(token)
        self.assertEqual(config[CONF_WEATHER_ICON_SIZE], 48)

    def test_rejects_different_theme_sizes_without_override(self):
        config = self._config()
        token = full_config.set(self._FullConfig({"icons_dark": 48, "icons_light": 32}))
        try:
            with self.assertRaises(cv.Invalid):
                _final_validate_weather_icon_size(config)
        finally:
            full_config.reset(token)

    def test_explicit_override_allows_different_theme_sizes(self):
        config = self._config()
        config[CONF_WEATHER_ICON_SIZE] = 64
        self.assertIs(_final_validate_weather_icon_size(config), config)
        self.assertEqual(config[CONF_WEATHER_ICON_SIZE], 64)


class WeatherIconDirectoryValidationTest(unittest.TestCase):
    def setUp(self):
        self._original_config_path = CORE.config_path

    def tearDown(self):
        CORE.config_path = self._original_config_path

    def test_accepts_complete_icon_set(self):
        with TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            CORE.config_path = directory / "dashboard.yaml"
            for condition in WEATHER_CONDITIONS:
                (directory / f"{condition}.svg").write_text("<svg/>", encoding="utf-8")

            self.assertEqual(
                _validate_weather_icon_directory(directory),
                directory,
            )

    def test_lists_all_missing_icons(self):
        with TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            CORE.config_path = directory / "dashboard.yaml"
            (directory / "clear-day.svg").write_text("<svg/>", encoding="utf-8")

            with self.assertRaises(cv.Invalid) as context:
                _validate_weather_icon_directory(directory)

            message = str(context.exception)
            self.assertIn("clear-night.svg", message)
            self.assertIn("windy-variant.svg", message)
            self.assertNotIn("clear-day.svg", message)


if __name__ == "__main__":
    unittest.main()
