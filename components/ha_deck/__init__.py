import re
from pathlib import Path

from esphome import automation
import esphome.codegen as cg
from esphome.components import font, image, lvgl
try:
    from esphome.components.file import image as file_image
except ImportError:
    file_image = None
from esphome.components.lvgl.defines import (
    CONF_LVGL_ID, add_lv_use, get_esphome_fonts_used, get_lv_images_used,
)
import esphome.config_validation as cv
from esphome.const import (
    CONF_DISABLED, CONF_DITHER, CONF_FILE, CONF_HEIGHT, CONF_ID, CONF_MAX_VALUE, CONF_MIN_VALUE,
    CONF_ON_CLICK, CONF_ON_PRESS, CONF_ON_RELEASE, CONF_ON_TURN_OFF, CONF_ON_TURN_ON,
    CONF_RAW_DATA_ID, CONF_RESIZE, CONF_SIZE, CONF_TEXT, CONF_TIME, CONF_TYPE, CONF_VALUE,
    CONF_VALUE_FONT, CONF_VISIBLE, CONF_WIDTH, CONF_X, CONF_Y,
)
from esphome.core import ID
from esphome.final_validate import full_config

DEPENDENCIES = ["lvgl"]
AUTO_LOAD = ["font", "image"]
CODEOWNERS = []

CONF_BACKGROUND_COLOR = "background_color"
CONF_BORDER_COLOR = "border_color"
CONF_BORDER_WIDTH = "border_width"
CONF_SCREENS = "screens"
CONF_WIDGETS = "widgets"
CONF_BUTTON = "button"
CONF_DEFAULT_SCREEN = "default_screen"
CONF_THEMES = "themes"
CONF_DEFAULT_THEME = "default_theme"
CONF_BASE = "base"
CONF_ICON = "icon"
CONF_FONT = "font"
CONF_BODY_FONT = "body_font"
CONF_FONTS = "fonts"
CONF_TEXT_SMALL = "text_small"
CONF_TEXT_MEDIUM = "text_medium"
CONF_TEXT_LARGE = "text_large"
CONF_ICON_SMALL = "icon_small"
CONF_ICON_MEDIUM = "icon_medium"
CONF_TEXT_COLOR = "text_color"
CONF_RADIUS = "radius"
CONF_DISABLED_OPACITY = "disabled_opacity"
CONF_VARIANT = "variant"
CONF_ANIMATION = "animation"
CONF_TOGGLE = "toggle"
CONF_CHECKED = "checked"
CONF_ACCENT = "accent"
CONF_ACCENTS = "accents"
CONF_NAME = "name"
CONF_COLOR = "color"
CONF_ON_COLOR = "on_color"
CONF_IMAGE = "image"
CONF_GLYPH = "glyph"
CONF_BACKGROUND_IMAGE = "background_image"
CONF_SENSOR_VALUE = "sensor_value"
CONF_WEATHER = "weather"
CONF_SLIDER = "slider"
CONF_FORMAT = "format"
CONF_UNITS = "units"
CONF_UNAVAILABLE_TEXT = "unavailable_text"
CONF_TOP_TEXT = "top_text"
CONF_BOTTOM_TEXT = "bottom_text"
CONF_TEXT_FONT = "text_font"
CONF_TEMPERATURE = "temperature"
CONF_CONDITION = "condition"
CONF_IS_NIGHT = "is_night"
CONF_LABEL = "label"
CONF_ORIENTATION = "orientation"
CONF_ON_VALUE = "on_value"
CONF_ON_LONG_PRESS = "on_long_press"
CONF_OPTIMISTIC = "optimistic"
CONF_OPTIMISTIC_TIMEOUT = "optimistic_timeout"
CONF_ACTION_HEIGHT = "action_height"
CONF_CLIMATE = "climate"
CONF_NAVIGATION_BUTTON = "navigation_button"
CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_STATE = "state"
CONF_STEP = "step"
CONF_PRIMARY_CONTROLS = "primary_controls"
CONF_ADDITIONAL_CONTROLS = "additional_controls"
CONF_OPTIONS = "options"
CONF_ACTIVE = "active"
CONF_POWER = "power"
CONF_ON_TARGET_CHANGE = "on_target_change"
CONF_ON_SELECT = "on_select"
CONF_BACK = "back"
CONF_TARGET = "target"
CONF_MENU_ICON = "menu_icon"
CONF_CLOSE_ICON = "close_icon"
CONF_METRICS = "metrics"
CONF_SPACING_SMALL = "spacing_small"
CONF_SPACING_MEDIUM = "spacing_medium"
CONF_TOUCH_TARGET = "touch_target"
CONF_CONTROL_HEIGHT = "control_height"
CONF_PADDING = "padding"
CONF_GAP = "gap"
CONF_CONTROLS_HEIGHT = "controls_height"
CONF_ALIGN = "align"
CONF_MARGIN = "margin"
CONF_ARC_MODE = "arc_mode"
CONF_HEATING_ACCENT = "heating_accent"
CONF_DRYING_ACCENT = "drying_accent"
CONF_ACCENT_ICON = "accent_icon"
CONF_WEATHER_ICON_SIZE = "weather_icon_size"
CONF_WEATHER_ICON_DIRECTORY = "weather_icon_directory"

NAVIGATION_ALIGNMENTS = {
    "top_left": "LV_ALIGN_TOP_LEFT",
    "top_right": "LV_ALIGN_TOP_RIGHT",
    "bottom_left": "LV_ALIGN_BOTTOM_LEFT",
    "bottom_right": "LV_ALIGN_BOTTOM_RIGHT",
    "center": "LV_ALIGN_CENTER",
}

WEATHER_CONDITIONS = (
    "clear-day", "clear-night", "cloudy", "exceptional", "fog", "hail",
    "lightning-rainy", "lightning", "partly-cloudy-day", "partly-cloudy-night",
    "pouring", "rain", "snow", "snowy-rainy", "wind", "windy-variant",
)


def _validate_weather_icon_directory(value):
    directory = cv.directory(value)
    missing = [
        f"{condition}.svg"
        for condition in WEATHER_CONDITIONS
        if not (directory / f"{condition}.svg").is_file()
    ]
    if missing:
        raise cv.Invalid(
            "weather_icon_directory is missing required SVG files: "
            + ", ".join(missing)
        )
    return directory

_FLOAT_FORMAT_TOKEN = re.compile(r"%[-+ #0]*\d*(?:\.\d+)?[eEfFgG]")


def _validate_float_format(value):
    """Allow literals, escaped percent signs, and exactly one float conversion."""
    index = 0
    conversions = 0
    while index < len(value):
        if value[index] != "%":
            index += 1
            continue
        if index + 1 < len(value) and value[index + 1] == "%":
            index += 2
            continue
        match = _FLOAT_FORMAT_TOKEN.match(value, index)
        if match is None:
            raise cv.Invalid(
                "'format' must contain one floating-point conversion and may use '%%' for a percent sign"
            )
        conversions += 1
        index = match.end()
    if conversions != 1:
        raise cv.Invalid("'format' must contain exactly one floating-point conversion")
    return value


