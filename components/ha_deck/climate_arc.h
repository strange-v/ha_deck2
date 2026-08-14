#pragma once

#include <cstdint>
#include <cmath>
#include <string>

#include <lvgl.h>

#include "theme.h"

namespace esphome::ha_deck {

// Layered, display-only climate dial inspired by Home Assistant's thermostat:
// neutral track, translucent target-to-end tail, saturated current-to-target
// range and independent markers. Every arc layer uses identical geometry.
class HaDeckClimateArc {
 public:
  void set_geometry(int16_t x, int16_t y, uint16_t size);
  void set_range(float minimum, float maximum, float step) {
    this->minimum_ = minimum;
    this->maximum_ = maximum;
    this->step_ = step;
  }
  void set_accent(const std::string &accent) { this->accent_ = accent; }
  void set_heating_accent(const std::string &accent) { this->heating_accent_ = accent; }
  void set_drying_accent(const std::string &accent) { this->drying_accent_ = accent; }

  void mount(lv_obj_t *parent, HaDeckTheme *theme);
  void unmount();
  void apply_theme(HaDeckTheme *theme);
  void update(float current, float target, bool active, bool heating, bool drying, bool force = false);

 protected:
  float clamp_(float value) const;
  int16_t angle_for_(float value) const;
  void position_marker_(lv_obj_t *marker, float value, uint16_t marker_size);

  int16_t x_{0};
  int16_t y_{0};
  uint16_t size_{0};
  float minimum_{5.0f};
  float maximum_{35.0f};
  float step_{0.5f};
  std::string accent_{};
  std::string heating_accent_{};
  std::string drying_accent_{};

  lv_obj_t *track_{nullptr};
  lv_obj_t *target_tail_{nullptr};
  lv_obj_t *active_range_{nullptr};
  lv_obj_t *current_marker_{nullptr};
  lv_obj_t *target_marker_{nullptr};
  HaDeckTheme *theme_{nullptr};
  float current_value_{NAN};
  float target_value_{NAN};
  bool active_{false};
  bool heating_{false};
  bool drying_{false};
  bool initialized_{false};
};

}  // namespace esphome::ha_deck
