#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/font/font.h"

namespace esphome::ha_deck {

class HaDeck;

class HaDeckTheme {
 public:
  struct Accent {
    std::string name;
    uint32_t color;
    uint32_t on_color;
  };

  void set_name(const std::string &name) { this->name_ = name; }
  void set_parent(HaDeck *parent) { this->parent_ = parent; }
  void activate();
  const std::string &get_name() const { return this->name_; }

  void set_base(bool dark) {
    this->dark_ = dark;
    if (dark) {
      this->background_color_ = 0x111318;
      this->surface_color_ = 0x111318;
      this->surface_container_color_ = 0x1D2026;
      this->primary_color_ = 0xA8C7FA;
      this->on_primary_color_ = 0x062E6F;
      this->primary_container_color_ = 0x284777;
      this->on_primary_container_color_ = 0xD7E3FF;
      this->error_color_ = 0xFFB4AB;
      this->on_error_color_ = 0x690005;
      this->on_surface_color_ = 0xE2E2E9;
      this->outline_color_ = 0x8C9199;
    } else {
      this->background_color_ = 0xF9F9FF;
      this->surface_color_ = 0xF9F9FF;
      this->surface_container_color_ = 0xEDEDF4;
      this->primary_color_ = 0x0B57D0;
      this->on_primary_color_ = 0xFFFFFF;
      this->primary_container_color_ = 0xD7E3FF;
      this->on_primary_container_color_ = 0x001B3F;
      this->error_color_ = 0xBA1A1A;
      this->on_error_color_ = 0xFFFFFF;
      this->on_surface_color_ = 0x1A1B20;
      this->outline_color_ = 0x74777F;
    }
  }

  void set_background_color(uint32_t value) { this->background_color_ = value; }
  void set_surface_color(uint32_t value) { this->surface_color_ = value; }
  void set_surface_container_color(uint32_t value) { this->surface_container_color_ = value; }
  void set_primary_color(uint32_t value) { this->primary_color_ = value; }
  void set_on_primary_color(uint32_t value) { this->on_primary_color_ = value; }
  void set_primary_container_color(uint32_t value) { this->primary_container_color_ = value; }
  void set_on_primary_container_color(uint32_t value) { this->on_primary_container_color_ = value; }
  void set_error_color(uint32_t value) { this->error_color_ = value; }
  void set_on_error_color(uint32_t value) { this->on_error_color_ = value; }
  void set_on_surface_color(uint32_t value) { this->on_surface_color_ = value; }
  void set_outline_color(uint32_t value) { this->outline_color_ = value; }
  void set_body_font(font::Font *font) { this->body_font_ = font; }
  void set_text_small_font(font::Font *font) { this->text_small_font_ = font; }
  void set_text_medium_font(font::Font *font) { this->text_medium_font_ = font; }
  void set_text_large_font(font::Font *font) { this->text_large_font_ = font; }
  void set_icon_small_font(font::Font *font) { this->icon_small_font_ = font; }
  void set_icon_medium_font(font::Font *font) { this->icon_medium_font_ = font; }
  void set_radius(uint16_t radius) { this->radius_ = radius; }
  void set_disabled_opacity(uint8_t opacity) { this->disabled_opacity_ = opacity; }
  void set_spacing_small(uint16_t value) { this->spacing_small_ = value; }
  void set_spacing_medium(uint16_t value) { this->spacing_medium_ = value; }
  void set_touch_target(uint16_t value) { this->touch_target_ = value; }
  void set_control_height(uint16_t value) { this->control_height_ = value; }
  void add_accent(const std::string &name, uint32_t color, uint32_t on_color) {
    for (auto &accent : this->accents_) {
      if (accent.name == name) {
        accent.color = color;
        accent.on_color = on_color;
        return;
      }
    }
    this->accents_.push_back({name, color, on_color});
  }
  bool resolve_accent(const std::string &name, uint32_t *color, uint32_t *on_color) const {
    if (!name.empty()) {
      for (const auto &accent : this->accents_) {
        if (accent.name == name) {
          *color = accent.color;
          *on_color = accent.on_color;
          return true;
        }
      }
      if (name == "neutral") {
        *color = this->on_surface_color_;
        *on_color = this->background_color_;
        return true;
      }
    }
    *color = this->primary_color_;
    *on_color = this->on_primary_color_;
    return false;
  }

