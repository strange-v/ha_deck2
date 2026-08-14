#pragma once

#include <string>
#include <vector>

#include "value_display.h"

namespace esphome::ha_deck {

class HaDeckWeather : public ValueDisplayWidget {
 public:
  void set_condition(TemplatableValue<std::string> condition) { this->condition_ = condition; }
  void set_is_night(TemplatableValue<bool> is_night) { this->is_night_ = is_night; }
  void add_condition_image(const std::string &condition, image::Image *image) {
    this->images_.push_back({condition, image});
    if (this->icon_image_ == nullptr)
      this->icon_image_ = image;
  }

 protected:
  struct ConditionImage {
    std::string condition;
    image::Image *image;
  };

  void update_extra_(bool force) override;
  image::Image *find_image_(const std::string &condition) const;
  static std::string normalize_(std::string condition);

  TemplatableValue<std::string> condition_{std::string("unknown")};
  TemplatableValue<bool> is_night_{false};
  std::vector<ConditionImage> images_{};
  std::string rendered_condition_{};
};

}  // namespace esphome::ha_deck
