#include "slider.h"

#include <cmath>
#include <cstdio>

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
static constexpr lv_style_selector_t INDICATOR_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_INDICATOR) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
static constexpr lv_style_selector_t KNOB_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_KNOB) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

void HaDeckSlider::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->root_ = lv_obj_create(parent);
  lv_obj_set_pos(this->root_, this->x_, this->y_);
  lv_obj_set_size(this->root_, this->width_, this->height_);
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_row(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->root_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_layout(this->root_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->root_, this->vertical_ ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);

  this->slider_ = lv_slider_create(this->root_);
  lv_slider_set_range(this->slider_, this->min_value_, this->max_value_);
  lv_obj_set_flex_grow(this->slider_, 1);
  if (this->vertical_) {
    lv_obj_set_width(this->slider_, LV_PCT(100));
  } else {
    lv_obj_set_height(this->slider_, LV_PCT(100));
  }
  lv_obj_add_event_cb(this->slider_, HaDeckSlider::slider_event_callback_, LV_EVENT_ALL, this);

  if (!this->label_text_.empty() || !this->icon_glyph_.empty()) {
    this->button_ = lv_button_create(this->root_);
    if (this->vertical_) {
      lv_obj_set_size(this->button_, LV_PCT(100), this->action_height_);
    } else {
      lv_obj_set_size(this->button_, this->action_height_, LV_PCT(100));
    }
    this->label_ = lv_label_create(this->button_);
    lv_obj_center(this->label_);
    lv_obj_add_event_cb(this->button_, HaDeckSlider::button_event_callback_, LV_EVENT_CLICKED, this);
  }

  this->apply_theme(theme);
  this->state_initialized_ = false;
  this->update(true);
}

void HaDeckSlider::unmount() {
  this->root_ = nullptr;
  this->slider_ = nullptr;
  this->button_ = nullptr;
  this->label_ = nullptr;
  this->action_text_lv_font_ = nullptr;
  this->action_icon_lv_font_ = nullptr;
  this->state_initialized_ = false;
  this->pending_ = false;
  this->reset_widget_state_();
}

void HaDeckSlider::apply_theme(HaDeckTheme *theme) {
  if (this->slider_ == nullptr || theme == nullptr)
    return;
  uint32_t accent;
  uint32_t on_accent;
  theme->resolve_accent(this->accent_, &accent, &on_accent);
  lv_obj_set_style_bg_color(this->slider_, lv_color_hex(theme->on_surface_color()), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->slider_, 64, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->slider_, theme->radius(), MAIN_DEFAULT);
  lv_obj_set_style_bg_color(this->slider_, lv_color_hex(accent), INDICATOR_DEFAULT);
  lv_obj_set_style_bg_opa(this->slider_, LV_OPA_COVER, INDICATOR_DEFAULT);
  lv_obj_set_style_radius(this->slider_, theme->radius(), INDICATOR_DEFAULT);
  lv_obj_set_style_bg_opa(this->slider_, LV_OPA_TRANSP, KNOB_DEFAULT);
  lv_obj_set_style_border_width(this->slider_, 0, KNOB_DEFAULT);
  lv_obj_set_style_shadow_width(this->slider_, 0, KNOB_DEFAULT);
  lv_obj_set_style_pad_all(this->slider_, 0, KNOB_DEFAULT);
  if (this->button_ != nullptr) {
    lv_obj_set_style_bg_opa(this->button_, LV_OPA_TRANSP, MAIN_DEFAULT);
    lv_obj_set_style_border_width(this->button_, 0, MAIN_DEFAULT);
    lv_obj_set_style_shadow_width(this->button_, 0, MAIN_DEFAULT);
  }
  if (this->label_ != nullptr)
    lv_obj_set_style_text_color(this->label_, lv_color_hex(theme->on_surface_color()), MAIN_DEFAULT);
  if (this->label_ != nullptr) {
    auto *text_font = this->font_ != nullptr ? this->font_ : theme->text_small_font();
    auto *icon_font = this->icon_font_ != nullptr ? this->icon_font_ : theme->icon_small_font();
    this->action_text_lv_font_ =
        text_font != nullptr ? text_font->get_lv_font() : lv_obj_get_style_text_font(this->label_, LV_PART_MAIN);
    this->action_icon_lv_font_ = icon_font != nullptr ? icon_font->get_lv_font() : nullptr;
    this->update_label_(false);
  }
}

