#include "ha_deck.h"

#include "esphome/core/log.h"

namespace esphome::ha_deck {

static const char *const TAG = "ha_deck";

void HaDeckTheme::activate() {
  if (this->parent_ != nullptr)
    this->parent_->set_theme(this);
}

void HaDeck::setup() {
  if (this->default_screen_ == nullptr || this->default_theme_ == nullptr) {
    ESP_LOGE(TAG, "A default screen and default theme are required");
    this->mark_failed();
    return;
  }

  lv_display_set_default(this->lvgl_->get_disp());
  this->active_theme_ = this->default_theme_;
  this->lvgl_->add_on_idle_callback([this](uint32_t idle_time) {
    if (this->active_screen_ == nullptr || this->active_screen_ == this->default_screen_ ||
        this->active_screen_->is_persistent() || idle_time < this->screen_timeout_ ||
        millis() - this->screen_activated_at_ < this->screen_timeout_)
      return;

    ESP_LOGD(TAG, "Screen timeout; returning to default screen");
    this->navigation_history_.clear();
    this->navigating_back_ = true;
    this->switch_screen(this->default_screen_);
    this->navigating_back_ = false;
  });
  if (!this->switch_screen(this->default_screen_)) {
    ESP_LOGE(TAG, "Unable to show default screen");
    this->mark_failed();
  }
}

bool HaDeck::switch_screen(const std::string &name, lv_screen_load_anim_t animation, uint32_t time) {
  for (auto *screen : this->screens_) {
    if (screen->get_name() == name)
      return this->switch_screen(screen, animation, time);
  }
  ESP_LOGW(TAG, "Screen '%s' was not found", name.c_str());
  return false;
}

bool HaDeck::switch_screen(HaDeckScreen *screen, lv_screen_load_anim_t animation, uint32_t time) {
  if (screen == nullptr)
    return false;
  if (screen == this->active_screen_)
    return true;

  lv_display_set_default(this->lvgl_->get_disp());
  screen->mount(this->active_theme_);
  // The default screen stays mounted while transient screens are active, so it
  // must receive theme changes that happened while it was in the background.
  screen->apply_theme(this->active_theme_);
  screen->update(true);

  auto *previous_screen = this->active_screen_;
  auto *previous_obj =
      previous_screen != nullptr ? previous_screen->get_obj() : this->lvgl_->get_screen_active();
  auto *target_obj = screen->get_obj();
  const bool delete_previous = previous_screen == nullptr || previous_screen != this->default_screen_;
  this->active_screen_ = screen;
  this->screen_activated_at_ = millis();
  if (!this->navigating_back_ && previous_screen != nullptr)
    this->navigation_history_.push_back(previous_screen);

  ESP_LOGD(TAG, "Switching to screen '%s'", screen->get_name().c_str());
  if (animation == LV_SCREEN_LOAD_ANIM_NONE || time == 0) {
    lv_screen_load(target_obj);
    if (delete_previous && previous_obj != nullptr && previous_obj != target_obj)
      lv_obj_delete(previous_obj);
  } else {
    lv_screen_load_anim(target_obj, animation, time, 0, delete_previous);
  }

  if (delete_previous && previous_screen != nullptr)
    previous_screen->unmount();

  return true;
}

bool HaDeck::go_back(lv_screen_load_anim_t animation, uint32_t time) {
  while (!this->navigation_history_.empty()) {
    auto *screen = this->navigation_history_.back();
    this->navigation_history_.pop_back();
    if (screen == nullptr || screen == this->active_screen_)
      continue;
    this->navigating_back_ = true;
    const bool result = this->switch_screen(screen, animation, time);
    this->navigating_back_ = false;
    return result;
  }
  if (this->default_screen_ != nullptr && this->default_screen_ != this->active_screen_) {
    this->navigating_back_ = true;
    const bool result = this->switch_screen(this->default_screen_, animation, time);
    this->navigating_back_ = false;
    return result;
  }
  return false;
}

bool HaDeck::set_theme(const std::string &name) {
  for (auto *theme : this->themes_) {
    if (theme->get_name() == name)
      return this->set_theme(theme);
  }
  ESP_LOGW(TAG, "Theme '%s' was not found", name.c_str());
  return false;
}

bool HaDeck::set_theme(HaDeckTheme *theme) {
  if (theme == nullptr)
    return false;

  this->active_theme_ = theme;
  ESP_LOGD(TAG, "Applying theme '%s'", theme->get_name().c_str());
  if (this->active_screen_ != nullptr) {
    this->active_screen_->apply_theme(theme);
    this->active_screen_->update(true);
  }
  return true;
}

void HaDeck::update() {
  if (this->active_screen_ != nullptr)
    this->active_screen_->update();
}

void HaDeck::dump_config() {
  ESP_LOGCONFIG(TAG,
                "HA Deck:\n"
                "  Screens: %u\n"
                "  Themes: %u\n"
                "  Screen timeout: %u ms\n"
                "  Active screen: %s\n"
                "  Active theme: %s",
                static_cast<unsigned>(this->screens_.size()), static_cast<unsigned>(this->themes_.size()),
                static_cast<unsigned>(this->screen_timeout_),
                this->active_screen_ == nullptr ? "none" : this->active_screen_->get_name().c_str(),
                this->active_theme_ == nullptr ? "none" : this->active_theme_->get_name().c_str());
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::ha_deck

