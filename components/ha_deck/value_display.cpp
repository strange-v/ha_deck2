#include "value_display.h"

#include <cstdio>
#include <algorithm>

#include "numeric_utils.h"

namespace esphome::ha_deck {

static constexpr lv_style_selector_t MAIN_DEFAULT =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT);

void ValueDisplayWidget::mount(lv_obj_t *parent, HaDeckTheme *theme) {
  this->root_ = lv_obj_create(parent);
  lv_obj_set_pos(this->root_, this->x_, this->y_);
  lv_obj_set_size(this->root_, this->width_, this->height_);
  lv_obj_remove_flag(this->root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(this->root_, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->root_, 0, MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->root_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_layout(this->root_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->root_, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(this->root_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

  if (!this->top_text_.empty()) {
    this->top_label_ = lv_label_create(this->root_);
    lv_label_set_text(this->top_label_, this->top_text_.c_str());
  }

  this->row_ = lv_obj_create(this->root_);
  lv_obj_set_size(this->row_, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_remove_flag(this->row_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(this->row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->row_, 4, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->row_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_layout(this->row_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->row_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->row_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  if (this->icon_image_ != nullptr) {
    this->icon_ = lv_image_create(this->row_);
    lv_image_set_src(this->icon_, this->icon_image_->get_lv_image_dsc());
  } else if (!this->icon_glyph_.empty()) {
    this->icon_ = lv_label_create(this->row_);
    lv_obj_set_style_text_align(this->icon_, LV_TEXT_ALIGN_CENTER, MAIN_DEFAULT);
    lv_label_set_text(this->icon_, this->icon_glyph_.c_str());
  }

  this->value_row_ = lv_obj_create(this->row_);
  lv_obj_set_size(this->value_row_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_remove_flag(this->value_row_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(this->value_row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->value_row_, 4, MAIN_DEFAULT);
  lv_obj_set_style_border_width(this->value_row_, 0, MAIN_DEFAULT);
  lv_obj_set_style_bg_opa(this->value_row_, LV_OPA_TRANSP, MAIN_DEFAULT);
  lv_obj_set_layout(this->value_row_, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(this->value_row_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(this->value_row_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

  this->value_label_ = lv_label_create(this->value_row_);
  lv_label_set_text(this->value_label_, this->unavailable_text_.c_str());
  if (!this->units_.empty()) {
    this->units_label_ = lv_label_create(this->value_row_);
    lv_label_set_text(this->units_label_, this->units_.c_str());
  }

  if (!this->bottom_text_.empty()) {
    this->bottom_label_ = lv_label_create(this->root_);
    lv_label_set_text(this->bottom_label_, this->bottom_text_.c_str());
  }

  this->apply_theme(theme);
  this->value_initialized_ = false;
  this->update(true);
}

void ValueDisplayWidget::unmount() {
  this->root_ = nullptr;
  this->top_label_ = nullptr;
  this->row_ = nullptr;
  this->icon_ = nullptr;
  this->value_row_ = nullptr;
  this->value_label_ = nullptr;
  this->units_label_ = nullptr;
  this->bottom_label_ = nullptr;
  this->value_initialized_ = false;
  this->reset_widget_state_();
}

void ValueDisplayWidget::apply_theme(HaDeckTheme *theme) {
  if (this->root_ == nullptr || theme == nullptr)
    return;
  uint32_t accent;
  uint32_t on_accent;
  theme->resolve_accent(this->accent_, &accent, &on_accent);
  for (auto *label : {this->top_label_, this->value_label_, this->units_label_, this->bottom_label_}) {
    if (label != nullptr)
      lv_obj_set_style_text_color(label, lv_color_hex(theme->on_surface_color()), MAIN_DEFAULT);
  }
  if (this->icon_ != nullptr && this->icon_image_ == nullptr)
    lv_obj_set_style_text_color(this->icon_, lv_color_hex(accent), MAIN_DEFAULT);

  if (this->value_label_ != nullptr) {
    auto *font = this->value_font_ != nullptr ? this->value_font_ : theme->text_medium_font();
    if (font != nullptr)
      lv_obj_set_style_text_font(this->value_label_, font->get_lv_font(), MAIN_DEFAULT);
  }
  if (this->icon_ != nullptr && this->icon_image_ == nullptr) {
    auto *font = this->icon_font_ != nullptr ? this->icon_font_ : theme->icon_medium_font();
    if (font != nullptr)
      lv_obj_set_style_text_font(this->icon_, font->get_lv_font(), MAIN_DEFAULT);
  }
  auto *text_font = this->text_font_ != nullptr ? this->text_font_ : theme->text_small_font();
  if (text_font != nullptr) {
    for (auto *label : {this->top_label_, this->units_label_, this->bottom_label_}) {
      if (label != nullptr)
        lv_obj_set_style_text_font(label, text_font->get_lv_font(), MAIN_DEFAULT);
    }
  }
  const uint16_t content_gap = std::max<uint16_t>(1, theme->spacing_small() / 2);
  lv_obj_set_style_pad_column(this->row_, content_gap, MAIN_DEFAULT);
  lv_obj_set_style_pad_column(this->value_row_, content_gap, MAIN_DEFAULT);
  this->update_icon_slot_();
  this->align_units_baseline_();
}

void ValueDisplayWidget::update_icon_slot_() {
  if (this->icon_ == nullptr)
    return;

  if (this->icon_image_ != nullptr) {
    lv_obj_set_size(this->icon_, this->icon_image_->get_width(), this->icon_image_->get_height());
    return;
  }

  const lv_font_t *icon_font = lv_obj_get_style_text_font(this->icon_, LV_PART_MAIN);
  if (icon_font != nullptr) {
    // Material icon fonts are square by design. line_height follows the actual
    // configured ESPHome font size and keeps every glyph in an identical slot.
    lv_obj_set_size(this->icon_, icon_font->line_height, icon_font->line_height);
  }
}

void ValueDisplayWidget::align_units_baseline_() {
  if (this->value_label_ == nullptr || this->units_label_ == nullptr)
    return;

  const lv_font_t *value_font = lv_obj_get_style_text_font(this->value_label_, LV_PART_MAIN);
  const lv_font_t *units_font = lv_obj_get_style_text_font(this->units_label_, LV_PART_MAIN);
  if (value_font == nullptr || units_font == nullptr)
    return;

  // The flex row aligns the label boxes at the bottom. Font base_line is the
  // distance from that bottom edge to the typographic baseline, so translating
  // by their difference aligns the glyph baselines for arbitrary font sizes.
  const int32_t translate_y = units_font->base_line - value_font->base_line;
  lv_obj_set_style_translate_y(this->units_label_, translate_y, MAIN_DEFAULT);
}

void ValueDisplayWidget::update_state_(bool force) {
  const float value = this->value_.value();
  if (force || !this->value_initialized_ || !same_float_state(value, this->rendered_value_)) {
    this->rendered_value_ = value;
    this->value_initialized_ = true;
    if (std::isnan(value)) {
      lv_label_set_text(this->value_label_, this->unavailable_text_.c_str());
    } else {
      char buffer[32];
      std::snprintf(buffer, sizeof(buffer), this->format_.c_str(), static_cast<double>(value));
      lv_label_set_text(this->value_label_, buffer);
    }
  }
  this->update_extra_(force);
}

void ValueDisplayWidget::set_dynamic_image_(image::Image *image) {
  if (this->icon_ != nullptr && image != nullptr)
    lv_image_set_src(this->icon_, image->get_lv_image_dsc());
}

}  // namespace esphome::ha_deck
