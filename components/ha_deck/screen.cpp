#include "screen.h"

#include "ha_deck.h"

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

void HaDeckScreen::mount(HaDeckTheme *theme) {
  if (this->obj_ != nullptr)
    return;

  this->obj_ = lv_obj_create(nullptr);
  lv_obj_remove_flag(this->obj_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(this->obj_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_border_width(this->obj_, 0, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->obj_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->obj_, 0, MAIN_DEFAULT);
  lv_obj_set_style_shadow_width(this->obj_, 0, MAIN_DEFAULT);
  this->apply_theme(theme);

  for (auto *widget : this->widgets_)
    widget->mount(this->obj_, theme);
}

void HaDeckScreen::unmount() {
  for (auto *widget : this->widgets_)
    widget->unmount();
  this->obj_ = nullptr;
}

void HaDeckScreen::update(bool force) {
  if (this->obj_ == nullptr)
    return;
  for (auto *widget : this->widgets_)
    widget->update(force);
}

void HaDeckScreen::apply_theme(HaDeckTheme *theme) {
  if (this->obj_ == nullptr || theme == nullptr)
    return;

  const uint32_t background =
      this->has_background_color_ ? this->background_color_ : theme->background_color();
  lv_obj_set_style_bg_color(this->obj_, lv_color_hex(background), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->obj_, LV_OPA_COVER, MAIN_DEFAULT);
  if (this->background_image_ != nullptr) {
    lv_obj_set_style_bg_image_src(this->obj_, this->background_image_->get_lv_image_dsc(), MAIN_DEFAULT);
    lv_obj_set_style_bg_image_opa(this->obj_, LV_OPA_COVER, MAIN_DEFAULT);
  } else {
    lv_obj_set_style_bg_image_src(this->obj_, nullptr, MAIN_DEFAULT);
  }
  for (auto *widget : this->widgets_)
    widget->apply_theme(theme);
  lv_obj_invalidate(this->obj_);
}

void HaDeckScreen::show(lv_screen_load_anim_t animation, uint32_t time) {
  if (this->parent_ != nullptr)
    this->parent_->switch_screen(this, animation, time);
}

}  // namespace esphome::ha_deck

