#pragma once
#include <vector>

namespace edge {

struct UplinkState {
  bool usable = false;
  int capacity_samples_per_min = 0;
};

struct MeshLink {
  int gateway_id = -1;
  bool link_ok = false;
  int capacity_samples_per_min = 0;
};

struct NodeView {
  int node_id = -1;
  UplinkState eth;
  UplinkState lte;
  std::vector<MeshLink> mesh_links;
};

struct WorldStep {
  int minute_idx = 0;
  std::vector<NodeView> nodes;
};

class WorldModel {
 public:
  void set_steps(const std::vector<WorldStep>& steps);
  WorldStep step_at(int minute_idx) const;

 private:
  std::vector<WorldStep> steps_;
};

} // namespace edge