FLOAT_FORMAT = cv.All(cv.string_strict, _validate_float_format)

THEME_COLOR_KEYS = (
    "surface_color", "surface_container_color", "primary_color", "on_primary_color",
    "primary_container_color", "on_primary_container_color", "error_color",
    "on_error_color", "on_surface_color", "outline_color",
)
THEME_FONT_KEYS = (
    CONF_TEXT_SMALL, CONF_TEXT_MEDIUM, CONF_TEXT_LARGE,
    CONF_ICON_SMALL, CONF_ICON_MEDIUM,
)

ANIMATIONS = {
    "none": "LV_SCREEN_LOAD_ANIM_NONE",
    "over_left": "LV_SCREEN_LOAD_ANIM_OVER_LEFT",
    "over_right": "LV_SCREEN_LOAD_ANIM_OVER_RIGHT",
    "over_top": "LV_SCREEN_LOAD_ANIM_OVER_TOP",
    "over_bottom": "LV_SCREEN_LOAD_ANIM_OVER_BOTTOM",
    "move_left": "LV_SCREEN_LOAD_ANIM_MOVE_LEFT",
    "move_right": "LV_SCREEN_LOAD_ANIM_MOVE_RIGHT",
    "move_top": "LV_SCREEN_LOAD_ANIM_MOVE_TOP",
    "move_bottom": "LV_SCREEN_LOAD_ANIM_MOVE_BOTTOM",
    "fade_in": "LV_SCREEN_LOAD_ANIM_FADE_IN",
    "fade_out": "LV_SCREEN_LOAD_ANIM_FADE_OUT",
    "out_left": "LV_SCREEN_LOAD_ANIM_OUT_LEFT",
    "out_right": "LV_SCREEN_LOAD_ANIM_OUT_RIGHT",
    "out_top": "LV_SCREEN_LOAD_ANIM_OUT_TOP",
    "out_bottom": "LV_SCREEN_LOAD_ANIM_OUT_BOTTOM",
}

ha_deck_ns = cg.esphome_ns.namespace("ha_deck")
HaDeck = ha_deck_ns.class_("HaDeck", cg.PollingComponent)
HaDeckScreen = ha_deck_ns.class_("HaDeckScreen")
HaDeckTheme = ha_deck_ns.class_("HaDeckTheme")
HaDeckButton = ha_deck_ns.class_("HaDeckButton", automation.Trigger.template())
HaDeckSensorValue = ha_deck_ns.class_("HaDeckSensorValue")
HaDeckWeather = ha_deck_ns.class_("HaDeckWeather")
HaDeckSlider = ha_deck_ns.class_("HaDeckSlider")
HaDeckClimate = ha_deck_ns.class_("HaDeckClimate")
HaDeckClimateOption = ha_deck_ns.class_("HaDeckClimateOption", automation.Trigger.template())
HaDeckClimateGroup = ha_deck_ns.class_("HaDeckClimateGroup")
HaDeckNavigationButton = ha_deck_ns.class_("HaDeckNavigationButton")
ShowScreenAction = ha_deck_ns.class_("ShowScreenAction", automation.Action)
SetThemeAction = ha_deck_ns.class_("SetThemeAction", automation.Action)


def _validate_content(value):
    if CONF_TEXT not in value and CONF_ICON not in value:
        raise cv.Invalid("A button needs exactly one of 'text' or 'icon'")
    if CONF_FONT in value and CONF_TEXT not in value:
        raise cv.Invalid("'font' can only be used with a text button")
    if not value[CONF_TOGGLE] and any(
        key in value for key in (CONF_ON_TURN_ON, CONF_ON_TURN_OFF)
    ):
        raise cv.Invalid("'on_turn_on' and 'on_turn_off' require 'toggle: true'")
    return value


def _validate_icon(value):
    if CONF_IMAGE not in value and CONF_GLYPH not in value:
        raise cv.Invalid("An icon needs exactly one of 'image' or 'glyph'")
    if CONF_FONT in value and CONF_GLYPH not in value:
        raise cv.Invalid("An icon font can only be used with 'glyph'")
    return value


_ICON_SCHEMA = cv.All(
    cv.Schema({
        cv.Exclusive(CONF_IMAGE, "source"): cv.use_id(image.Image_),
        cv.Exclusive(CONF_GLYPH, "source"): cv.string_strict,
        cv.Optional(CONF_FONT): cv.use_id(font.Font),
    }),
    _validate_icon,
)


def ICON_SCHEMA(value):
    if not isinstance(value, dict):
        value = {CONF_IMAGE: value}
    return _ICON_SCHEMA(value)


def SLIDER_ICON_SCHEMA(value):
    if not isinstance(value, dict):
        value = {CONF_GLYPH: value}
    return cv.Schema({
        cv.Required(CONF_GLYPH): cv.string_strict,
        cv.Optional(CONF_FONT): cv.use_id(font.Font),
    })(value)


BUTTON_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HaDeckButton),
        cv.Required(CONF_X): cv.int_,
        cv.Required(CONF_Y): cv.int_,
        cv.Required(CONF_WIDTH): cv.positive_int,
        cv.Required(CONF_HEIGHT): cv.positive_int,
        cv.Optional(CONF_VARIANT, default="glass"): cv.one_of(
            "filled", "glass", "icon", lower=True
        ),
        cv.Optional(CONF_ACCENT, default=""): cv.templatable(cv.string_strict),
        cv.Optional(CONF_ACCENT_ICON, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_DISABLED, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
        cv.Optional(CONF_TOGGLE, default=False): cv.boolean,
        cv.Optional(CONF_CHECKED): cv.templatable(cv.boolean),
        cv.Optional(CONF_BACKGROUND_COLOR): cv.hex_uint32_t,
        cv.Optional(CONF_TEXT_COLOR): cv.hex_uint32_t,
        cv.Optional(CONF_BORDER_COLOR): cv.hex_uint32_t,
        cv.Optional(CONF_BORDER_WIDTH): cv.int_range(min=0),
        cv.Optional(CONF_RADIUS): cv.int_range(min=0),
        cv.Optional(CONF_DISABLED_OPACITY): cv.percentage,
        cv.Optional(CONF_ON_CLICK): automation.validate_automation(single=True),
        cv.Optional(CONF_ON_PRESS): automation.validate_automation(single=True),
        cv.Optional(CONF_ON_RELEASE): automation.validate_automation(single=True),
        cv.Optional(CONF_ON_LONG_PRESS): automation.validate_automation(single=True),
        cv.Optional(CONF_ON_TURN_ON): automation.validate_automation(single=True),
        cv.Optional(CONF_ON_TURN_OFF): automation.validate_automation(single=True),
        cv.Optional(CONF_TEXT): cv.string_strict,
        cv.Optional(CONF_ICON): ICON_SCHEMA,
        cv.Optional(CONF_FONT): cv.use_id(font.Font),
    }),
    _validate_content,
)

