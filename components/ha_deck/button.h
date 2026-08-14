#pragma once

#include <cstdint>
#include <string>

#include "esphome/components/font/font.h"
#include "esphome/components/image/image.h"
#include "esphome/core/automation.h"

#include "widget.h"
#include "button_style.h"

namespace esphome::ha_deck {

enum class ButtonVariant : uint8_t {
  FILLED,
  GLASS,
};

class HaDeckButton : public HaDeckWidget, public Trigger<> {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
  }

  void set_text(const std::string &text) { this->text_ = text; }
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon(image::Image *icon) { this->icon_image_ = icon; }
  void set_icon_glyph(const std::string &glyph) { this->icon_glyph_ = glyph; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
  void set_variant(ButtonVariant variant) { this->variant_ = variant; }
  void set_accent(TemplatableValue<std::string> accent) { this->accent_ = accent; }
  void set_accent_icon(TemplatableValue<bool> accent_icon) { this->accent_icon_ = accent_icon; }
  void set_disabled(TemplatableValue<bool> disabled) { this->disabled_ = disabled; }
  void set_toggle(bool toggle) { this->toggle_ = toggle; }
  void set_checked(TemplatableValue<bool> checked) {
    this->checked_ = checked;
    this->has_checked_source_ = true;
  }
  Trigger<> *get_turn_on_trigger() { return &this->turn_on_trigger_; }
  Trigger<> *get_turn_off_trigger() { return &this->turn_off_trigger_; }
  Trigger<> *get_press_trigger() { return &this->press_trigger_; }
  Trigger<> *get_release_trigger() { return &this->release_trigger_; }
  Trigger<> *get_long_press_trigger() { return &this->long_press_trigger_; }

  void set_background_color(uint32_t value) {
    this->background_color_ = value;
    this->has_background_color_ = true;
  }
  void set_text_color(uint32_t value) {
    this->text_color_ = value;
    this->has_text_color_ = true;
  }
  void set_border_color(uint32_t value) {
    this->border_color_ = value;
    this->has_border_color_ = true;
  }
  void set_border_width(uint16_t value) {
    this->border_width_ = value;
    this->has_border_width_ = true;
  }
  void set_radius(uint16_t value) {
    this->radius_ = value;
    this->has_radius_ = true;
  }
  void set_disabled_opacity(uint8_t value) {
    this->disabled_opacity_ = value;
    this->has_disabled_opacity_ = true;
  }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;

 protected:
  static void event_callback_(lv_event_t *event);
  void set_content_checked_(bool checked);
  void update_icon_accent_state_();
  lv_obj_t *get_root_obj_() const override { return this->button_; }
  void update_state_(bool force) override;

  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{0};
  uint16_t height_{0};
  std::string text_{};
  std::string icon_glyph_{};
  TemplatableValue<std::string> accent_{std::string{}};
  font::Font *font_{nullptr};
  font::Font *icon_font_{nullptr};
  image::Image *icon_image_{nullptr};
  ButtonVariant variant_{ButtonVariant::GLASS};
  TemplatableValue<bool> disabled_{false};
  TemplatableValue<bool> checked_{false};
  TemplatableValue<bool> accent_icon_{false};
  Trigger<> turn_on_trigger_{};
  Trigger<> turn_off_trigger_{};
  Trigger<> press_trigger_{};
  Trigger<> release_trigger_{};
  Trigger<> long_press_trigger_{};
  bool toggle_{false};
  bool has_checked_source_{false};
  bool long_press_fired_{false};

  uint32_t background_color_{0};
  uint32_t text_color_{0};
  uint32_t border_color_{0};
  uint16_t border_width_{0};
  uint16_t radius_{0};
  uint8_t disabled_opacity_{0};
  bool has_background_color_{false};
  bool has_text_color_{false};
  bool has_border_color_{false};
  bool has_border_width_{false};
  bool has_radius_{false};
  bool has_disabled_opacity_{false};

  bool disabled_state_{false};
  bool checked_state_{false};
  bool accent_icon_state_{false};
  std::string accent_state_{};
  bool initialized_state_{false};
  HaDeckTheme *theme_{nullptr};
  lv_obj_t *button_{nullptr};
  lv_obj_t *icon_content_{nullptr};
  lv_obj_t *label_{nullptr};
};

}  // namespace esphome::ha_deck

