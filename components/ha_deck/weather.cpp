#include "weather.h"

#include <algorithm>
#include <cctype>

namespace esphome::ha_deck {

std::string HaDeckWeather::normalize_(std::string condition) {
  std::transform(condition.begin(), condition.end(), condition.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  std::replace(condition.begin(), condition.end(), '_', '-');
  condition.erase(std::remove_if(condition.begin(), condition.end(),
                                 [](unsigned char c) { return std::isspace(c); }),
                  condition.end());
  if (condition == "partly-cloudy")
    condition = "partlycloudy";
  if (condition == "snowy-rainy" || condition == "sleet")
    condition = "snowy-rainy";
  return condition;
}

image::Image *HaDeckWeather::find_image_(const std::string &condition) const {
  for (const auto &entry : this->images_) {
    if (entry.condition == condition)
      return entry.image;
  }
  for (const auto &entry : this->images_) {
    if (entry.condition == "exceptional")
      return entry.image;
  }
  return nullptr;
}

void HaDeckWeather::update_extra_(bool force) {
  auto condition = normalize_(this->condition_.value());
  const bool night = this->is_night_.value();
  if (condition == "sunny" || condition == "clear" || condition == "clear-day")
    condition = night ? "clear-night" : "clear-day";
  else if (condition == "partlycloudy")
    condition = night ? "partly-cloudy-night" : "partly-cloudy-day";
  else if (condition == "rainy")
    condition = "rain";
  else if (condition == "snowy")
    condition = "snow";
  else if (condition == "windy")
    condition = "wind";

  if (!force && condition == this->rendered_condition_)
    return;
  this->rendered_condition_ = condition;
  this->set_dynamic_image_(this->find_image_(condition));
}

}  // namespace esphome::ha_deck