VALUE_DISPLAY_FIELDS = {
    cv.Required(CONF_X): cv.int_,
    cv.Required(CONF_Y): cv.int_,
    cv.Required(CONF_WIDTH): cv.positive_int,
    cv.Required(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
    cv.Optional(CONF_FORMAT, default="%.1f"): FLOAT_FORMAT,
    cv.Optional(CONF_UNITS, default=""): cv.string_strict,
    cv.Optional(CONF_UNAVAILABLE_TEXT, default="−"): cv.string_strict,
    cv.Optional(CONF_TOP_TEXT, default=""): cv.string_strict,
    cv.Optional(CONF_BOTTOM_TEXT, default=""): cv.string_strict,
    cv.Optional(CONF_ICON): ICON_SCHEMA,
    cv.Optional(CONF_VALUE_FONT): cv.use_id(font.Font),
    cv.Optional(CONF_TEXT_FONT): cv.use_id(font.Font),
    cv.Optional(CONF_ACCENT, default=""): cv.string_strict,
}

SENSOR_VALUE_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckSensorValue),
    cv.Required(CONF_VALUE): cv.templatable(cv.float_),
    **VALUE_DISPLAY_FIELDS,
})

WEATHER_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckWeather),
    cv.Required(CONF_TEMPERATURE): cv.templatable(cv.float_),
    cv.Required(CONF_CONDITION): cv.templatable(cv.string_strict),
    cv.Optional(CONF_IS_NIGHT, default=False): cv.templatable(cv.boolean),
    **{key: value for key, value in VALUE_DISPLAY_FIELDS.items() if key.schema != CONF_ICON},
})

def _validate_slider(value):
    if value[CONF_MIN_VALUE] >= value[CONF_MAX_VALUE]:
        raise cv.Invalid("'min_value' must be smaller than 'max_value'")
    if value[CONF_LABEL] and CONF_ICON in value:
        raise cv.Invalid("A slider action can use either 'label' or 'icon', not both")
    if CONF_ON_CLICK in value and not value[CONF_LABEL] and CONF_ICON not in value:
        raise cv.Invalid("A slider with 'on_click' requires 'label' or 'icon'")
    if value[CONF_LABEL] or CONF_ICON in value:
        available = value[CONF_HEIGHT] if value[CONF_ORIENTATION] == "vertical" else value[CONF_WIDTH]
        if value[CONF_ACTION_HEIGHT] >= available:
            raise cv.Invalid("'action_height' must be smaller than the slider's main-axis size")
    return value


SLIDER_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckSlider),
    cv.Required(CONF_X): cv.int_,
    cv.Required(CONF_Y): cv.int_,
    cv.Required(CONF_WIDTH): cv.positive_int,
    cv.Required(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
    cv.Optional(CONF_DISABLED, default=False): cv.templatable(cv.boolean),
    cv.Optional(CONF_VALUE, default=0): cv.templatable(cv.float_),
    cv.Optional(CONF_MIN_VALUE, default=0): cv.int_,
    cv.Optional(CONF_MAX_VALUE, default=100): cv.int_,
    cv.Optional(CONF_FORMAT, default="%.0f%%"): FLOAT_FORMAT,
    cv.Optional(CONF_LABEL, default=""): cv.string_strict,
    cv.Optional(CONF_FONT): cv.use_id(font.Font),
    cv.Optional(CONF_ICON): SLIDER_ICON_SCHEMA,
    cv.Optional(CONF_ACTION_HEIGHT, default=56): cv.positive_int,
    cv.Optional(CONF_ACCENT, default=""): cv.string_strict,
    cv.Optional(CONF_ORIENTATION, default="vertical"): cv.one_of("vertical", "horizontal", lower=True),
    cv.Optional(CONF_OPTIMISTIC, default=True): cv.boolean,
    cv.Optional(CONF_OPTIMISTIC_TIMEOUT, default="2s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_ON_VALUE): automation.validate_automation(single=True),
    cv.Optional(CONF_ON_RELEASE): automation.validate_automation(single=True),
    cv.Optional(CONF_ON_CLICK): automation.validate_automation(single=True),
}), _validate_slider)

CLIMATE_OPTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckClimateOption),
    cv.Optional(CONF_LABEL, default=""): cv.string_strict,
    cv.Optional(CONF_ICON): ICON_SCHEMA,
    cv.Optional(CONF_ACTIVE, default=False): cv.templatable(cv.boolean),
    cv.Optional(CONF_ACCENT, default=""): cv.string_strict,
    cv.Optional(CONF_ON_SELECT): automation.validate_automation(single=True),
})

CLIMATE_GROUP_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckClimateGroup),
    cv.Required(CONF_LABEL): cv.string_strict,
    cv.Required(CONF_OPTIONS): cv.All(cv.ensure_list(CLIMATE_OPTION_SCHEMA), cv.Length(min=1)),
})

CLIMATE_POWER_SCHEMA = cv.Schema({
    cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
    cv.Optional(CONF_STATE, default=False): cv.templatable(cv.boolean),
    cv.Optional(CONF_ICON, default="\U000F0425"): cv.string_strict,
    cv.Optional(CONF_ACCENT, default=""): cv.string_strict,
    cv.Optional(CONF_ON_TURN_ON): automation.validate_automation(single=True),
    cv.Optional(CONF_ON_TURN_OFF): automation.validate_automation(single=True),
})


def _validate_climate(value):
    if value[CONF_MIN_VALUE] >= value[CONF_MAX_VALUE]:
        raise cv.Invalid("'min_value' must be smaller than 'max_value'")
    if value[CONF_STEP] <= 0:
        raise cv.Invalid("'step' must be greater than zero")
    return value


