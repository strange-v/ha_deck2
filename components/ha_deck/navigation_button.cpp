#include "navigation_button.h"

#include "ha_deck.h"
#include "button_style.h"

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

void HaDeckNavigationButton::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->button_ = lv_button_create(parent);
  this->apply_geometry_(theme);
  lv_obj_remove_flag(this->button_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(this->button_, HaDeckNavigationButton::event_callback_, LV_EVENT_CLICKED, this);
  this->label_ = lv_label_create(this->button_);
  lv_label_set_text(this->label_, this->glyph_.c_str());
  lv_obj_center(this->label_);
  this->apply_theme(theme);
  this->update(true);
}

void HaDeckNavigationButton::unmount() {
  this->button_ = nullptr;
  this->label_ = nullptr;
  this->reset_widget_state_();
}

void HaDeckNavigationButton::apply_theme(HaDeckTheme *theme) {
  if (this->button_ == nullptr || theme == nullptr)
    return;
  this->apply_geometry_(theme);
  const auto palette = apply_glass_button_surface(this->button_, theme, "", theme->disabled_opacity());
  apply_glass_text_content(this->label_, palette);
#ifdef USE_LVGL_FONT
  if (theme->icon_small_font() != nullptr)
    lv_obj_set_style_text_font(this->label_, theme->icon_small_font()->get_lv_font(), MAIN_DEFAULT);
#endif
}

void HaDeckNavigationButton::apply_geometry_(HaDeckTheme *theme) {
  if (this->button_ == nullptr || theme == nullptr)
    return;
  const uint16_t size = theme->touch_target();
  lv_obj_set_size(this->button_, this->has_size_ ? this->width_ : size, this->has_size_ ? this->height_ : size);
  if (!this->has_alignment_) {
    lv_obj_set_pos(this->button_, this->x_, this->y_);
    return;
  }
  const int16_t margin = this->has_margin_ ? this->margin_ : theme->spacing_small();
  int16_t x = 0;
  int16_t y = 0;
  switch (this->alignment_) {
    case LV_ALIGN_TOP_LEFT:
      x = margin;
      y = margin;
      break;
    case LV_ALIGN_TOP_RIGHT:
      x = -margin;
      y = margin;
      break;
    case LV_ALIGN_BOTTOM_LEFT:
      x = margin;
      y = -margin;
      break;
    case LV_ALIGN_BOTTOM_RIGHT:
      x = -margin;
      y = -margin;
      break;
    default:
      break;
  }
  lv_obj_align(this->button_, this->alignment_, x, y);
}

void HaDeckNavigationButton::event_callback_(lv_event_t *event) {
  auto *button = static_cast<HaDeckNavigationButton *>(lv_event_get_user_data(event));
  if (button->parent_ == nullptr)
    return;
  if (button->back_)
    button->parent_->go_back(button->animation_, button->time_);
  else if (button->target_ != nullptr)
    button->parent_->switch_screen(button->target_, button->animation_, button->time_);
}

}  // namespace esphome::ha_deck
