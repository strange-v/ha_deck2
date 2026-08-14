#include "overlay.h"

#include <algorithm>

namespace esphome::ha_deck {

static constexpr lv_opa_t BACKDROP_OPACITY = 150;
static constexpr lv_opa_t SHADOW_OPACITY = 90;

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

void HaDeckOverlay::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->root_ = lv_obj_create(parent);
  lv_obj_set_pos(this->root_, 0, 0);
  lv_obj_set_size(this->root_, LV_PCT(100), LV_PCT(100));
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(this->root_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->root_, 0, MAIN_DEFAULT);
  lv_obj_add_event_cb(this->root_, HaDeckOverlay::backdrop_event_callback_, LV_EVENT_CLICKED, this);

  this->panel_ = lv_obj_create(this->root_);
  lv_obj_set_size(this->panel_, LV_PCT(100), LV_PCT(100));
  lv_obj_center(this->panel_);
  lv_obj_remove_flag(this->panel_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(this->panel_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_all(this->panel_, 0, MAIN_DEFAULT);

  this->content_ = lv_obj_create(this->panel_);
  lv_obj_set_size(this->content_, LV_PCT(100), LV_PCT(100));
  lv_obj_set_pos(this->content_, 0, 0);
  lv_obj_set_style_bg_opa(this->content_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->content_, 0, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->content_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->content_, 0, MAIN_DEFAULT);
  lv_obj_remove_flag(this->content_, LV_OBJ_FLAG_SCROLLABLE);

  this->apply_theme(theme);
  lv_obj_add_flag(this->root_, LV_OBJ_FLAG_HIDDEN);
  this->open_ = false;
}

void HaDeckOverlay::unmount() {
  this->root_ = nullptr;
  this->panel_ = nullptr;
  this->content_ = nullptr;
  this->open_ = false;
}

void HaDeckOverlay::apply_theme(HaDeckTheme *theme) {
  if (this->root_ == nullptr || theme == nullptr)
    return;
  lv_obj_update_layout(this->root_);
  const int16_t inset = theme->spacing_small();
  const int16_t panel_width = std::max<int16_t>(1, lv_obj_get_content_width(this->root_) - 2 * inset);
  const int16_t panel_height = std::max<int16_t>(1, lv_obj_get_content_height(this->root_) - 2 * inset);
  lv_obj_set_size(this->panel_, panel_width, panel_height);
  lv_obj_center(this->panel_);
  lv_obj_set_style_bg_color(this->root_, lv_color_hex(0x000000), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->root_, BACKDROP_OPACITY, MAIN_DEFAULT);
  lv_obj_set_style_bg_color(this->panel_, lv_color_hex(theme->surface_container_color()), MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->panel_, LV_OPA_COVER, MAIN_DEFAULT);
  lv_obj_set_style_border_color(this->panel_, lv_color_hex(theme->outline_color()), MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->panel_, 1, MAIN_DEFAULT);
  lv_obj_set_style_radius(this->panel_, theme->radius(), MAIN_DEFAULT);
  lv_obj_set_style_shadow_width(this->panel_, theme->spacing_medium() + theme->spacing_small(), MAIN_DEFAULT);
  lv_obj_set_style_shadow_opa(this->panel_, SHADOW_OPACITY, MAIN_DEFAULT);
}

void HaDeckOverlay::open() {
  if (this->root_ == nullptr)
    return;
  this->open_ = true;
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(this->root_);
}

void HaDeckOverlay::close() {
  if (this->root_ == nullptr)
    return;
  this->open_ = false;
  lv_obj_add_flag(this->root_, LV_OBJ_FLAG_HIDDEN);
}

void HaDeckOverlay::backdrop_event_callback_(lv_event_t *event) {
  auto *overlay = static_cast<HaDeckOverlay *>(lv_event_get_user_data(event));
  if (lv_event_get_target(event) == overlay->root_)
    overlay->close();
}

}  // namespace esphome::ha_deck