CLIMATE_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckClimate),
    cv.Required(CONF_X): cv.int_,
    cv.Required(CONF_Y): cv.int_,
    cv.Required(CONF_WIDTH): cv.positive_int,
    cv.Required(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
    cv.Optional(CONF_DISABLED, default=False): cv.templatable(cv.boolean),
    cv.Required(CONF_CURRENT_TEMPERATURE): cv.templatable(cv.float_),
    cv.Required(CONF_TARGET_TEMPERATURE): cv.templatable(cv.float_),
    cv.Optional(CONF_MIN_VALUE, default=5.0): cv.float_,
    cv.Optional(CONF_MAX_VALUE, default=35.0): cv.float_,
    cv.Optional(CONF_STEP, default=0.5): cv.float_,
    cv.Optional(CONF_FORMAT, default="%.1f"): FLOAT_FORMAT,
    cv.Optional(CONF_UNITS, default="°C"): cv.string_strict,
    cv.Optional(CONF_ACCENT, default="climate"): cv.string_strict,
    cv.Optional(CONF_HEATING_ACCENT, default="heat"): cv.string_strict,
    cv.Optional(CONF_DRYING_ACCENT, default="dry"): cv.string_strict,
    cv.Optional(CONF_ARC_MODE, default="cooling"): cv.templatable(
        cv.one_of("cooling", "heating", "drying", lower=True)
    ),
    cv.Optional(CONF_OPTIMISTIC_TIMEOUT, default="2s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_PADDING): cv.int_range(min=0),
    cv.Optional(CONF_GAP): cv.int_range(min=0),
    cv.Optional(CONF_CONTROLS_HEIGHT): cv.positive_int,
    cv.Required(CONF_PRIMARY_CONTROLS): cv.All(
        cv.ensure_list(CLIMATE_OPTION_SCHEMA), cv.Length(min=1, max=3)
    ),
    cv.Optional(CONF_ADDITIONAL_CONTROLS, default=[]): cv.ensure_list(CLIMATE_GROUP_SCHEMA),
    cv.Optional(CONF_POWER, default={}): CLIMATE_POWER_SCHEMA,
    cv.Optional(CONF_MENU_ICON, default="\U000F01D9"): cv.string_strict,
    cv.Optional(CONF_CLOSE_ICON, default="\U000F0156"): cv.string_strict,
    cv.Optional(CONF_ON_TARGET_CHANGE): automation.validate_automation(single=True),
}), _validate_climate)


def _validate_navigation_button(value):
    if not value[CONF_BACK] and CONF_TARGET not in value:
        raise cv.Invalid("A navigation_button needs 'back: true' or 'target'")
    if value[CONF_BACK] and CONF_TARGET in value:
        raise cv.Invalid("Use either 'back: true' or 'target', not both")
    has_x = CONF_X in value
    has_y = CONF_Y in value
    if has_x != has_y:
        raise cv.Invalid("navigation_button requires both 'x' and 'y' when using absolute positioning")
    if CONF_ALIGN in value and has_x:
        raise cv.Invalid("Use either 'align' or 'x'/'y', not both")
    if CONF_ALIGN not in value and not has_x:
        value[CONF_ALIGN] = "top_left"
    return value


NAVIGATION_BUTTON_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckNavigationButton),
    cv.Optional(CONF_X): cv.int_,
    cv.Optional(CONF_Y): cv.int_,
    cv.Optional(CONF_WIDTH): cv.positive_int,
    cv.Optional(CONF_HEIGHT): cv.positive_int,
    cv.Optional(CONF_ALIGN): cv.one_of(*NAVIGATION_ALIGNMENTS, lower=True),
    cv.Optional(CONF_MARGIN): cv.int_range(min=0),
    cv.Optional(CONF_VISIBLE, default=True): cv.templatable(cv.boolean),
    cv.Optional(CONF_BACK, default=False): cv.boolean,
    cv.Optional(CONF_TARGET): cv.use_id(HaDeckScreen),
    cv.Optional(CONF_GLYPH, default="\U000F004D"): cv.string_strict,
    cv.Optional(CONF_ANIMATION, default="none"): cv.one_of(*ANIMATIONS, lower=True),
    cv.Optional(CONF_TIME, default="200ms"): cv.positive_time_period_milliseconds,
}), _validate_navigation_button)

WIDGET_SCHEMA = cv.Any(
    cv.Schema({cv.Required(CONF_BUTTON): BUTTON_SCHEMA}),
    cv.Schema({cv.Required(CONF_SENSOR_VALUE): SENSOR_VALUE_SCHEMA}),
    cv.Schema({cv.Required(CONF_WEATHER): WEATHER_SCHEMA}),
    cv.Schema({cv.Required(CONF_SLIDER): SLIDER_SCHEMA}),
    cv.Schema({cv.Required(CONF_CLIMATE): CLIMATE_SCHEMA}),
    cv.Schema({cv.Required(CONF_NAVIGATION_BUTTON): NAVIGATION_BUTTON_SCHEMA}),
)
SCREEN_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckScreen),
    cv.Optional(CONF_BACKGROUND_COLOR): cv.hex_uint32_t,
    cv.Optional(CONF_BACKGROUND_IMAGE): cv.use_id(image.Image_),
    cv.Required(CONF_WIDGETS): cv.ensure_list(WIDGET_SCHEMA),
})
ACCENT_SCHEMA = cv.Schema({
    cv.Required(CONF_NAME): cv.string_strict,
    cv.Required(CONF_COLOR): cv.hex_uint32_t,
    cv.Required(CONF_ON_COLOR): cv.hex_uint32_t,
})
THEME_FONTS_SCHEMA = cv.Schema({
    **{cv.Optional(key): cv.use_id(font.Font) for key in THEME_FONT_KEYS},
})
THEME_METRICS_SCHEMA = cv.Schema({
    cv.Optional(CONF_SPACING_SMALL): cv.int_range(min=0),
    cv.Optional(CONF_SPACING_MEDIUM): cv.int_range(min=0),
    cv.Optional(CONF_TOUCH_TARGET): cv.positive_int,
    cv.Optional(CONF_CONTROL_HEIGHT): cv.positive_int,
})
THEME_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HaDeckTheme),
    cv.Optional(CONF_BASE, default="dark"): cv.one_of("dark", "light", lower=True),
    **{cv.Optional(key): cv.hex_uint32_t for key in THEME_COLOR_KEYS},
    cv.Optional(CONF_BACKGROUND_COLOR): cv.hex_uint32_t,
    cv.Optional(CONF_BODY_FONT): cv.use_id(font.Font),
    cv.Optional(CONF_FONTS, default={}): THEME_FONTS_SCHEMA,
    cv.Optional(CONF_METRICS, default={}): THEME_METRICS_SCHEMA,
    cv.Optional(CONF_RADIUS): cv.int_range(min=0),
    cv.Optional(CONF_DISABLED_OPACITY): cv.percentage,
    cv.Optional(CONF_ACCENTS, default=[]): cv.ensure_list(ACCENT_SCHEMA),
})


