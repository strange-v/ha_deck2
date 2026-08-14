#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "esphome/components/font/font.h"
#include "esphome/components/image/image.h"
#include "esphome/core/automation.h"

#include "widget.h"

namespace esphome::ha_deck {

class ValueDisplayWidget : public HaDeckWidget {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
  }
  void set_value(TemplatableValue<float> value) { this->value_ = value; }
  void set_format(const std::string &format) { this->format_ = format; }
  void set_units(const std::string &units) { this->units_ = units; }
  void set_unavailable_text(const std::string &text) { this->unavailable_text_ = text; }
  void set_top_text(const std::string &text) { this->top_text_ = text; }
  void set_bottom_text(const std::string &text) { this->bottom_text_ = text; }
  void set_icon(image::Image *icon) { this->icon_image_ = icon; }
  void set_icon_glyph(const std::string &glyph) { this->icon_glyph_ = glyph; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
  void set_value_font(font::Font *font) { this->value_font_ = font; }
  void set_text_font(font::Font *font) { this->text_font_ = font; }
  void set_accent(const std::string &accent) { this->accent_ = accent; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;

 protected:
  lv_obj_t *get_root_obj_() const override { return this->root_; }
  void update_state_(bool force) override;
  virtual void update_extra_(bool force) {}
  void set_dynamic_image_(image::Image *image);
  void update_icon_slot_();
  void align_units_baseline_();

  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{0};
  uint16_t height_{0};
  TemplatableValue<float> value_{NAN};
  std::string format_{"%.1f"};
  std::string units_{};
  std::string unavailable_text_{"−"};
  std::string top_text_{};
  std::string bottom_text_{};
  std::string icon_glyph_{};
  std::string accent_{};
  image::Image *icon_image_{nullptr};
  font::Font *icon_font_{nullptr};
  font::Font *value_font_{nullptr};
  font::Font *text_font_{nullptr};

  lv_obj_t *root_{nullptr};
  lv_obj_t *top_label_{nullptr};
  lv_obj_t *row_{nullptr};
  lv_obj_t *icon_{nullptr};
  lv_obj_t *value_row_{nullptr};
  lv_obj_t *value_label_{nullptr};
  lv_obj_t *units_label_{nullptr};
  lv_obj_t *bottom_label_{nullptr};
  float rendered_value_{NAN};
  bool value_initialized_{false};
};

class HaDeckSensorValue : public ValueDisplayWidget {};

}  // namespace esphome::ha_deck