  uint32_t background_color() const { return this->background_color_; }
  uint32_t surface_color() const { return this->surface_color_; }
  uint32_t surface_container_color() const { return this->surface_container_color_; }
  uint32_t primary_color() const { return this->primary_color_; }
  uint32_t on_primary_color() const { return this->on_primary_color_; }
  uint32_t primary_container_color() const { return this->primary_container_color_; }
  uint32_t on_primary_container_color() const { return this->on_primary_container_color_; }
  uint32_t error_color() const { return this->error_color_; }
  uint32_t on_error_color() const { return this->on_error_color_; }
  uint32_t on_surface_color() const { return this->on_surface_color_; }
  uint32_t outline_color() const { return this->outline_color_; }
  font::Font *body_font() const { return this->body_font_; }
  font::Font *text_small_font() const {
    return this->text_small_font_ != nullptr ? this->text_small_font_ : this->body_font_;
  }
  font::Font *text_medium_font() const {
    return this->text_medium_font_ != nullptr ? this->text_medium_font_ : this->text_small_font();
  }
  font::Font *text_large_font() const {
    return this->text_large_font_ != nullptr ? this->text_large_font_ : this->text_medium_font();
  }
  font::Font *icon_small_font() const {
    return this->icon_small_font_ != nullptr ? this->icon_small_font_ : this->icon_medium_font_;
  }
  font::Font *icon_medium_font() const {
    return this->icon_medium_font_ != nullptr ? this->icon_medium_font_ : this->icon_small_font_;
  }
  uint16_t radius() const { return this->radius_; }
  uint8_t disabled_opacity() const { return this->disabled_opacity_; }
  uint16_t spacing_small() const { return this->spacing_small_; }
  uint16_t spacing_medium() const { return this->spacing_medium_; }
  uint16_t touch_target() const { return this->touch_target_; }
  uint16_t control_height() const { return this->control_height_; }
  bool is_dark() const { return this->dark_; }

 protected:
  HaDeck *parent_{nullptr};
  std::string name_{};
  bool dark_{true};
  uint32_t background_color_{0x111318};
  uint32_t surface_color_{0x111318};
  uint32_t surface_container_color_{0x1D2026};
  uint32_t primary_color_{0xA8C7FA};
  uint32_t on_primary_color_{0x062E6F};
  uint32_t primary_container_color_{0x284777};
  uint32_t on_primary_container_color_{0xD7E3FF};
  uint32_t error_color_{0xFFB4AB};
  uint32_t on_error_color_{0x690005};
  uint32_t on_surface_color_{0xE2E2E9};
  uint32_t outline_color_{0x8C9199};
  font::Font *body_font_{nullptr};
  font::Font *text_small_font_{nullptr};
  font::Font *text_medium_font_{nullptr};
  font::Font *text_large_font_{nullptr};
  font::Font *icon_small_font_{nullptr};
  font::Font *icon_medium_font_{nullptr};
  uint16_t radius_{5};
  uint8_t disabled_opacity_{97};
  uint16_t spacing_small_{8};
  uint16_t spacing_medium_{16};
  uint16_t touch_target_{56};
  uint16_t control_height_{88};
  std::vector<Accent> accents_{{"light", 0xFCD663, 0x211A0A},
                                {"climate", 0x2196F3, 0x001D36},
                                {"heat", 0xFF6F22, 0x3D1200},
                                {"dry", 0x26A69A, 0x00201C}};
};

}  // namespace esphome::ha_deck
