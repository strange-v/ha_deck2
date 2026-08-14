#pragma once

#include <lvgl.h>

#include "theme.h"

namespace esphome::ha_deck {

// A reusable modal surface. It owns only the backdrop/panel lifecycle; callers
// are free to populate content_obj() with any LVGL controls they need.
class HaDeckOverlay {
 public:
  void mount(lv_obj_t *parent, HaDeckTheme *theme);
  void unmount();
  void apply_theme(HaDeckTheme *theme);
  void open();
  void close();

  lv_obj_t *content_obj() const { return this->content_; }
  bool is_open() const { return this->open_; }

 protected:
  static void backdrop_event_callback_(lv_event_t *event);

  lv_obj_t *root_{nullptr};
  lv_obj_t *panel_{nullptr};
  lv_obj_t *content_{nullptr};
  bool open_{false};
};

}  // namespace esphome::ha_deck
