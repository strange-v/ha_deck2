#pragma once

#include <string>

#include <lvgl.h>

#include "theme.h"

namespace esphome::ha_deck {

static constexpr lv_opa_t GLASS_BACKGROUND_OPACITY = 64;
static constexpr lv_opa_t GLASS_INTERACTION_OPACITY = 90;

struct GlassButtonPalette {
  uint32_t foreground;
  uint32_t checked_background;
  uint32_t checked_foreground;
};

inline GlassButtonPalette apply_glass_button_surface(lv_obj_t *button, HaDeckTheme *theme,
                                                      const std::string &accent,
                                                      uint8_t disabled_opacity) {
  constexpr lv_style_selector_t main_default =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
  constexpr lv_style_selector_t main_pressed =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED);
  constexpr lv_style_selector_t main_disabled =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DISABLED);
  constexpr lv_style_selector_t main_checked =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
  constexpr lv_style_selector_t main_checked_pressed = static_cast<lv_style_selector_t>(LV_PART_MAIN) |
                                                        static_cast<lv_style_selector_t>(LV_STATE_CHECKED) |
                                                        static_cast<lv_style_selector_t>(LV_STATE_PRESSED);

  uint32_t accent_color;
  uint32_t on_accent_color;
  theme->resolve_accent(accent, &accent_color, &on_accent_color);

  lv_obj_set_style_bg_color(button, lv_color_hex(theme->on_surface_color()), main_default);
  lv_obj_set_style_bg_opa(button, GLASS_BACKGROUND_OPACITY, main_default);
  lv_obj_set_style_border_width(button, 0, main_default);
  lv_obj_set_style_border_color(button, lv_color_hex(theme->outline_color()), main_default);
  lv_obj_set_style_radius(button, theme->radius(), main_default);
  lv_obj_set_style_shadow_width(button, 0, main_default);
  lv_obj_set_style_opa(button, LV_OPA_COVER, main_default);

  lv_obj_set_style_bg_color(button, lv_color_hex(theme->on_surface_color()), main_pressed);
  lv_obj_set_style_bg_opa(button, GLASS_INTERACTION_OPACITY, main_pressed);
  lv_obj_set_style_opa(button, LV_OPA_COVER, main_pressed);

  lv_obj_set_style_opa(button, disabled_opacity, main_disabled);

  lv_obj_set_style_bg_color(button, lv_color_hex(accent_color), main_checked);
  lv_obj_set_style_bg_opa(button, GLASS_INTERACTION_OPACITY, main_checked);
  lv_obj_set_style_border_color(button, lv_color_hex(accent_color), main_checked);
  lv_obj_set_style_bg_color(button, lv_color_hex(accent_color), main_checked_pressed);
  lv_obj_set_style_bg_opa(button, GLASS_INTERACTION_OPACITY, main_checked_pressed);

  return {theme->on_surface_color(), accent_color, theme->on_surface_color()};
}

inline void apply_glass_text_content(lv_obj_t *content, const GlassButtonPalette &palette) {
  if (content == nullptr)
    return;
  constexpr lv_style_selector_t main_default =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
  constexpr lv_style_selector_t main_checked =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
  lv_obj_set_style_text_color(content, lv_color_hex(palette.foreground), main_default);
  lv_obj_set_style_text_color(content, lv_color_hex(palette.checked_foreground), main_checked);
}

inline void apply_glass_image_content(lv_obj_t *content, const GlassButtonPalette &palette) {
  if (content == nullptr)
    return;
#ifdef USE_LVGL_IMAGE
  constexpr lv_style_selector_t main_default =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
  constexpr lv_style_selector_t main_checked =
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
  lv_obj_set_style_image_recolor(content, lv_color_hex(palette.foreground), main_default);
  lv_obj_set_style_image_recolor_opa(content, LV_OPA_COVER, main_default);
  lv_obj_set_style_image_recolor(content, lv_color_hex(palette.checked_foreground), main_checked);
#endif
}

}  // namespace esphome::ha_deck
