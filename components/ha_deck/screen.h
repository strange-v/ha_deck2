#pragma once

#include <string>
#include <vector>

#include <lvgl.h>

#include "esphome/components/image/image.h"

#include "theme.h"
#include "widget.h"

namespace esphome::ha_deck {

class HaDeck;

class HaDeckScreen {
 public:
  void set_name(const std::string &name) { this->name_ = name; }
  const std::string &get_name() const { return this->name_; }
  void set_parent(HaDeck *parent) { this->parent_ = parent; }
  void add_widget(HaDeckWidget *widget) { this->widgets_.push_back(widget); }
  void set_background_color(uint32_t color) {
    this->background_color_ = color;
    this->has_background_color_ = true;
  }
  void set_background_image(image::Image *image) { this->background_image_ = image; }

  void mount(HaDeckTheme *theme);
  void unmount();
  void update(bool force = false);
  void apply_theme(HaDeckTheme *theme);
  void show(lv_screen_load_anim_t animation = LV_SCREEN_LOAD_ANIM_NONE, uint32_t time = 0);

  lv_obj_t *get_obj() const { return this->obj_; }
  bool is_mounted() const { return this->obj_ != nullptr; }
  size_t widget_count() const { return this->widgets_.size(); }

 protected:
  HaDeck *parent_{nullptr};
  std::string name_{};
  std::vector<HaDeckWidget *> widgets_{};
  lv_obj_t *obj_{nullptr};
  image::Image *background_image_{nullptr};
  uint32_t background_color_{0};
  bool has_background_color_{false};
};

}  // namespace esphome::ha_deck

