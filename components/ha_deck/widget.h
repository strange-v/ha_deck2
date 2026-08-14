#pragma once

#include <lvgl.h>

#include "esphome/core/automation.h"

#include "theme.h"

namespace esphome::ha_deck {

class HaDeckWidget {
 public:
  virtual ~HaDeckWidget() = default;
  void set_visible(TemplatableValue<bool> visible) { this->visible_ = visible; }

  virtual void mount(lv_obj_t *parent, HaDeckTheme *theme) = 0;
  virtual void unmount() = 0;
  virtual void apply_theme(HaDeckTheme *theme) = 0;

  void update(bool force = false) {
    auto *obj = this->get_root_obj_();
    if (obj == nullptr)
      return;

    const bool visible = this->visible_.value();
    if (force || !this->visibility_initialized_ || visible != this->visible_state_) {
      this->visibility_initialized_ = true;
      this->visible_state_ = visible;
      if (visible)
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
      else
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    this->update_state_(force);
  }

 protected:
  virtual lv_obj_t *get_root_obj_() const = 0;
  virtual void update_state_(bool force) = 0;
  void reset_widget_state_() { this->visibility_initialized_ = false; }

  TemplatableValue<bool> visible_{true};
  bool visible_state_{true};
  bool visibility_initialized_{false};
};

}  // namespace esphome::ha_deck

