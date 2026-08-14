#pragma once

#include <cstdint>
#include <string>

#include "esphome/components/font/font.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"

#include "widget.h"

namespace esphome::ha_deck {

class HaDeckSlider : public HaDeckWidget {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
  }
  void set_range(int32_t min_value, int32_t max_value) {
    this->min_value_ = min_value;
    this->max_value_ = max_value;
  }
  void set_value(TemplatableValue<float> value) { this->value_ = value; }
  void set_format(const std::string &format) { this->format_ = format; }
  void set_disabled(TemplatableValue<bool> disabled) { this->disabled_ = disabled; }
  void set_label(const std::string &label) { this->label_text_ = label; }
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_glyph(const std::string &glyph) { this->icon_glyph_ = glyph; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
  void set_action_height(uint16_t height) { this->action_height_ = height; }
  void set_accent(const std::string &accent) { this->accent_ = accent; }
  void set_vertical(bool vertical) { this->vertical_ = vertical; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_optimistic_timeout(uint32_t timeout) { this->optimistic_timeout_ = timeout; }
  Trigger<float> *get_value_trigger() { return &this->value_trigger_; }
  Trigger<float> *get_release_trigger() { return &this->release_trigger_; }
  Trigger<> *get_click_trigger() { return &this->click_trigger_; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;

 protected:
  static void slider_event_callback_(lv_event_t *event);
  static void button_event_callback_(lv_event_t *event);
  lv_obj_t *get_root_obj_() const override { return this->root_; }
  void update_state_(bool force) override;
  void update_label_(bool show_value);

  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{0};
  uint16_t height_{0};
  int32_t min_value_{0};
  int32_t max_value_{100};
  TemplatableValue<float> value_{0.0f};
  TemplatableValue<bool> disabled_{false};
  std::string label_text_{};
  std::string format_{"%.0f%%"};
  std::string icon_glyph_{};
  std::string accent_{};
  font::Font *font_{nullptr};
  font::Font *icon_font_{nullptr};
  const lv_font_t *action_text_lv_font_{nullptr};
  const lv_font_t *action_icon_lv_font_{nullptr};
  uint16_t action_height_{56};
  bool vertical_{true};
  bool optimistic_{true};
  uint32_t optimistic_timeout_{2000};
  bool pending_{false};
  int32_t pending_value_{0};
  uint32_t pending_since_{0};
  bool disabled_state_{false};
  bool state_initialized_{false};
  int32_t rendered_value_{0};

  lv_obj_t *root_{nullptr};
  lv_obj_t *slider_{nullptr};
  lv_obj_t *button_{nullptr};
  lv_obj_t *label_{nullptr};
  Trigger<float> value_trigger_{};
  Trigger<float> release_trigger_{};
  Trigger<> click_trigger_{};
};

}  // namespace esphome::ha_deck