def _register_lvgl_resources(config):
    add_lv_use("obj", "button", "flex", "label", "image", "bar", "slider", "arc")
    for theme in config[CONF_THEMES]:
        if font_id := theme.get(CONF_BODY_FONT):
            add_lv_use("font")
            get_esphome_fonts_used().add(font_id)
        for font_id in theme[CONF_FONTS].values():
            add_lv_use("font")
            get_esphome_fonts_used().add(font_id)
    for screen in config[CONF_SCREENS]:
        if image_id := screen.get(CONF_BACKGROUND_IMAGE):
            add_lv_use("image")
            get_lv_images_used().add(image_id)
        for widget in screen[CONF_WIDGETS]:
            if CONF_CLIMATE in widget:
                climate = widget[CONF_CLIMATE]
                for option in climate[CONF_PRIMARY_CONTROLS]:
                    icon = option.get(CONF_ICON)
                    if icon and CONF_IMAGE in icon:
                        add_lv_use("image")
                        get_lv_images_used().add(icon[CONF_IMAGE])
                    if icon and (font_id := icon.get(CONF_FONT)):
                        add_lv_use("font")
                        get_esphome_fonts_used().add(font_id)
                for group in climate[CONF_ADDITIONAL_CONTROLS]:
                    for option in group[CONF_OPTIONS]:
                        icon = option.get(CONF_ICON)
                        if icon and CONF_IMAGE in icon:
                            add_lv_use("image")
                            get_lv_images_used().add(icon[CONF_IMAGE])
                        if icon and (font_id := icon.get(CONF_FONT)):
                            add_lv_use("font")
                            get_esphome_fonts_used().add(font_id)
                continue
            if CONF_NAVIGATION_BUTTON in widget:
                add_lv_use("label")
                continue
            if CONF_WEATHER in widget:
                add_lv_use("image")
                item = widget[CONF_WEATHER]
            elif CONF_SENSOR_VALUE in widget:
                item = widget[CONF_SENSOR_VALUE]
            elif CONF_SLIDER in widget:
                slider = widget[CONF_SLIDER]
                if font_id := slider.get(CONF_FONT):
                    add_lv_use("font")
                    get_esphome_fonts_used().add(font_id)
                if icon := slider.get(CONF_ICON):
                    add_lv_use("label")
                    if font_id := icon.get(CONF_FONT):
                        add_lv_use("font")
                        get_esphome_fonts_used().add(font_id)
                continue
            else:
                item = widget[CONF_BUTTON]
            button = item
            if CONF_TEXT in button:
                add_lv_use("label")
            if font_id := button.get(CONF_FONT):
                add_lv_use("font")
                get_esphome_fonts_used().add(font_id)
            if icon := button.get(CONF_ICON):
                if image_id := icon.get(CONF_IMAGE):
                    add_lv_use("image")
                    get_lv_images_used().add(image_id)
                if font_id := icon.get(CONF_FONT):
                    add_lv_use("label", "font")
                    get_esphome_fonts_used().add(font_id)
    return config


def _validate_theme_font_defaults(config):
    uses_default_icon_font = False
    uses_default_small_icon_font = False
    for screen in config[CONF_SCREENS]:
        for widget in screen[CONF_WIDGETS]:
            if CONF_CLIMATE in widget:
                uses_default_small_icon_font = True  # menu, close and option glyphs
                uses_default_icon_font = True  # power control
                continue
            if CONF_NAVIGATION_BUTTON in widget:
                uses_default_small_icon_font = True
                continue
            item = widget.get(CONF_BUTTON) or widget.get(CONF_SENSOR_VALUE)
            if item is None and CONF_SLIDER in widget:
                slider_icon = widget[CONF_SLIDER].get(CONF_ICON)
                if slider_icon and CONF_FONT not in slider_icon:
                    uses_default_small_icon_font = True
                continue
            if item is None:
                continue
            icon = item.get(CONF_ICON)
            if icon and CONF_GLYPH in icon and CONF_FONT not in icon:
                uses_default_icon_font = True

    if uses_default_icon_font:
        missing = [
            str(theme[CONF_ID]) for theme in config[CONF_THEMES]
            if CONF_ICON_MEDIUM not in theme[CONF_FONTS]
        ]
        if missing:
            raise cv.Invalid(
                "Glyph icons without a local font require fonts.icon_medium in every theme; "
                f"missing in: {', '.join(missing)}"
            )
    if uses_default_small_icon_font:
        missing = [
            str(theme[CONF_ID]) for theme in config[CONF_THEMES]
            if CONF_ICON_SMALL not in theme[CONF_FONTS]
        ]
        if missing:
            raise cv.Invalid(
                "Slider icons without a local font require fonts.icon_small in every theme; "
                f"missing in: {', '.join(missing)}"
            )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HaDeck),
        cv.Required(CONF_LVGL_ID): cv.use_id(lvgl.LvglComponent),
        cv.Required(CONF_DEFAULT_SCREEN): cv.use_id(HaDeckScreen),
        cv.Required(CONF_DEFAULT_THEME): cv.use_id(HaDeckTheme),
        cv.Optional(CONF_WEATHER_ICON_SIZE): cv.int_range(min=8, max=128),
        cv.Optional(CONF_WEATHER_ICON_DIRECTORY): _validate_weather_icon_directory,
        cv.Required(CONF_SCREENS): cv.All(cv.ensure_list(SCREEN_SCHEMA), cv.Length(min=1)),
        cv.Required(CONF_THEMES): cv.All(cv.ensure_list(THEME_SCHEMA), cv.Length(min=1)),
    }).extend(cv.polling_component_schema("100ms")),
    _validate_theme_font_defaults,
    _register_lvgl_resources,
)


def _final_validate_weather_icon_size(config):
    has_weather = any(
        CONF_WEATHER in widget
        for screen in config[CONF_SCREENS]
        for widget in screen[CONF_WIDGETS]
    )
    if not has_weather or CONF_WEATHER_ICON_SIZE in config:
        return config

    global_config = full_config.get()
    theme_sizes = {}
    for theme in config[CONF_THEMES]:
        icon_font_id = theme[CONF_FONTS].get(CONF_ICON_MEDIUM)
        if icon_font_id is None:
            raise cv.Invalid(
                "Weather widgets derive their image size from fonts.icon_medium; "
                f"theme '{theme[CONF_ID]}' must define it or weather_icon_size must be set"
            )
        font_path = global_config.get_path_for_id(icon_font_id)[:-1]
        font_config = global_config.get_config_for_path(font_path)
        theme_sizes[str(theme[CONF_ID])] = font_config[CONF_SIZE]

    sizes = set(theme_sizes.values())
    if len(sizes) != 1:
        details = ", ".join(f"{name}: {size}" for name, size in theme_sizes.items())
        raise cv.Invalid(
            "Weather widgets use one shared image set, so fonts.icon_medium must "
            f"have the same size in every theme ({details}); set weather_icon_size "
            "to override this intentionally"
        )

    default_theme_name = str(config[CONF_DEFAULT_THEME])
    config[CONF_WEATHER_ICON_SIZE] = theme_sizes[default_theme_name]
    return config


FINAL_VALIDATE_SCHEMA = _final_validate_weather_icon_size


def _variant_expression(value):
    return cg.RawExpression(f"ha_deck::ButtonVariant::{value.upper()}")


