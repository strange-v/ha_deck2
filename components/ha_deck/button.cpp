#include "button.h"

#include "button_style.h"

#include <algorithm>

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
static constexpr lv_style_selector_t MAIN_PRESSED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED);
static constexpr lv_style_selector_t MAIN_DISABLED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DISABLED);
static constexpr lv_style_selector_t MAIN_CHECKED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
static constexpr lv_style_selector_t MAIN_ACCENTED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_USER_1);
static constexpr lv_style_selector_t MAIN_CHECKED_ACCENTED = static_cast<lv_style_selector_t>(LV_PART_MAIN) |
                                                              static_cast<lv_style_selector_t>(LV_STATE_CHECKED) |
                                                              static_cast<lv_style_selector_t>(LV_STATE_USER_1);

void HaDeckButton::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->button_ = lv_button_create(parent);
  lv_obj_set_pos(this->button_, this->x_, this->y_);
  lv_obj_set_size(this->button_, this->width_, this->height_);
  lv_obj_remove_flag(this->button_, LV_OBJ_FLAG_SCROLLABLE);
  if (this->toggle_)
    lv_obj_add_flag(this->button_, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(this->button_, HaDeckButton::event_callback_, LV_EVENT_ALL, this);

#ifdef USE_LVGL_FLEX
  lv_obj_set_layout(this->button_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->button_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(this->button_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
#endif
  if (this->icon_image_ != nullptr) {
#ifdef USE_LVGL_IMAGE
    this->icon_content_ = lv_image_create(this->button_);
    lv_image_set_src(this->icon_content_, this->icon_image_->get_lv_image_dsc());
#endif
  } else if (!this->icon_glyph_.empty()) {
#ifdef USE_LVGL_LABEL
    this->icon_content_ = lv_label_create(this->button_);
    lv_label_set_text(this->icon_content_, this->icon_glyph_.c_str());
#endif
  }

  if (!this->text_.empty()) {
#ifdef USE_LVGL_LABEL
    this->label_ = lv_label_create(this->button_);
    lv_label_set_text(this->label_, this->text_.c_str());
    lv_obj_set_style_text_align(this->label_, LV_TEXT_ALIGN_CENTER, MAIN_DEFAULT);
#endif
  }

  this->apply_theme(theme);
  this->initialized_state_ = false;
  this->update(true);
}

void HaDeckButton::unmount() {
  this->button_ = nullptr;
  this->icon_content_ = nullptr;
  this->label_ = nullptr;
  this->initialized_state_ = false;
  this->long_press_fired_ = false;
  this->theme_ = nullptr;
  this->reset_widget_state_();
}

void HaDeckButton::apply_theme(HaDeckTheme *theme) {
  if (this->button_ == nullptr || theme == nullptr)
    return;

  this->theme_ = theme;
  const std::string accent = this->accent_.value();
  this->accent_state_ = accent;
  const uint16_t radius = this->has_radius_ ? this->radius_ : theme->radius();
  const uint16_t content_gap = std::max<uint16_t>(1, theme->spacing_small() / 2);
  lv_obj_set_style_pad_all(this->button_, content_gap, MAIN_DEFAULT);
  lv_obj_set_style_pad_row(this->button_, content_gap, MAIN_DEFAULT);
  const uint8_t disabled_opacity =
      this->has_disabled_opacity_ ? this->disabled_opacity_ : theme->disabled_opacity();
  uint32_t accent_color;
  uint32_t on_accent_color;
  theme->resolve_accent(accent, &accent_color, &on_accent_color);
  uint32_t foreground;
  if (this->variant_ == ButtonVariant::ICON) {
    foreground = this->has_text_color_ ? this->text_color_ : theme->on_surface_color();
    lv_obj_set_style_bg_opa(this->button_, LV_OPA_TRANSP, MAIN_DEFAULT);
    lv_obj_set_style_border_width(this->button_, 0, MAIN_DEFAULT);
    lv_obj_set_style_shadow_width(this->button_, 0, MAIN_DEFAULT);
    lv_obj_set_style_radius(this->button_, radius, MAIN_DEFAULT);
    lv_obj_set_style_opa(this->button_, LV_OPA_COVER, MAIN_DEFAULT);
    lv_obj_set_style_opa(this->button_, 180, MAIN_PRESSED);
    lv_obj_set_style_opa(this->button_, disabled_opacity, MAIN_DISABLED);
  } else if (this->variant_ == ButtonVariant::FILLED) {
    foreground = this->has_text_color_ ? this->text_color_ : on_accent_color;
    lv_obj_set_style_bg_color(this->button_,
                              lv_color_hex(this->has_background_color_ ? this->background_color_ : accent_color),
                              MAIN_DEFAULT);
    lv_obj_set_style_bg_opa(this->button_, LV_OPA_COVER, MAIN_DEFAULT);
    lv_obj_set_style_border_width(this->button_, this->has_border_width_ ? this->border_width_ : 0, MAIN_DEFAULT);
    lv_obj_set_style_border_color(this->button_,
                                  lv_color_hex(this->has_border_color_ ? this->border_color_ : accent_color),
                                  MAIN_DEFAULT);
    lv_obj_set_style_radius(this->button_, radius, MAIN_DEFAULT);
    lv_obj_set_style_shadow_width(this->button_, 0, MAIN_DEFAULT);
    lv_obj_set_style_opa(this->button_, LV_OPA_COVER, MAIN_DEFAULT);
    lv_obj_set_style_opa(this->button_, 220, MAIN_PRESSED);
    lv_obj_set_style_opa(this->button_, disabled_opacity, MAIN_DISABLED);
  } else {
    const auto palette = apply_glass_button_surface(this->button_, theme, accent, disabled_opacity);
    foreground = this->has_text_color_ ? this->text_color_ : palette.foreground;
    if (this->has_background_color_)
      lv_obj_set_style_bg_color(this->button_, lv_color_hex(this->background_color_), MAIN_DEFAULT);
    if (this->has_border_color_)
      lv_obj_set_style_border_color(this->button_, lv_color_hex(this->border_color_), MAIN_DEFAULT);
    if (this->has_border_width_)
      lv_obj_set_style_border_width(this->button_, this->border_width_, MAIN_DEFAULT);
    lv_obj_set_style_radius(this->button_, radius, MAIN_DEFAULT);
  }

  if (this->icon_content_ != nullptr && this->icon_image_ != nullptr) {
#ifdef USE_LVGL_IMAGE
    lv_obj_set_style_image_recolor(this->icon_content_, lv_color_hex(foreground), MAIN_DEFAULT);
    lv_obj_set_style_image_recolor_opa(this->icon_content_, LV_OPA_COVER, MAIN_DEFAULT);
    lv_obj_set_style_image_recolor(this->icon_content_, lv_color_hex(accent_color), MAIN_ACCENTED);
    if (this->toggle_ || this->has_checked_source_)
      lv_obj_set_style_image_recolor(this->icon_content_, lv_color_hex(foreground), MAIN_CHECKED);
    lv_obj_set_style_image_recolor(this->icon_content_, lv_color_hex(accent_color), MAIN_CHECKED_ACCENTED);
#endif
  } else if (this->icon_content_ != nullptr) {
#ifdef USE_LVGL_LABEL
    lv_obj_set_style_text_color(this->icon_content_, lv_color_hex(foreground), MAIN_DEFAULT);
    lv_obj_set_style_text_color(this->icon_content_, lv_color_hex(accent_color), MAIN_ACCENTED);
    if (this->toggle_ || this->has_checked_source_)
      lv_obj_set_style_text_color(this->icon_content_, lv_color_hex(foreground), MAIN_CHECKED);
    lv_obj_set_style_text_color(this->icon_content_, lv_color_hex(accent_color), MAIN_CHECKED_ACCENTED);
#ifdef USE_LVGL_FONT
    auto *font = this->icon_font_ != nullptr ? this->icon_font_ : theme->icon_medium_font();
    if (font != nullptr)
      lv_obj_set_style_text_font(this->icon_content_, font->get_lv_font(), MAIN_DEFAULT);
#endif
#endif
  }

  if (this->label_ != nullptr) {
#ifdef USE_LVGL_LABEL
    lv_obj_set_style_text_color(this->label_, lv_color_hex(foreground), MAIN_DEFAULT);
    if (this->toggle_ || this->has_checked_source_)
      lv_obj_set_style_text_color(this->label_, lv_color_hex(foreground), MAIN_CHECKED);
#ifdef USE_LVGL_FONT
    auto *font = this->font_ != nullptr ? this->font_ : theme->text_small_font();
    if (font != nullptr)
      lv_obj_set_style_text_font(this->label_, font->get_lv_font(), MAIN_DEFAULT);
#endif
#endif
  }
}

void HaDeckButton::set_content_checked_(bool checked) {
  for (auto *obj : {this->icon_content_, this->label_}) {
    if (obj == nullptr)
      continue;
    if (checked)
      lv_obj_add_state(obj, LV_STATE_CHECKED);
    else
      lv_obj_remove_state(obj, LV_STATE_CHECKED);
  }
}

void HaDeckButton::update_icon_accent_state_() {
  if (this->icon_content_ == nullptr)
    return;
  if (this->accent_icon_state_ && this->variant_ == ButtonVariant::GLASS)
    lv_obj_add_state(this->icon_content_, LV_STATE_USER_1);
  else
    lv_obj_remove_state(this->icon_content_, LV_STATE_USER_1);
}

void HaDeckButton::update_state_(bool force) {
  if (this->button_ == nullptr)
    return;

  const bool disabled = this->disabled_.value();
  const bool checked = this->has_checked_source_ ? this->checked_.value() : (this->toggle_ && this->checked_state_);
  const bool accent_icon = this->accent_icon_.value();
  const std::string accent = this->accent_.value();
  const bool accent_changed = accent != this->accent_state_;
  if (!force && this->initialized_state_ && disabled == this->disabled_state_ &&
      checked == this->checked_state_ && accent_icon == this->accent_icon_state_ && !accent_changed)
    return;

  if (accent_changed && this->theme_ != nullptr)
    this->apply_theme(this->theme_);

  this->initialized_state_ = true;
  this->disabled_state_ = disabled;
  this->checked_state_ = checked;
  this->accent_icon_state_ = accent_icon;
  if (disabled)
    lv_obj_add_state(this->button_, LV_STATE_DISABLED);
  else
    lv_obj_remove_state(this->button_, LV_STATE_DISABLED);

  if (checked) {
    lv_obj_add_state(this->button_, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(this->button_, LV_STATE_CHECKED);
  }
  this->set_content_checked_(checked);

  this->update_icon_accent_state_();
}

void HaDeckButton::event_callback_(lv_event_t *event) {
  auto *button = static_cast<HaDeckButton *>(lv_event_get_user_data(event));
  if (button->disabled_state_)
    return;

  const auto code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESSED) {
    button->long_press_fired_ = false;
    button->press_trigger_.trigger();
    return;
  }
  if (code == LV_EVENT_LONG_PRESSED) {
    button->long_press_fired_ = true;
    button->long_press_trigger_.trigger();
    return;
  }
  if (code == LV_EVENT_RELEASED) {
    if (button->long_press_fired_ && button->toggle_) {
      // LVGL toggles CHECKED automatically on RELEASED. Restore the state that
      // preceded this gesture because a long press is not a toggle click.
      if (button->checked_state_)
        lv_obj_add_state(button->button_, LV_STATE_CHECKED);
      else
        lv_obj_remove_state(button->button_, LV_STATE_CHECKED);
      button->set_content_checked_(button->checked_state_);
    }
    button->release_trigger_.trigger();
    return;
  }
  if (code != LV_EVENT_CLICKED)
    return;

  // LVGL emits CLICKED after RELEASED even when LONG_PRESSED already fired.
  // Treat long press and click as mutually exclusive gestures.
  if (button->long_press_fired_)
    return;

  if (button->toggle_) {
    button->checked_state_ = lv_obj_has_state(button->button_, LV_STATE_CHECKED);
    button->initialized_state_ = true;
    button->set_content_checked_(button->checked_state_);
    button->update_icon_accent_state_();
    if (button->checked_state_)
      button->turn_on_trigger_.trigger();
    else
      button->turn_off_trigger_.trigger();
  }
  button->trigger();
}

}  // namespace esphome::ha_deck