void HaDeckSlider::update_state_(bool force) {
  if (this->slider_ == nullptr)
    return;
  const bool disabled = this->disabled_.value();
  const bool pressed = lv_obj_has_state(this->slider_, LV_STATE_PRESSED);
  const float external_value = this->value_.value();
  const int32_t value = std::isnan(external_value) ? this->min_value_ : static_cast<int32_t>(std::round(external_value));

  if (force || !this->state_initialized_ || disabled != this->disabled_state_) {
    this->disabled_state_ = disabled;
    for (auto *obj : {this->slider_, this->button_}) {
      if (obj == nullptr)
        continue;
      if (disabled)
        lv_obj_add_state(obj, LV_STATE_DISABLED);
      else
        lv_obj_remove_state(obj, LV_STATE_DISABLED);
    }
  }
  bool accept_external_value = !pressed;
  if (accept_external_value && this->pending_) {
    if (value == this->pending_value_) {
      // The bound state has confirmed the optimistic value.
      this->pending_ = false;
    } else if (static_cast<uint32_t>(millis() - this->pending_since_) < this->optimistic_timeout_) {
      // Keep rendering the user's value while Home Assistant still reports the
      // pre-action state.
      accept_external_value = false;
    } else {
      // The action was rejected, transformed, or never confirmed. Return to the
      // real source of truth rather than remaining optimistically stale.
      this->pending_ = false;
    }
  }
  if (accept_external_value && (force || !this->state_initialized_ || value != this->rendered_value_)) {
    this->rendered_value_ = value;
    lv_slider_set_value(this->slider_, value, LV_ANIM_OFF);
  }
  this->state_initialized_ = true;
}

void HaDeckSlider::update_label_(bool show_value) {
  if (this->label_ == nullptr)
    return;
  if (!show_value) {
    if (!this->icon_glyph_.empty()) {
      lv_label_set_text(this->label_, this->icon_glyph_.c_str());
      if (this->action_icon_lv_font_ != nullptr)
        lv_obj_set_style_text_font(this->label_, this->action_icon_lv_font_, MAIN_DEFAULT);
    } else {
      lv_label_set_text(this->label_, this->label_text_.c_str());
      if (this->action_text_lv_font_ != nullptr)
        lv_obj_set_style_text_font(this->label_, this->action_text_lv_font_, MAIN_DEFAULT);
    }
    return;
  }
  if (this->action_text_lv_font_ != nullptr)
    lv_obj_set_style_text_font(this->label_, this->action_text_lv_font_, MAIN_DEFAULT);
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), this->format_.c_str(),
                static_cast<double>(lv_slider_get_value(this->slider_)));
  lv_label_set_text(this->label_, buffer);
}

void HaDeckSlider::slider_event_callback_(lv_event_t *event) {
  auto *widget = static_cast<HaDeckSlider *>(lv_event_get_user_data(event));
  const auto code = lv_event_get_code(event);
  const float value = static_cast<float>(lv_slider_get_value(widget->slider_));
  if (code == LV_EVENT_PRESSED) {
    widget->pending_ = false;
    widget->update_label_(true);
  } else if (code == LV_EVENT_VALUE_CHANGED && lv_obj_has_state(widget->slider_, LV_STATE_PRESSED)) {
    widget->rendered_value_ = static_cast<int32_t>(value);
    widget->update_label_(true);
    widget->value_trigger_.trigger(value);
  } else if (code == LV_EVENT_RELEASED) {
    widget->rendered_value_ = static_cast<int32_t>(value);
    if (widget->optimistic_) {
      widget->pending_ = true;
      widget->pending_value_ = widget->rendered_value_;
      widget->pending_since_ = millis();
    }
    widget->release_trigger_.trigger(value);
    widget->update_label_(false);
  }
}

void HaDeckSlider::button_event_callback_(lv_event_t *event) {
  auto *widget = static_cast<HaDeckSlider *>(lv_event_get_user_data(event));
  if (!widget->disabled_state_)
    widget->click_trigger_.trigger();
}

}  // namespace esphome::ha_deck
