#pragma once
#include <cstdint>
#include <string>

namespace edge {

struct FleetMetrics {
  double fleet_completeness = 0.0;
  int64_t max_node_backlog = 0;
  int switch_count = 0;
  int gateway_change_count = 0;

  std::string to_string() const;
};

} // namespace edge
