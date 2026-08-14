#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/font/font.h"
#include "esphome/components/image/image.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"

#include "climate_arc.h"
#include "overlay.h"
#include "widget.h"

namespace esphome::ha_deck {

class HaDeckClimateOption : public Trigger<> {
 public:
  void set_label(const std::string &label) { this->label_ = label; }
  void set_icon(image::Image *icon) { this->icon_image_ = icon; }
  void set_icon_glyph(const std::string &glyph) { this->icon_glyph_ = glyph; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
  void set_active(TemplatableValue<bool> active) { this->active_ = active; }
  void set_accent(const std::string &accent) { this->accent_ = accent; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme, bool primary, uint16_t width = 0, uint16_t height = 0);
  void unmount();
  void apply_theme(HaDeckTheme *theme);
  void update(bool force = false);
  void set_disabled_state(bool disabled);
  void set_size(uint16_t width, uint16_t height);

 protected:
  static void event_callback_(lv_event_t *event);

  std::string label_{};
  std::string icon_glyph_{};
  std::string accent_{};
  image::Image *icon_image_{nullptr};
  font::Font *icon_font_{nullptr};
  TemplatableValue<bool> active_{false};
  lv_obj_t *button_{nullptr};
  lv_obj_t *icon_{nullptr};
  lv_obj_t *label_obj_{nullptr};
  bool active_state_{false};
  bool initialized_{false};
  bool disabled_state_{false};
  bool primary_{false};
};

class HaDeckClimateGroup {
 public:
  void set_label(const std::string &label) { this->label_ = label; }
  void add_option(HaDeckClimateOption *option) { this->options_.push_back(option); }
  const std::string &label() const { return this->label_; }
  const std::vector<HaDeckClimateOption *> &options() const { return this->options_; }

 protected:
  std::string label_{};
  std::vector<HaDeckClimateOption *> options_{};
};

class HaDeckClimate : public HaDeckWidget {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
  }
  void set_current_temperature(TemplatableValue<float> value) { this->current_temperature_ = value; }
  void set_target_temperature(TemplatableValue<float> value) { this->target_temperature_ = value; }
  void set_range(float min_value, float max_value) {
    this->min_temperature_ = min_value;
    this->max_temperature_ = max_value;
  }
  void set_step(float step) { this->step_ = step; }
  void set_format(const std::string &format) { this->format_ = format; }
  void set_units(const std::string &units) { this->units_ = units; }
  void set_accent(const std::string &accent) { this->accent_ = accent; }
  void set_heating_accent(const std::string &accent) { this->heating_accent_ = accent; }
  void set_drying_accent(const std::string &accent) { this->drying_accent_ = accent; }
  void set_arc_mode(TemplatableValue<std::string> mode) { this->arc_mode_ = mode; }
  void set_disabled(TemplatableValue<bool> disabled) { this->disabled_ = disabled; }
  void set_optimistic_timeout(uint32_t timeout) { this->optimistic_timeout_ = timeout; }
  void set_padding(uint16_t value) {
    this->padding_ = value;
    this->has_padding_ = true;
  }
  void set_gap(uint16_t value) {
    this->gap_ = value;
    this->has_gap_ = true;
  }
  void set_controls_height(uint16_t value) {
    this->controls_height_ = value;
    this->has_controls_height_ = true;
  }
  void add_primary_option(HaDeckClimateOption *option) { this->primary_options_.push_back(option); }
  void add_additional_group(HaDeckClimateGroup *group) { this->additional_groups_.push_back(group); }

  void set_power_visible(TemplatableValue<bool> visible) { this->power_visible_ = visible; }
  void set_power_state(TemplatableValue<bool> state) { this->power_state_ = state; }
  void set_power_accent(const std::string &accent) { this->power_accent_ = accent; }
  void set_power_glyph(const std::string &glyph) { this->power_glyph_ = glyph; }
  void set_menu_glyph(const std::string &glyph) { this->menu_glyph_ = glyph; }
  void set_close_glyph(const std::string &glyph) { this->close_glyph_ = glyph; }

