#pragma once

#include <cstdint>
#include <string>

#include "widget.h"

namespace esphome::ha_deck {

class HaDeck;
class HaDeckScreen;

class HaDeckNavigationButton : public HaDeckWidget {
 public:
  void set_parent(HaDeck *parent) { this->parent_ = parent; }
  void set_geometry(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    this->x_ = x;
    this->y_ = y;
    this->width_ = width;
    this->height_ = height;
    this->has_position_ = true;
    this->has_size_ = true;
  }
  void set_position(int16_t x, int16_t y) {
    this->x_ = x;
    this->y_ = y;
    this->has_position_ = true;
  }
  void set_size(uint16_t width, uint16_t height) {
    this->width_ = width;
    this->height_ = height;
    this->has_size_ = true;
  }
  void set_alignment(lv_align_t alignment) {
    this->alignment_ = alignment;
    this->has_alignment_ = true;
  }
  void set_margin(uint16_t margin) {
    this->margin_ = margin;
    this->has_margin_ = true;
  }
  void set_target(HaDeckScreen *target) { this->target_ = target; }
  void set_back(bool back) { this->back_ = back; }
  void set_glyph(const std::string &glyph) { this->glyph_ = glyph; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme) override;
  void unmount() override;
  void apply_theme(HaDeckTheme *theme) override;

 protected:
  static void event_callback_(lv_event_t *event);
  void apply_geometry_(HaDeckTheme *theme);
  lv_obj_t *get_root_obj_() const override { return this->button_; }
  void update_state_(bool force) override {}

  HaDeck *parent_{nullptr};
  HaDeckScreen *target_{nullptr};
  int16_t x_{0};
  int16_t y_{0};
  uint16_t width_{56};
  uint16_t height_{56};
  std::string glyph_{"‹"};
  bool back_{false};
  bool has_position_{false};
  bool has_size_{false};
  bool has_alignment_{false};
  bool has_margin_{false};
  lv_align_t alignment_{LV_ALIGN_TOP_LEFT};
  uint16_t margin_{0};
  lv_obj_t *button_{nullptr};
  lv_obj_t *label_{nullptr};
};

}  // namespace esphome::ha_deck
