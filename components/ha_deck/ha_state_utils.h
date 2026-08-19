#pragma once

namespace esphome::ha_deck {

/** Return whether a Home Assistant text entity has a usable state. */
template<typename Entity> inline bool ha_is_available(const Entity *entity) {
  if (entity == nullptr || !entity->has_state())
    return false;

  const auto &state = entity->state;
  return !state.empty() && state != "unavailable" && state != "unknown";
}

/** Return whether an available Home Assistant text entity matches any supplied state. */
template<typename Entity, typename... States> inline bool ha_state_in(const Entity *entity, const States &...states) {
  return ha_is_available(entity) && ((entity->state == states) || ...);
}

/** Return whether an available Home Assistant text entity matches none of the supplied states. */
template<typename Entity, typename... States>
inline bool ha_state_not_in(const Entity *entity, const States &...states) {
  return ha_is_available(entity) && !((entity->state == states) || ...);
}

}  // namespace esphome::ha_deck

// ESPHome compiles YAML lambdas inside the esphome namespace. Re-export the
// helpers there so dashboard expressions can stay concise.
namespace esphome {
using ha_deck::ha_is_available;
using ha_deck::ha_state_in;
using ha_deck::ha_state_not_in;
}  // namespace esphome
