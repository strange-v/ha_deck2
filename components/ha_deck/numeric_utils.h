#pragma once

#include <cmath>

namespace esphome::ha_deck {

inline bool same_float_state(float left, float right) {
  return (std::isnan(left) && std::isnan(right)) || left == right;
}

}  // namespace esphome::ha_deck
