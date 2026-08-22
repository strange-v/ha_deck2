#pragma once

#include <string>
#include <vector>

#include "esphome/components/lvgl/lvgl_esphome.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include "button.h"
#include "climate.h"
#include "ha_state_utils.h"
#include "navigation_button.h"
#include "screen.h"
#include "slider.h"
#include "theme.h"
#include "value_display.h"
#include "weather.h"

namespace esphome::ha_deck {

class HaDeck : public PollingComponent {
 public:
  explicit HaDeck(lvgl::LvglComponent *lvgl) : lvgl_(lvgl) {}

  void add_screen(HaDeckScreen *screen) {
    screen->set_parent(this);
    this->screens_.push_back(screen);
  }
  void add_theme(HaDeckTheme *theme) {
    theme->set_parent(this);
    this->themes_.push_back(theme);
  }
  void set_default_screen(HaDeckScreen *screen) { this->default_screen_ = screen; }
  void set_default_theme(HaDeckTheme *theme) { this->default_theme_ = theme; }
  void set_screen_timeout(uint32_t timeout) { this->screen_timeout_ = timeout; }

  bool switch_screen(const std::string &name, lv_screen_load_anim_t animation = LV_SCREEN_LOAD_ANIM_NONE,
                     uint32_t time = 0);
  bool switch_screen(HaDeckScreen *screen, lv_screen_load_anim_t animation = LV_SCREEN_LOAD_ANIM_NONE,
                     uint32_t time = 0);
  bool go_back(lv_screen_load_anim_t animation = LV_SCREEN_LOAD_ANIM_NONE, uint32_t time = 0);
  bool set_theme(const std::string &name);
  bool set_theme(HaDeckTheme *theme);

  HaDeckScreen *get_active_screen() const { return this->active_screen_; }
  HaDeckTheme *get_active_theme() const { return this->active_theme_; }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR - 5.0f; }

 protected:
  lvgl::LvglComponent *lvgl_;
  std::vector<HaDeckScreen *> screens_{};
  std::vector<HaDeckTheme *> themes_{};
  HaDeckScreen *default_screen_{nullptr};
  HaDeckTheme *default_theme_{nullptr};
  HaDeckScreen *active_screen_{nullptr};
  HaDeckTheme *active_theme_{nullptr};
  std::vector<HaDeckScreen *> navigation_history_{};
  uint32_t screen_timeout_{30000};
  uint32_t screen_activated_at_{0};
  bool navigating_back_{false};
};

template<typename... Ts> class ShowScreenAction : public Action<Ts...> {
 public:
  void set_screen(HaDeckScreen *screen) { this->screen_ = screen; }
  void set_animation(lv_screen_load_anim_t animation) { this->animation_ = animation; }
  void set_time(uint32_t time) { this->time_ = time; }

 protected:
  void play(const Ts &...x) override {
    if (this->screen_ != nullptr)
      this->screen_->show(this->animation_, this->time_);
  }

  HaDeckScreen *screen_{nullptr};
  lv_screen_load_anim_t animation_{LV_SCREEN_LOAD_ANIM_NONE};
  uint32_t time_{0};
};

template<typename... Ts> class SetThemeAction : public Action<Ts...> {
 public:
  void set_theme(HaDeckTheme *theme) { this->theme_ = theme; }

 protected:
  void play(const Ts &...x) override {
    if (this->theme_ != nullptr)
      this->theme_->activate();
  }

  HaDeckTheme *theme_{nullptr};
};

}  // namespace esphome::ha_deck