async def _theme_to_code(conf):
    theme = cg.new_Pvariable(conf[CONF_ID])
    cg.add(theme.set_name(str(conf[CONF_ID])))
    cg.add(theme.set_base(conf[CONF_BASE] == "dark"))
    for key in (CONF_BACKGROUND_COLOR, *THEME_COLOR_KEYS):
        if key in conf:
            cg.add(getattr(theme, f"set_{key}")(conf[key]))
    if CONF_BODY_FONT in conf:
        font_var = await cg.get_variable(conf[CONF_BODY_FONT])
        cg.add(theme.set_body_font(font_var))
    for key, font_id in conf[CONF_FONTS].items():
        cg.add(getattr(theme, f"set_{key}_font")(await cg.get_variable(font_id)))
    for key, value in conf[CONF_METRICS].items():
        cg.add(getattr(theme, f"set_{key}")(value))
    if CONF_RADIUS in conf:
        cg.add(theme.set_radius(conf[CONF_RADIUS]))
    if CONF_DISABLED_OPACITY in conf:
        cg.add(theme.set_disabled_opacity(round(conf[CONF_DISABLED_OPACITY] * 255)))
    for accent in conf[CONF_ACCENTS]:
        cg.add(theme.add_accent(accent[CONF_NAME], accent[CONF_COLOR], accent[CONF_ON_COLOR]))
    return theme


async def _button_to_code(conf, screen):
    button = cg.new_Pvariable(conf[CONF_ID])
    cg.add(button.set_geometry(conf[CONF_X], conf[CONF_Y], conf[CONF_WIDTH], conf[CONF_HEIGHT]))
    cg.add(button.set_variant(_variant_expression(conf[CONF_VARIANT])))
    cg.add(button.set_accent(await cg.templatable(conf[CONF_ACCENT], [], cg.std_string)))
    cg.add(button.set_accent_icon(await cg.templatable(conf[CONF_ACCENT_ICON], [], cg.bool_)))
    cg.add(button.set_toggle(conf[CONF_TOGGLE]))
    visible = await cg.templatable(conf[CONF_VISIBLE], [], cg.bool_)
    cg.add(button.set_visible(visible))
    disabled = await cg.templatable(conf[CONF_DISABLED], [], cg.bool_)
    cg.add(button.set_disabled(disabled))
    if CONF_CHECKED in conf:
        checked = await cg.templatable(conf[CONF_CHECKED], [], cg.bool_)
        cg.add(button.set_checked(checked))
    if CONF_TEXT in conf:
        cg.add(button.set_text(conf[CONF_TEXT]))
    if CONF_FONT in conf:
        cg.add(button.set_font(await cg.get_variable(conf[CONF_FONT])))
    if CONF_ICON in conf:
        icon = conf[CONF_ICON]
        if CONF_IMAGE in icon:
            cg.add(button.set_icon(await cg.get_variable(icon[CONF_IMAGE])))
        else:
            cg.add(button.set_icon_glyph(icon[CONF_GLYPH]))
            if CONF_FONT in icon:
                cg.add(button.set_icon_font(await cg.get_variable(icon[CONF_FONT])))
    for key in (
        CONF_BACKGROUND_COLOR, CONF_TEXT_COLOR, CONF_BORDER_COLOR,
        CONF_BORDER_WIDTH, CONF_RADIUS,
    ):
        if key in conf:
            cg.add(getattr(button, f"set_{key}")(conf[key]))
    if CONF_DISABLED_OPACITY in conf:
        cg.add(button.set_disabled_opacity(round(conf[CONF_DISABLED_OPACITY] * 255)))
    if CONF_ON_CLICK in conf:
        await automation.build_automation(button, [], conf[CONF_ON_CLICK])
    if CONF_ON_PRESS in conf:
        await automation.build_automation(button.get_press_trigger(), [], conf[CONF_ON_PRESS])
    if CONF_ON_RELEASE in conf:
        await automation.build_automation(button.get_release_trigger(), [], conf[CONF_ON_RELEASE])
    if CONF_ON_LONG_PRESS in conf:
        await automation.build_automation(button.get_long_press_trigger(), [], conf[CONF_ON_LONG_PRESS])
    if CONF_ON_TURN_ON in conf:
        await automation.build_automation(button.get_turn_on_trigger(), [], conf[CONF_ON_TURN_ON])
    if CONF_ON_TURN_OFF in conf:
        await automation.build_automation(button.get_turn_off_trigger(), [], conf[CONF_ON_TURN_OFF])
    cg.add(screen.add_widget(button))


async def _configure_value_display(var, conf, value_key):
    cg.add(var.set_geometry(conf[CONF_X], conf[CONF_Y], conf[CONF_WIDTH], conf[CONF_HEIGHT]))
    cg.add(var.set_value(await cg.templatable(conf[value_key], [], cg.float_)))
    cg.add(var.set_visible(await cg.templatable(conf[CONF_VISIBLE], [], cg.bool_)))
    cg.add(var.set_format(conf[CONF_FORMAT]))
    cg.add(var.set_units(conf[CONF_UNITS]))
    cg.add(var.set_unavailable_text(conf[CONF_UNAVAILABLE_TEXT]))
    cg.add(var.set_top_text(conf[CONF_TOP_TEXT]))
    cg.add(var.set_bottom_text(conf[CONF_BOTTOM_TEXT]))
    cg.add(var.set_accent(conf[CONF_ACCENT]))
    if CONF_VALUE_FONT in conf:
        cg.add(var.set_value_font(await cg.get_variable(conf[CONF_VALUE_FONT])))
    if CONF_TEXT_FONT in conf:
        cg.add(var.set_text_font(await cg.get_variable(conf[CONF_TEXT_FONT])))
    if CONF_ICON in conf:
        icon = conf[CONF_ICON]
        if CONF_IMAGE in icon:
            cg.add(var.set_icon(await cg.get_variable(icon[CONF_IMAGE])))
        else:
            cg.add(var.set_icon_glyph(icon[CONF_GLYPH]))
            if CONF_FONT in icon:
                cg.add(var.set_icon_font(await cg.get_variable(icon[CONF_FONT])))


async def _sensor_value_to_code(conf, screen):
    var = cg.new_Pvariable(conf[CONF_ID])
    await _configure_value_display(var, conf, CONF_VALUE)
    cg.add(screen.add_widget(var))


async def _weather_to_code(conf, screen, weather_images):
    var = cg.new_Pvariable(conf[CONF_ID])
    await _configure_value_display(var, conf, CONF_TEMPERATURE)
    cg.add(var.set_condition(await cg.templatable(conf[CONF_CONDITION], [], cg.std_string)))
    cg.add(var.set_is_night(await cg.templatable(conf[CONF_IS_NIGHT], [], cg.bool_)))
    for condition, image_id in weather_images.items():
        cg.add(var.add_condition_image(condition, await cg.get_variable(image_id)))
    cg.add(screen.add_widget(var))