  Trigger<float> *get_target_change_trigger() { return &this->target_change_trigger_; }
  Trigger<> *get_turn_on_trigger() { return &this->turn_on_trigger_; }
  Trigger<> *get_turn_off_trigger() { return &this->turn_off_trigger_; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;

 protected:
  static void event_callback_(lv_event_t *event);
  lv_obj_t *create_text_button_(lv_obj_t *parent, const char *text, int16_t x, int16_t y, uint16_t width,
                                uint16_t height);
  void build_overlay_(HaDeckTheme *theme);
  void layout_overlay_(HaDeckTheme *theme);
  void apply_layout_metrics_(HaDeckTheme *theme);
  void layout_content_();
  void align_units_baseline_();
  const std::string &mode_accent_(const std::string &mode) const;
  void apply_power_theme_();
  void request_target_(float value);
  lv_obj_t *get_root_obj_() const override { return this->root_; }
  void update_state_(bool force) override;

  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{0};
  uint16_t height_{0};
  float min_temperature_{5.0f};
  float max_temperature_{35.0f};
  float step_{0.5f};
  std::string format_{"%.1f"};
  std::string units_{"°C"};
  std::string accent_{};
  std::string heating_accent_{};
  std::string drying_accent_{};
  std::string power_accent_{};
  std::string power_glyph_{"P"};
  std::string menu_glyph_{"..."};
  std::string close_glyph_{"X"};
  TemplatableValue<float> current_temperature_{NAN};
  TemplatableValue<float> target_temperature_{NAN};
  TemplatableValue<bool> disabled_{false};
  TemplatableValue<bool> power_visible_{true};
  TemplatableValue<bool> power_state_{false};
  TemplatableValue<std::string> arc_mode_{std::string("cooling")};
  std::vector<HaDeckClimateOption *> primary_options_{};
  std::vector<HaDeckClimateGroup *> additional_groups_{};
  Trigger<float> target_change_trigger_{};
  Trigger<> turn_on_trigger_{};
  Trigger<> turn_off_trigger_{};

  lv_obj_t *root_{nullptr};
  lv_obj_t *top_area_{nullptr};
  lv_obj_t *bottom_controls_{nullptr};
  lv_obj_t *primary_controls_{nullptr};
  HaDeckClimateArc climate_arc_{};
  lv_obj_t *current_label_{nullptr};
  lv_obj_t *target_row_{nullptr};
  lv_obj_t *target_value_label_{nullptr};
  lv_obj_t *target_units_label_{nullptr};
  lv_obj_t *adjust_controls_{nullptr};
  lv_obj_t *minus_button_{nullptr};
  lv_obj_t *plus_button_{nullptr};
  lv_obj_t *power_button_{nullptr};
  lv_obj_t *power_label_{nullptr};
  lv_obj_t *menu_button_{nullptr};
  lv_obj_t *menu_label_{nullptr};
  lv_obj_t *overlay_close_button_{nullptr};
  std::vector<lv_obj_t *> overlay_titles_{};
  std::vector<lv_obj_t *> overlay_option_rows_{};
  HaDeckOverlay overlay_{};

  HaDeckTheme *theme_{nullptr};
  bool disabled_state_{false};
  bool power_state_value_{false};
  bool power_visible_state_{true};
  float displayed_target_{NAN};
  float displayed_current_{NAN};
  float pending_target_{NAN};
  uint32_t pending_since_{0};
  uint32_t optimistic_timeout_{2000};
  bool pending_{false};
  bool initialized_{false};
  std::string arc_mode_state_{"cooling"};
  uint16_t padding_{0};
  uint16_t gap_{0};
  uint16_t controls_height_{0};
  uint16_t resolved_padding_{8};
  uint16_t resolved_gap_{8};
  uint16_t resolved_controls_height_{88};
  uint16_t resolved_touch_target_{56};
  bool has_padding_{false};
  bool has_gap_{false};
  bool has_controls_height_{false};
};

}  // namespace esphome::ha_deck
