#include "climate.h"

#include "button_style.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "numeric_utils.h"

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);
static constexpr lv_style_selector_t MAIN_CHECKED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED);
static constexpr lv_style_selector_t MAIN_DISABLED =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DISABLED);

void HaDeckClimateOption::mount(lv_obj_t *parent, HaDeckTheme *theme, bool primary, uint16_t width,
                                uint16_t height) {
  this->primary_ = primary;
  this->button_ = lv_button_create(parent);
  lv_obj_set_height(this->button_, primary ? LV_PCT(100) : height);
  lv_obj_set_width(this->button_, primary ? 0 : width);
  if (primary) {
    lv_obj_set_flex_grow(this->button_, 1);
  }
  lv_obj_remove_flag(this->button_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(this->button_, HaDeckClimateOption::event_callback_, LV_EVENT_CLICKED, this);
#ifdef USE_LVGL_FLEX
  lv_obj_set_layout(this->button_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->button_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(this->button_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
#endif
  const uint16_t content_gap = std::max<uint16_t>(1, theme->spacing_small() / 2);
  lv_obj_set_style_pad_all(this->button_, content_gap, MAIN_DEFAULT);
  lv_obj_set_style_pad_row(this->button_, content_gap, MAIN_DEFAULT);

  if (this->icon_image_ != nullptr) {
#ifdef USE_LVGL_IMAGE
    this->icon_ = lv_image_create(this->button_);
    lv_image_set_src(this->icon_, this->icon_image_->get_lv_image_dsc());
#endif
  } else if (!this->icon_glyph_.empty()) {
    this->icon_ = lv_label_create(this->button_);
    lv_label_set_text(this->icon_, this->icon_glyph_.c_str());
  }
  if (!this->label_.empty()) {
    this->label_obj_ = lv_label_create(this->button_);
    lv_label_set_text(this->label_obj_, this->label_.c_str());
    lv_obj_set_style_text_align(this->label_obj_, LV_TEXT_ALIGN_CENTER, MAIN_DEFAULT);
  }
  this->apply_theme(theme);
  this->initialized_ = false;
  this->update(true);
}

void HaDeckClimateOption::set_size(uint16_t width, uint16_t height) {
  if (this->button_ != nullptr && !this->primary_)
    lv_obj_set_size(this->button_, width, height);
}

void HaDeckClimateOption::unmount() {
  this->button_ = nullptr;
  this->icon_ = nullptr;
  this->label_obj_ = nullptr;
  this->initialized_ = false;
  this->disabled_state_ = false;
}

void HaDeckClimateOption::apply_theme(HaDeckTheme *theme) {
  if (this->button_ == nullptr || theme == nullptr)
    return;
  const auto palette =
      apply_glass_button_surface(this->button_, theme, this->accent_, theme->disabled_opacity());
  for (auto *obj : {this->icon_, this->label_obj_}) {
    if (obj == nullptr)
      continue;
    if (obj == this->icon_ && this->icon_image_ != nullptr) {
#ifdef USE_LVGL_IMAGE
      apply_glass_image_content(obj, palette);
#endif
    } else {
      apply_glass_text_content(obj, palette);
    }
  }
#ifdef USE_LVGL_FONT
  if (this->icon_ != nullptr && this->icon_image_ == nullptr) {
    auto *icon_font = this->icon_font_ != nullptr
                          ? this->icon_font_
                          : (this->primary_ ? theme->icon_medium_font() : theme->icon_small_font());
    if (icon_font != nullptr)
      lv_obj_set_style_text_font(this->icon_, icon_font->get_lv_font(), MAIN_DEFAULT);
  }
  if (this->label_obj_ != nullptr && theme->text_small_font() != nullptr)
    lv_obj_set_style_text_font(this->label_obj_, theme->text_small_font()->get_lv_font(), MAIN_DEFAULT);
#endif
}

void HaDeckClimateOption::update(bool force) {
  if (this->button_ == nullptr)
    return;
  const bool active = this->active_.value();
  if (!force && this->initialized_ && active == this->active_state_)
    return;
  this->initialized_ = true;
  this->active_state_ = active;
  if (active)
    lv_obj_add_state(this->button_, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(this->button_, LV_STATE_CHECKED);
  for (auto *obj : {this->icon_, this->label_obj_}) {
    if (obj == nullptr)
      continue;
    if (active)
      lv_obj_add_state(obj, LV_STATE_CHECKED);
    else
      lv_obj_remove_state(obj, LV_STATE_CHECKED);
  }
}

void HaDeckClimateOption::set_disabled_state(bool disabled) {
  if (this->button_ == nullptr || disabled == this->disabled_state_)
    return;
  this->disabled_state_ = disabled;
  if (disabled)
    lv_obj_add_state(this->button_, LV_STATE_DISABLED);
  else
    lv_obj_remove_state(this->button_, LV_STATE_DISABLED);
}

void HaDeckClimateOption::event_callback_(lv_event_t *event) {
  auto *option = static_cast<HaDeckClimateOption *>(lv_event_get_user_data(event));
  option->trigger();
}

lv_obj_t *HaDeckClimate::create_text_button_(lv_obj_t *parent, const char *text, int16_t x, int16_t y,
                                             uint16_t width, uint16_t height) {
  auto *button = lv_button_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, width, height);
  lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  auto *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return button;
}

void HaDeckClimate::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->theme_ = theme;
  this->root_ = lv_obj_create(parent);
  lv_obj_set_pos(this->root_, this->x_, this->y_);
  lv_obj_set_size(this->root_, this->width_, this->height_);
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(this->root_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_layout(this->root_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->root_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(this->root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  this->top_area_ = lv_obj_create(this->root_);
  lv_obj_set_width(this->top_area_, LV_PCT(100));
  lv_obj_set_height(this->top_area_, 0);
  lv_obj_set_flex_grow(this->top_area_, 1);
  lv_obj_remove_flag(this->top_area_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->top_area_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(this->top_area_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->top_area_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->top_area_, 0, MAIN_DEFAULT);

  this->bottom_controls_ = lv_obj_create(this->root_);
  lv_obj_set_width(this->bottom_controls_, LV_PCT(100));
  lv_obj_remove_flag(this->bottom_controls_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->bottom_controls_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(this->bottom_controls_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->bottom_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->bottom_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_layout(this->bottom_controls_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->bottom_controls_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->bottom_controls_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  this->current_label_ = lv_label_create(this->top_area_);
  lv_obj_set_style_text_align(this->current_label_, LV_TEXT_ALIGN_CENTER, MAIN_DEFAULT);

  this->target_row_ = lv_obj_create(this->top_area_);
  lv_obj_set_size(this->target_row_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_remove_flag(this->target_row_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->target_row_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(this->target_row_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->target_row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->target_row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->target_row_, 2, MAIN_DEFAULT);
  lv_obj_set_layout(this->target_row_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->target_row_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->target_row_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
  this->target_value_label_ = lv_label_create(this->target_row_);
  this->target_units_label_ = lv_label_create(this->target_row_);
  lv_label_set_text(this->target_units_label_, this->units_.c_str());

  this->adjust_controls_ = lv_obj_create(this->top_area_);
  lv_obj_set_size(this->adjust_controls_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_remove_flag(this->adjust_controls_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->adjust_controls_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(this->adjust_controls_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->adjust_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->adjust_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_layout(this->adjust_controls_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->adjust_controls_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->adjust_controls_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  this->minus_button_ = this->create_text_button_(this->adjust_controls_, "−", 0, 0, 1, 1);
  this->plus_button_ = this->create_text_button_(this->adjust_controls_, "+", 0, 0, 1, 1);
  lv_obj_add_event_cb(this->minus_button_, HaDeckClimate::event_callback_, LV_EVENT_CLICKED, this);
  lv_obj_add_event_cb(this->plus_button_, HaDeckClimate::event_callback_, LV_EVENT_CLICKED, this);

  if (!this->additional_groups_.empty()) {
    this->menu_button_ = this->create_text_button_(this->top_area_, this->menu_glyph_.c_str(), 0, 0, 1, 1);
    this->menu_label_ = lv_obj_get_child(this->menu_button_, 0);
    lv_obj_add_event_cb(this->menu_button_, HaDeckClimate::event_callback_, LV_EVENT_CLICKED, this);
  }

  this->primary_controls_ = lv_obj_create(this->bottom_controls_);
  lv_obj_set_width(this->primary_controls_, 0);
  lv_obj_set_height(this->primary_controls_, LV_PCT(100));
  lv_obj_set_flex_grow(this->primary_controls_, 1);
  lv_obj_set_style_bg_opa(this->primary_controls_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->primary_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_all(this->primary_controls_, 0, MAIN_DEFAULT);
  lv_obj_remove_flag(this->primary_controls_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(this->primary_controls_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_layout(this->primary_controls_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->primary_controls_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->primary_controls_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  for (auto *option : this->primary_options_)
    option->mount(this->primary_controls_, theme, true);

  this->power_button_ = this->create_text_button_(this->bottom_controls_, this->power_glyph_.c_str(), 0, 0, 1, 1);
  this->power_label_ = lv_obj_get_child(this->power_button_, 0);
  lv_obj_add_event_cb(this->power_button_, HaDeckClimate::event_callback_, LV_EVENT_CLICKED, this);

  this->apply_layout_metrics_(theme);
  lv_obj_update_layout(this->root_);
  const int16_t top_width = lv_obj_get_content_width(this->top_area_);
  const int16_t top_height = lv_obj_get_content_height(this->top_area_);
  const uint16_t arc_size = std::max<int16_t>(1, std::min(top_width, top_height));
  const int16_t arc_x = (top_width - arc_size) / 2;
  const int16_t arc_y = (top_height - arc_size) / 2;
  this->climate_arc_.set_geometry(arc_x, arc_y, arc_size);
  this->climate_arc_.set_range(this->min_temperature_, this->max_temperature_, this->step_);
  this->climate_arc_.set_accent(this->accent_);
  this->climate_arc_.set_heating_accent(this->heating_accent_);
  this->climate_arc_.set_drying_accent(this->drying_accent_);
  this->climate_arc_.mount(this->top_area_, theme);
  for (auto *obj : {this->current_label_, this->target_row_, this->adjust_controls_, this->menu_button_})
    if (obj != nullptr)
      lv_obj_move_foreground(obj);

  if (!this->additional_groups_.empty()) {
    // Mount as a screen-level sibling so move_foreground() can place the modal
    // above navigation and other widgets, regardless of their YAML order.
    this->overlay_.mount(parent, theme);
    this->build_overlay_(theme);
  }
  this->apply_theme(theme);
  this->initialized_ = false;
  this->pending_ = false;
  this->update(true);
}

void HaDeckClimate::apply_layout_metrics_(HaDeckTheme *theme) {
  if (this->root_ == nullptr || theme == nullptr)
    return;
  this->resolved_padding_ = this->has_padding_ ? this->padding_ : theme->spacing_small();
  this->resolved_gap_ = this->has_gap_ ? this->gap_ : theme->spacing_small();
  this->resolved_controls_height_ =
      this->has_controls_height_ ? this->controls_height_ : theme->control_height();
  this->resolved_touch_target_ = theme->touch_target();

  lv_obj_set_style_pad_all(this->root_, this->resolved_padding_, MAIN_DEFAULT);
  lv_obj_set_style_pad_row(this->root_, this->resolved_gap_, MAIN_DEFAULT);
  lv_obj_set_height(this->bottom_controls_, this->resolved_controls_height_);
  // The visual separation before power is deliberately twice the ordinary
  // spacing used between primary controls.
  lv_obj_set_style_pad_column(this->bottom_controls_, this->resolved_gap_ * 2, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->primary_controls_, this->resolved_gap_, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->adjust_controls_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->target_row_, std::max<uint16_t>(1, this->resolved_gap_ / 4), MAIN_DEFAULT);

  for (auto *button : {this->minus_button_, this->plus_button_, this->menu_button_}) {
    if (button != nullptr)
      lv_obj_set_size(button, this->resolved_touch_target_, this->resolved_touch_target_);
  }
  lv_obj_set_size(this->power_button_, this->resolved_controls_height_, LV_PCT(100));
}

void HaDeckClimate::layout_content_() {
  if (this->root_ == nullptr || this->top_area_ == nullptr)
    return;
  lv_obj_update_layout(this->root_);
  const int16_t top_width = lv_obj_get_content_width(this->top_area_);
  const int16_t top_height = lv_obj_get_content_height(this->top_area_);
  const uint16_t arc_size = std::max<int16_t>(1, std::min(top_width, top_height));
  this->climate_arc_.set_geometry((top_width - arc_size) / 2, (top_height - arc_size) / 2, arc_size);

  // Keep the controls proportional to the available dial rather than using a
  // fixed pixel gap. The lower bound preserves comfortable separation on small
  // screens; the proportional target naturally expands on larger displays.
  const int16_t minimum_adjust_width = 2 * this->resolved_touch_target_ + 2 * this->resolved_gap_;
  const int16_t proportional_adjust_width = arc_size / 2;
  const int16_t adjust_width = std::min<int16_t>(top_width, std::max(minimum_adjust_width,
                                                                    proportional_adjust_width));
  lv_obj_set_size(this->adjust_controls_, adjust_width, this->resolved_touch_target_);

  lv_obj_align(this->target_row_, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align_to(this->current_label_, this->target_row_, LV_ALIGN_OUT_TOP_MID, 0, -this->resolved_gap_);
  lv_obj_align(this->adjust_controls_, LV_ALIGN_BOTTOM_MID, 0, -this->resolved_gap_);
  if (this->menu_button_ != nullptr)
    lv_obj_align(this->menu_button_, LV_ALIGN_TOP_RIGHT, 0, 0);
  this->align_units_baseline_();
}

void HaDeckClimate::align_units_baseline_() {
  if (this->target_value_label_ == nullptr || this->target_units_label_ == nullptr)
    return;
  const lv_font_t *value_font = lv_obj_get_style_text_font(this->target_value_label_, LV_PART_MAIN);
  const lv_font_t *units_font = lv_obj_get_style_text_font(this->target_units_label_, LV_PART_MAIN);
  if (value_font == nullptr || units_font == nullptr)
    return;
  lv_obj_set_style_translate_y(this->target_units_label_, units_font->base_line - value_font->base_line,
                               MAIN_DEFAULT);
}

void HaDeckClimate::build_overlay_(HaDeckTheme *theme) {
  auto *content = this->overlay_.content_obj();
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  this->overlay_close_button_ = this->create_text_button_(content, this->close_glyph_.c_str(), 0, 0, 1, 1);
  lv_obj_add_event_cb(this->overlay_close_button_, HaDeckClimate::event_callback_, LV_EVENT_CLICKED, this);
  for (auto *group : this->additional_groups_) {
    auto *title = lv_label_create(content);
    lv_label_set_text(title, group->label().c_str());
    this->overlay_titles_.push_back(title);
    auto *row = lv_obj_create(content);
    this->overlay_option_rows_.push_back(row);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, MAIN_DEFAULT);
    lv_obj_set_style_border_width(row, 0, MAIN_DEFAULT);
    lv_obj_set_style_pad_all(row, 0, MAIN_DEFAULT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
#ifdef USE_LVGL_FLEX
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
#endif
    for (auto *option : group->options())
      option->mount(row, theme, false, 1, 1);
  }
  this->layout_overlay_(theme);
}

void HaDeckClimate::layout_overlay_(HaDeckTheme *theme) {
  auto *content = this->overlay_.content_obj();
  if (content == nullptr || theme == nullptr)
    return;

  lv_obj_update_layout(content);
  const int32_t content_width = std::max<int32_t>(1, lv_obj_get_content_width(content));
  const uint16_t padding = theme->spacing_medium();
  const uint16_t gap = theme->spacing_small();
  const uint16_t control_size = theme->touch_target();
  const int32_t available_width = std::max<int32_t>(1, content_width - 2 * padding);
  lv_obj_set_pos(this->overlay_close_button_, std::max<int32_t>(0, content_width - padding - control_size), padding);
  lv_obj_set_size(this->overlay_close_button_, control_size, control_size);

  int32_t y = padding + control_size + gap;
  for (size_t index = 0; index < this->additional_groups_.size(); index++) {
    auto *group = this->additional_groups_[index];
    auto *title = this->overlay_titles_[index];
    auto *row = this->overlay_option_rows_[index];
    const auto &options = group->options();
    const uint16_t title_height = theme->text_small_font() != nullptr
                                      ? theme->text_small_font()->get_lv_font()->line_height
                                      : std::max<uint16_t>(1, control_size / 2);
    lv_obj_set_pos(title, padding, y);
    lv_obj_set_size(title, available_width, title_height);
    y += title_height + gap;

    const uint16_t preferred_width = std::max<uint16_t>(control_size, control_size * 2);
    const uint16_t fitting_columns = std::max<int32_t>(1, (available_width + gap) / (preferred_width + gap));
    const uint16_t columns = std::max<uint16_t>(1, std::min<uint16_t>(options.size(), fitting_columns));
    const int32_t rows = (options.size() + columns - 1) / columns;
    const uint16_t option_width = std::max<int32_t>(1, (available_width - gap * (columns - 1)) / columns);
    const int32_t row_height = rows * control_size + gap * (rows - 1);
    lv_obj_set_pos(row, padding, y);
    lv_obj_set_size(row, available_width, row_height);
    lv_obj_set_style_pad_row(row, gap, MAIN_DEFAULT);
    lv_obj_set_style_pad_column(row, gap, MAIN_DEFAULT);
    for (auto *option : options)
      option->set_size(option_width, control_size);
    y += row_height + theme->spacing_medium();
  }
}

void HaDeckClimate::unmount() {
  for (auto *option : this->primary_options_)
    option->unmount();
  for (auto *group : this->additional_groups_)
    for (auto *option : group->options())
      option->unmount();
  this->overlay_.unmount();
  this->climate_arc_.unmount();
  this->root_ = this->top_area_ = this->bottom_controls_ = this->primary_controls_ = nullptr;
  this->current_label_ = this->target_row_ = this->target_value_label_ = this->target_units_label_ = nullptr;
  this->adjust_controls_ = nullptr;
  this->minus_button_ = this->plus_button_ = this->power_button_ = this->power_label_ = nullptr;
  this->menu_button_ = this->menu_label_ = this->overlay_close_button_ = nullptr;
  this->overlay_titles_.clear();
  this->overlay_option_rows_.clear();
  this->initialized_ = false;
  this->reset_widget_state_();
}

void HaDeckClimate::apply_theme(HaDeckTheme *theme) {
  if (this->root_ == nullptr || theme == nullptr)
    return;
  this->theme_ = theme;
  this->apply_layout_metrics_(theme);
  this->climate_arc_.apply_theme(theme);
  for (auto *label : {this->current_label_, this->target_value_label_, this->target_units_label_})
    lv_obj_set_style_text_color(label, lv_color_hex(theme->on_surface_color()), MAIN_DEFAULT);
#ifdef USE_LVGL_FONT
  if (theme->text_small_font() != nullptr) {
    lv_obj_set_style_text_font(this->current_label_, theme->text_small_font()->get_lv_font(), MAIN_DEFAULT);
    lv_obj_set_style_text_font(this->target_units_label_, theme->text_small_font()->get_lv_font(), MAIN_DEFAULT);
  }
  if (theme->text_large_font() != nullptr)
    lv_obj_set_style_text_font(this->target_value_label_, theme->text_large_font()->get_lv_font(), MAIN_DEFAULT);
#endif
  for (auto *button : {this->minus_button_, this->plus_button_, this->menu_button_}) {
    if (button == nullptr)
      continue;
    const auto palette = apply_glass_button_surface(button, theme, this->accent_, theme->disabled_opacity());
    auto *label = lv_obj_get_child(button, 0);
    apply_glass_text_content(label, palette);
#ifdef USE_LVGL_FONT
    auto *control_font = button == this->menu_button_ ? theme->icon_small_font() : theme->text_medium_font();
    if (control_font != nullptr)
      lv_obj_set_style_text_font(label, control_font->get_lv_font(), MAIN_DEFAULT);
#endif
  }
  this->apply_power_theme_();
#ifdef USE_LVGL_FONT
  if (theme->icon_medium_font() != nullptr)
    lv_obj_set_style_text_font(this->power_label_, theme->icon_medium_font()->get_lv_font(), MAIN_DEFAULT);
#endif
  for (auto *option : this->primary_options_)
    option->apply_theme(theme);
  for (auto *group : this->additional_groups_)
    for (auto *option : group->options())
      option->apply_theme(theme);
  this->overlay_.apply_theme(theme);
  for (auto *title : this->overlay_titles_) {
    lv_obj_set_style_text_color(title, lv_color_hex(theme->on_surface_color()), MAIN_DEFAULT);
#ifdef USE_LVGL_FONT
    if (theme->text_small_font() != nullptr)
      lv_obj_set_style_text_font(title, theme->text_small_font()->get_lv_font(), MAIN_DEFAULT);
#endif
  }
  if (this->overlay_close_button_ != nullptr) {
    const auto palette = apply_glass_button_surface(this->overlay_close_button_, theme, this->accent_,
                                                     theme->disabled_opacity());
    auto *label = lv_obj_get_child(this->overlay_close_button_, 0);
    apply_glass_text_content(label, palette);
#ifdef USE_LVGL_FONT
    if (theme->icon_small_font() != nullptr)
      lv_obj_set_style_text_font(label, theme->icon_small_font()->get_lv_font(), MAIN_DEFAULT);
#endif
  }
  this->layout_overlay_(theme);
  this->layout_content_();
}

const std::string &HaDeckClimate::mode_accent_(const std::string &mode) const {
  if (!this->power_accent_.empty())
    return this->power_accent_;
  if (mode == "heating" && !this->heating_accent_.empty())
    return this->heating_accent_;
  if (mode == "drying" && !this->drying_accent_.empty())
    return this->drying_accent_;
  return this->accent_;
}

void HaDeckClimate::apply_power_theme_() {
  if (this->power_button_ == nullptr || this->theme_ == nullptr)
    return;
  const auto palette = apply_glass_button_surface(this->power_button_, this->theme_,
                                                   this->mode_accent_(this->arc_mode_state_),
                                                   this->theme_->disabled_opacity());
  apply_glass_text_content(this->power_label_, palette);
}

void HaDeckClimate::request_target_(float value) {
  value = std::max(this->min_temperature_, std::min(this->max_temperature_, value));
  value = std::round(value / this->step_) * this->step_;
  this->pending_target_ = value;
  this->pending_since_ = millis();
  this->pending_ = true;
  this->target_change_trigger_.trigger(value);
  this->update_state_(true);
}

void HaDeckClimate::update_state_(bool force) {
  if (this->root_ == nullptr)
    return;
  if (!this->visible_state_)
    this->overlay_.close();
  const float source_target = this->target_temperature_.value();
  if (this->pending_) {
    if ((!std::isnan(source_target) && std::fabs(source_target - this->pending_target_) < this->step_ / 2.0f) ||
        millis() - this->pending_since_ >= this->optimistic_timeout_)
      this->pending_ = false;
  }
  const float target = this->pending_ ? this->pending_target_ : source_target;
  const float current = this->current_temperature_.value();
  const bool disabled = this->disabled_.value();
  const bool power = this->power_state_.value();
  const bool power_visible = this->power_visible_.value();
  const std::string mode = this->arc_mode_.value();
  const bool heating = mode == "heating";
  const bool drying = mode == "drying";
  if (force || !this->initialized_ || mode != this->arc_mode_state_) {
    this->arc_mode_state_ = mode;
    this->apply_power_theme_();
  }
  this->climate_arc_.update(current, target, power, heating, drying, force);

  if (force || !this->initialized_ || !same_float_state(target, this->displayed_target_)) {
    char buffer[48];
    if (std::isnan(target))
      std::snprintf(buffer, sizeof(buffer), "−");
    else {
      std::snprintf(buffer, sizeof(buffer), this->format_.c_str(), target);
    }
    lv_label_set_text(this->target_value_label_, buffer);
    this->displayed_target_ = target;
    this->layout_content_();
  }
  if (force || !this->initialized_ || !same_float_state(current, this->displayed_current_)) {
    char buffer[48];
    if (std::isnan(current))
      std::snprintf(buffer, sizeof(buffer), "−%s", this->units_.c_str());
    else {
      char number[24];
      std::snprintf(number, sizeof(number), this->format_.c_str(), current);
      std::snprintf(buffer, sizeof(buffer), "%s%s", number, this->units_.c_str());
    }
    lv_label_set_text(this->current_label_, buffer);
    this->displayed_current_ = current;
    this->layout_content_();
  }

  if (force || !this->initialized_ || disabled != this->disabled_state_) {
    this->disabled_state_ = disabled;
    for (auto *button : {this->minus_button_, this->plus_button_}) {
      if (disabled)
        lv_obj_add_state(button, LV_STATE_DISABLED);
      else
        lv_obj_remove_state(button, LV_STATE_DISABLED);
    }
    if (disabled) {
      lv_obj_add_state(this->power_button_, LV_STATE_DISABLED);
      if (this->menu_button_ != nullptr)
        lv_obj_add_state(this->menu_button_, LV_STATE_DISABLED);
    } else {
      lv_obj_remove_state(this->power_button_, LV_STATE_DISABLED);
      if (this->menu_button_ != nullptr)
        lv_obj_remove_state(this->menu_button_, LV_STATE_DISABLED);
    }
    for (auto *option : this->primary_options_)
      option->set_disabled_state(disabled);
    for (auto *group : this->additional_groups_)
      for (auto *option : group->options())
        option->set_disabled_state(disabled);
  }
  if (force || !this->initialized_ || power != this->power_state_value_) {
    this->power_state_value_ = power;
    if (power) {
      lv_obj_add_state(this->power_button_, LV_STATE_CHECKED);
      lv_obj_add_state(this->power_label_, LV_STATE_CHECKED);
    } else {
      lv_obj_remove_state(this->power_button_, LV_STATE_CHECKED);
      lv_obj_remove_state(this->power_label_, LV_STATE_CHECKED);
    }
  }
  if (force || !this->initialized_ || power_visible != this->power_visible_state_) {
    this->power_visible_state_ = power_visible;
    if (power_visible)
      lv_obj_remove_flag(this->power_button_, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(this->power_button_, LV_OBJ_FLAG_HIDDEN);
  }
  for (auto *option : this->primary_options_)
    option->update(force);
  for (auto *group : this->additional_groups_)
    for (auto *option : group->options())
      option->update(force);
  this->initialized_ = true;
}

void HaDeckClimate::event_callback_(lv_event_t *event) {
  auto *climate = static_cast<HaDeckClimate *>(lv_event_get_user_data(event));
  auto *target = static_cast<lv_obj_t *>(lv_event_get_target(event));
  if (target == climate->minus_button_ && !climate->disabled_state_) {
    const float base = climate->pending_ ? climate->pending_target_ : climate->target_temperature_.value();
    if (!std::isnan(base))
      climate->request_target_(base - climate->step_);
  } else if (target == climate->plus_button_ && !climate->disabled_state_) {
    const float base = climate->pending_ ? climate->pending_target_ : climate->target_temperature_.value();
    if (!std::isnan(base))
      climate->request_target_(base + climate->step_);
  } else if (target == climate->power_button_ && !climate->disabled_state_) {
    if (climate->power_state_value_)
      climate->turn_off_trigger_.trigger();
    else
      climate->turn_on_trigger_.trigger();
  } else if (target == climate->menu_button_ && !climate->disabled_state_) {
    climate->overlay_.open();
  } else if (target == climate->overlay_close_button_) {
    climate->overlay_.close();
  }
}

}  // namespace esphome::ha_deck