async def _slider_to_code(conf, screen):
    var = cg.new_Pvariable(conf[CONF_ID])
    cg.add(var.set_geometry(conf[CONF_X], conf[CONF_Y], conf[CONF_WIDTH], conf[CONF_HEIGHT]))
    cg.add(var.set_range(conf[CONF_MIN_VALUE], conf[CONF_MAX_VALUE]))
    cg.add(var.set_format(conf[CONF_FORMAT]))
    cg.add(var.set_value(await cg.templatable(conf[CONF_VALUE], [], cg.float_)))
    cg.add(var.set_visible(await cg.templatable(conf[CONF_VISIBLE], [], cg.bool_)))
    cg.add(var.set_disabled(await cg.templatable(conf[CONF_DISABLED], [], cg.bool_)))
    cg.add(var.set_label(conf[CONF_LABEL]))
    if CONF_FONT in conf:
        cg.add(var.set_font(await cg.get_variable(conf[CONF_FONT])))
    if CONF_ICON in conf:
        cg.add(var.set_icon_glyph(conf[CONF_ICON][CONF_GLYPH]))
        if CONF_FONT in conf[CONF_ICON]:
            cg.add(var.set_icon_font(await cg.get_variable(conf[CONF_ICON][CONF_FONT])))
    cg.add(var.set_action_height(conf[CONF_ACTION_HEIGHT]))
    cg.add(var.set_accent(conf[CONF_ACCENT]))
    cg.add(var.set_vertical(conf[CONF_ORIENTATION] == "vertical"))
    cg.add(var.set_optimistic(conf[CONF_OPTIMISTIC]))
    cg.add(var.set_optimistic_timeout(conf[CONF_OPTIMISTIC_TIMEOUT].total_milliseconds))
    if CONF_ON_VALUE in conf:
        await automation.build_automation(var.get_value_trigger(), [(cg.float_, "x")], conf[CONF_ON_VALUE])
    if CONF_ON_RELEASE in conf:
        await automation.build_automation(var.get_release_trigger(), [(cg.float_, "x")], conf[CONF_ON_RELEASE])
    if CONF_ON_CLICK in conf:
        await automation.build_automation(var.get_click_trigger(), [], conf[CONF_ON_CLICK])
    cg.add(screen.add_widget(var))


async def _climate_option_to_code(conf):
    option = cg.new_Pvariable(conf[CONF_ID])
    cg.add(option.set_label(conf[CONF_LABEL]))
    cg.add(option.set_active(await cg.templatable(conf[CONF_ACTIVE], [], cg.bool_)))
    cg.add(option.set_accent(conf[CONF_ACCENT]))
    if CONF_ICON in conf:
        icon = conf[CONF_ICON]
        if CONF_IMAGE in icon:
            cg.add(option.set_icon(await cg.get_variable(icon[CONF_IMAGE])))
        else:
            cg.add(option.set_icon_glyph(icon[CONF_GLYPH]))
            if CONF_FONT in icon:
                cg.add(option.set_icon_font(await cg.get_variable(icon[CONF_FONT])))
    if CONF_ON_SELECT in conf:
        await automation.build_automation(option, [], conf[CONF_ON_SELECT])
    return option


async def _climate_to_code(conf, screen):
    var = cg.new_Pvariable(conf[CONF_ID])
    cg.add(var.set_geometry(conf[CONF_X], conf[CONF_Y], conf[CONF_WIDTH], conf[CONF_HEIGHT]))
    cg.add(var.set_visible(await cg.templatable(conf[CONF_VISIBLE], [], cg.bool_)))
    cg.add(var.set_disabled(await cg.templatable(conf[CONF_DISABLED], [], cg.bool_)))
    cg.add(var.set_current_temperature(
        await cg.templatable(conf[CONF_CURRENT_TEMPERATURE], [], cg.float_)
    ))
    cg.add(var.set_target_temperature(
        await cg.templatable(conf[CONF_TARGET_TEMPERATURE], [], cg.float_)
    ))
    cg.add(var.set_range(conf[CONF_MIN_VALUE], conf[CONF_MAX_VALUE]))
    cg.add(var.set_step(conf[CONF_STEP]))
    cg.add(var.set_format(conf[CONF_FORMAT]))
    cg.add(var.set_units(conf[CONF_UNITS]))
    cg.add(var.set_accent(conf[CONF_ACCENT]))
    cg.add(var.set_heating_accent(conf[CONF_HEATING_ACCENT]))
    cg.add(var.set_drying_accent(conf[CONF_DRYING_ACCENT]))
    cg.add(var.set_arc_mode(await cg.templatable(conf[CONF_ARC_MODE], [], cg.std_string)))
    cg.add(var.set_optimistic_timeout(conf[CONF_OPTIMISTIC_TIMEOUT].total_milliseconds))
    for key in (CONF_PADDING, CONF_GAP, CONF_CONTROLS_HEIGHT):
        if key in conf:
            cg.add(getattr(var, f"set_{key}")(conf[key]))
    cg.add(var.set_menu_glyph(conf[CONF_MENU_ICON]))
    cg.add(var.set_close_glyph(conf[CONF_CLOSE_ICON]))
    for option_conf in conf[CONF_PRIMARY_CONTROLS]:
        cg.add(var.add_primary_option(await _climate_option_to_code(option_conf)))
    for group_conf in conf[CONF_ADDITIONAL_CONTROLS]:
        group = cg.new_Pvariable(group_conf[CONF_ID])
        cg.add(group.set_label(group_conf[CONF_LABEL]))
        for option_conf in group_conf[CONF_OPTIONS]:
            cg.add(group.add_option(await _climate_option_to_code(option_conf)))
        cg.add(var.add_additional_group(group))
    power = conf[CONF_POWER]
    cg.add(var.set_power_visible(await cg.templatable(power[CONF_VISIBLE], [], cg.bool_)))
    cg.add(var.set_power_state(await cg.templatable(power[CONF_STATE], [], cg.bool_)))
    cg.add(var.set_power_glyph(power[CONF_ICON]))
    cg.add(var.set_power_accent(power[CONF_ACCENT]))
    if CONF_ON_TURN_ON in power:
        await automation.build_automation(var.get_turn_on_trigger(), [], power[CONF_ON_TURN_ON])
    if CONF_ON_TURN_OFF in power:
        await automation.build_automation(var.get_turn_off_trigger(), [], power[CONF_ON_TURN_OFF])
    if CONF_ON_TARGET_CHANGE in conf:
        await automation.build_automation(
            var.get_target_change_trigger(), [(cg.float_, "x")], conf[CONF_ON_TARGET_CHANGE]
        )
    cg.add(screen.add_widget(var))


