#include "sim/world.h"

namespace edge {

void WorldModel::set_steps(const std::vector<WorldStep>& steps) { steps_ = steps; }

WorldStep WorldModel::step_at(int minute_idx) const {
  if (steps_.empty()) return WorldStep{};
  if (minute_idx < 0) minute_idx = 0;
  if (minute_idx >= (int)steps_.size()) return steps_.back();
  return steps_[minute_idx];
}

} // namespace edge
