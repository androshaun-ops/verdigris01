#include "common/metrics.h"
#include <sstream>

namespace edge {

std::string FleetMetrics::to_string() const {
  std::ostringstream oss;
  oss << "fleet_completeness=" << fleet_completeness
      << " max_node_backlog=" << max_node_backlog
      << " switch_count=" << switch_count
      << " gateway_change_count=" << gateway_change_count;
  return oss.str();
}

} // namespace edge