async def _navigation_button_to_code(conf, screen, deck):
    var = cg.new_Pvariable(conf[CONF_ID])
    cg.add(var.set_parent(deck))
    if CONF_X in conf:
        if CONF_WIDTH in conf or CONF_HEIGHT in conf:
            width = conf[CONF_WIDTH] if CONF_WIDTH in conf else conf[CONF_HEIGHT]
            height = conf[CONF_HEIGHT] if CONF_HEIGHT in conf else conf[CONF_WIDTH]
            cg.add(var.set_geometry(conf[CONF_X], conf[CONF_Y], width, height))
        else:
            cg.add(var.set_position(conf[CONF_X], conf[CONF_Y]))
    else:
        cg.add(var.set_alignment(cg.RawExpression(NAVIGATION_ALIGNMENTS[conf[CONF_ALIGN]])))
        if CONF_WIDTH in conf or CONF_HEIGHT in conf:
            width = conf.get(CONF_WIDTH, conf.get(CONF_HEIGHT, 56))
            height = conf.get(CONF_HEIGHT, conf.get(CONF_WIDTH, 56))
            cg.add(var.set_size(width, height))
        if CONF_MARGIN in conf:
            cg.add(var.set_margin(conf[CONF_MARGIN]))
    cg.add(var.set_visible(await cg.templatable(conf[CONF_VISIBLE], [], cg.bool_)))
    cg.add(var.set_back(conf[CONF_BACK]))
    cg.add(var.set_glyph(conf[CONF_GLYPH]))
    cg.add(var.set_animation(cg.RawExpression(ANIMATIONS[conf[CONF_ANIMATION]])))
    cg.add(var.set_time(conf[CONF_TIME].total_milliseconds))
    if CONF_TARGET in conf:
        cg.add(var.set_target(await cg.get_variable(conf[CONF_TARGET])))
    cg.add(screen.add_widget(var))


async def _generate_weather_images(icon_size, asset_dir):
    entries = []
    result = {}
    for condition in WEATHER_CONDITIONS:
        safe_name = condition.replace("-", "_")
        image_id = ID(f"ha_deck_weather_{safe_name}", is_declaration=True, type=image.Image_)
        raw_id = ID(f"ha_deck_weather_{safe_name}_raw", is_declaration=True, type=cg.uint8)
        entries.append({
            CONF_ID: image_id,
            CONF_RAW_DATA_ID: raw_id,
            CONF_FILE: str(asset_dir / f"{condition}.svg"),
            CONF_RESIZE: f"{icon_size}x{icon_size}",
            CONF_TYPE: "RGB565",
            image.CONF_TRANSPARENCY: image.CONF_ALPHA_CHANNEL,
            CONF_DITHER: "NONE",
            image.CONF_INVERT_ALPHA: False,
            image.CONF_BYTE_ORDER: "LITTLE_ENDIAN",
        })
        result[condition] = image_id
    if file_image is None:
        entries = image.CONFIG_SCHEMA(entries)
        entries = image.FINAL_VALIDATE_SCHEMA(entries)
        await image.to_code(entries)
    else:
        for entry in entries:
            entry = file_image.CONFIG_SCHEMA(entry)
            entry = file_image.FINAL_VALIDATE_SCHEMA(entry)
            await file_image.to_code(entry)
    return result


async def to_code(config):
    lvgl_var = await cg.get_variable(config[CONF_LVGL_ID])
    deck = cg.new_Pvariable(config[CONF_ID], lvgl_var)
    await cg.register_component(deck, config)

    has_weather = any(
        CONF_WEATHER in widget
        for screen in config[CONF_SCREENS]
        for widget in screen[CONF_WIDGETS]
    )
    weather_images = (
        await _generate_weather_images(
            config[CONF_WEATHER_ICON_SIZE],
            config.get(
                CONF_WEATHER_ICON_DIRECTORY,
                Path(__file__).parent / "assets" / "weather",
            ),
        )
        if has_weather
        else {}
    )

    for conf in config[CONF_THEMES]:
        cg.add(deck.add_theme(await _theme_to_code(conf)))

    screens = {}
    for conf in config[CONF_SCREENS]:
        screen = cg.new_Pvariable(conf[CONF_ID])
        cg.add(screen.set_name(str(conf[CONF_ID])))
        if CONF_BACKGROUND_COLOR in conf:
            cg.add(screen.set_background_color(conf[CONF_BACKGROUND_COLOR]))
        if CONF_BACKGROUND_IMAGE in conf:
            cg.add(screen.set_background_image(await cg.get_variable(conf[CONF_BACKGROUND_IMAGE])))
        cg.add(deck.add_screen(screen))
        screens[conf[CONF_ID]] = screen

    for conf in config[CONF_SCREENS]:
        for widget in conf[CONF_WIDGETS]:
            screen = screens[conf[CONF_ID]]
            if CONF_BUTTON in widget:
                await _button_to_code(widget[CONF_BUTTON], screen)
            elif CONF_SENSOR_VALUE in widget:
                await _sensor_value_to_code(widget[CONF_SENSOR_VALUE], screen)
            elif CONF_WEATHER in widget:
                await _weather_to_code(widget[CONF_WEATHER], screen, weather_images)
            elif CONF_SLIDER in widget:
                await _slider_to_code(widget[CONF_SLIDER], screen)
            elif CONF_CLIMATE in widget:
                await _climate_to_code(widget[CONF_CLIMATE], screen)
            else:
                await _navigation_button_to_code(widget[CONF_NAVIGATION_BUTTON], screen, deck)

    cg.add(deck.set_default_screen(await cg.get_variable(config[CONF_DEFAULT_SCREEN])))
    cg.add(deck.set_default_theme(await cg.get_variable(config[CONF_DEFAULT_THEME])))


SHOW_SCREEN_SCHEMA = cv.maybe_simple_value({
    cv.Required(CONF_ID): cv.use_id(HaDeckScreen),
    cv.Optional(CONF_ANIMATION, default="none"): cv.one_of(*ANIMATIONS, lower=True),
    cv.Optional(CONF_TIME, default="200ms"): cv.positive_time_period_milliseconds,
}, key=CONF_ID)


@automation.register_action(
    "ha_deck.screen.show", ShowScreenAction, SHOW_SCREEN_SCHEMA, synchronous=True
)
async def show_screen_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    cg.add(var.set_screen(await cg.get_variable(config[CONF_ID])))
    cg.add(var.set_animation(cg.RawExpression(ANIMATIONS[config[CONF_ANIMATION]])))
    cg.add(var.set_time(config[CONF_TIME].total_milliseconds))
    return var


@automation.register_action(
    "ha_deck.theme.set", SetThemeAction,
    cv.maybe_simple_value({cv.Required(CONF_ID): cv.use_id(HaDeckTheme)}, key=CONF_ID),
    synchronous=True,
)
async def set_theme_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    cg.add(var.set_theme(await cg.get_variable(config[CONF_ID])))
    return var

