#include "climate_arc.h"

#include <algorithm>
#include <cmath>

#include "numeric_utils.h"

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
static constexpr int16_t START_ANGLE = 135;
static constexpr int16_t SWEEP_ANGLE = 270;
static constexpr uint16_t TRACK_WIDTH = 24;
static constexpr uint16_t CURRENT_MARKER_SIZE = 10;
static constexpr uint16_t TARGET_MARKER_SIZE = TRACK_WIDTH;

void HaDeckClimateArc::set_geometry(int16_t x, int16_t y, uint16_t size) {
  this->x_ = x;
  this->y_ = y;
  this->size_ = size;
  for (auto *arc : {this->track_, this->target_tail_, this->active_range_}) {
    if (arc == nullptr)
      continue;
    lv_obj_set_pos(arc, x, y);
    lv_obj_set_size(arc, size, size);
  }
  if (this->initialized_) {
    this->position_marker_(this->current_marker_, this->current_value_, CURRENT_MARKER_SIZE);
    this->position_marker_(this->target_marker_, this->target_value_, TARGET_MARKER_SIZE);
  }
}

void HaDeckClimateArc::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->track_ = lv_arc_create(parent);
  this->target_tail_ = lv_arc_create(parent);
  this->active_range_ = lv_arc_create(parent);
  for (auto *arc : {this->track_, this->target_tail_, this->active_range_}) {
    lv_obj_set_pos(arc, this->x_, this->y_);
    lv_obj_set_size(arc, this->size_, this->size_);
    lv_obj_set_style_pad_all(arc, 0, MAIN_DEFAULT);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
  }

  lv_arc_set_bg_angles(this->track_, START_ANGLE, (START_ANGLE + SWEEP_ANGLE) % 360);

  this->current_marker_ = lv_obj_create(parent);
  this->target_marker_ = lv_obj_create(parent);
  for (auto *marker : {this->current_marker_, this->target_marker_}) {
    lv_obj_remove_flag(marker, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(marker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(marker, 0, MAIN_DEFAULT);
    lv_obj_set_style_border_width(marker, 0, MAIN_DEFAULT);
    lv_obj_set_style_shadow_width(marker, 0, MAIN_DEFAULT);
    lv_obj_set_style_radius(marker, LV_RADIUS_CIRCLE, MAIN_DEFAULT);
  }
  lv_obj_set_size(this->current_marker_, CURRENT_MARKER_SIZE, CURRENT_MARKER_SIZE);
  lv_obj_set_size(this->target_marker_, TARGET_MARKER_SIZE, TARGET_MARKER_SIZE);

  this->apply_theme(theme);
  this->initialized_ = false;
}

void HaDeckClimateArc::unmount() {
  this->track_ = nullptr;
  this->target_tail_ = nullptr;
  this->active_range_ = nullptr;
  this->current_marker_ = nullptr;
  this->target_marker_ = nullptr;
  this->theme_ = nullptr;
  this->initialized_ = false;
}

void HaDeckClimateArc::apply_theme(HaDeckTheme *theme) {
  if (this->track_ == nullptr || theme == nullptr)
    return;
  this->theme_ = theme;
  uint32_t accent;
  uint32_t on_accent;
  const std::string &accent_name = this->heating_ ? this->heating_accent_
                                   : this->drying_ ? this->drying_accent_
                                                   : this->accent_;
  theme->resolve_accent(accent_name, &accent, &on_accent);
  const uint32_t active_color = this->active_ ? accent : theme->outline_color();

  lv_obj_set_style_arc_color(this->track_, lv_color_hex(theme->outline_color()), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(this->track_, 100, LV_PART_MAIN);
  lv_obj_set_style_arc_width(this->track_, TRACK_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(this->track_, true, LV_PART_MAIN);

  lv_obj_set_style_arc_color(this->target_tail_, lv_color_hex(active_color), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(this->target_tail_, this->active_ ? 130 : 55, LV_PART_MAIN);
  lv_obj_set_style_arc_width(this->target_tail_, TRACK_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(this->target_tail_, true, LV_PART_MAIN);

  lv_obj_set_style_arc_color(this->active_range_, lv_color_hex(active_color), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(this->active_range_, this->active_ ? LV_OPA_COVER : 100, LV_PART_MAIN);
  lv_obj_set_style_arc_width(this->active_range_, TRACK_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(this->active_range_, true, LV_PART_MAIN);

  lv_obj_set_style_bg_color(this->current_marker_, lv_color_hex(0xFFFFFF), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->current_marker_, 160, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->current_marker_, 0, MAIN_DEFAULT);

  lv_obj_set_style_bg_color(this->target_marker_, lv_color_hex(0xFFFFFF), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->target_marker_, LV_OPA_COVER, MAIN_DEFAULT);
  lv_obj_set_style_border_color(this->target_marker_, lv_color_hex(active_color), MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->target_marker_, 3, MAIN_DEFAULT);
  lv_obj_set_style_border_opa(this->target_marker_, 255, MAIN_DEFAULT);
}

float HaDeckClimateArc::clamp_(float value) const {
  return std::max(this->minimum_, std::min(this->maximum_, value));
}

int16_t HaDeckClimateArc::angle_for_(float value) const {
  if (this->maximum_ <= this->minimum_)
    return START_ANGLE;
  const float ratio = (this->clamp_(value) - this->minimum_) / (this->maximum_ - this->minimum_);
  return START_ANGLE + static_cast<int16_t>(std::round(ratio * SWEEP_ANGLE));
}

void HaDeckClimateArc::position_marker_(lv_obj_t *marker, float value, uint16_t marker_size) {
  if (marker == nullptr || std::isnan(value))
    return;
  const float radians = this->angle_for_(value) * 3.14159265358979323846f / 180.0f;
  const float radius = (this->size_ - TRACK_WIDTH) / 2.0f;
  const float center_x = this->x_ + this->size_ / 2.0f;
  const float center_y = this->y_ + this->size_ / 2.0f;
  const int16_t x = static_cast<int16_t>(std::round(center_x + radius * std::cos(radians) - marker_size / 2.0f));
  const int16_t y = static_cast<int16_t>(std::round(center_y + radius * std::sin(radians) - marker_size / 2.0f));
  lv_obj_set_pos(marker, x, y);
}

void HaDeckClimateArc::update(float current, float target, bool active, bool heating, bool drying, bool force) {
  if (this->track_ == nullptr)
    return;
  const bool active_changed = !this->initialized_ || active != this->active_ || heating != this->heating_ ||
                              drying != this->drying_;
  this->active_ = active;
  this->heating_ = heating;
  this->drying_ = drying;
  if (active_changed)
    this->apply_theme(this->theme_);

  if (force || !this->initialized_ || !same_float_state(target, this->target_value_)) {
    this->target_value_ = target;
    if (std::isnan(target)) {
      lv_obj_add_flag(this->target_tail_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(this->active_range_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(this->target_marker_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_remove_flag(this->target_tail_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(this->target_marker_, LV_OBJ_FLAG_HIDDEN);
      this->position_marker_(this->target_marker_, target, TARGET_MARKER_SIZE);
    }
  }

  if (force || !this->initialized_ || !same_float_state(current, this->current_value_)) {
    this->current_value_ = current;
    if (std::isnan(current)) {
      lv_obj_add_flag(this->current_marker_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_remove_flag(this->current_marker_, LV_OBJ_FLAG_HIDDEN);
      this->position_marker_(this->current_marker_, current, CURRENT_MARKER_SIZE);
    }
  }

  const bool has_heating_demand = this->heating_ && !std::isnan(current) && !std::isnan(target) && current < target;
  const bool has_cooling_demand = !this->heating_ && !std::isnan(current) && !std::isnan(target) && current > target;
  if (!has_heating_demand && !has_cooling_demand) {
    lv_obj_add_flag(this->active_range_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_remove_flag(this->active_range_, LV_OBJ_FLAG_HIDDEN);
    const float range_start = this->heating_ ? current : target;
    const float range_end = this->heating_ ? target : current;
    lv_arc_set_bg_angles(this->active_range_, this->angle_for_(range_start) % 360,
                         this->angle_for_(range_end) % 360);
  }
  if (std::isnan(target) || (this->heating_ && std::isnan(current))) {
    lv_obj_add_flag(this->target_tail_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_remove_flag(this->target_tail_, LV_OBJ_FLAG_HIDDEN);
    if (this->heating_) {
      // Heating accent must never continue beyond target. When the room is
      // already warmer than requested, only the min -> target context remains.
      lv_arc_set_bg_angles(this->target_tail_, START_ANGLE,
                           this->angle_for_(std::min(current, target)) % 360);
    } else {
      lv_arc_set_bg_angles(this->target_tail_, this->angle_for_(target) % 360,
                           (START_ANGLE + SWEEP_ANGLE) % 360);
    }
  }
  this->initialized_ = true;
}

}  // namespace esphome::ha_deck
