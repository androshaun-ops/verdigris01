#pragma once
#include "probe/probe.h"
#include <vector>

namespace edge {

struct PerMinuteProbe {
  ProbeResult eth;
  ProbeResult lte;
};

class FakeProbe final : public IProbe {
 public:
  void set_node_sequences(int node_id, const std::vector<PerMinuteProbe>& seq);
  void set_mesh_probe_ok(bool ok) { mesh_ok_ = ok; }
  ProbeResult probe(int node_id, LinkType link, int gateway_id, int minute_idx) override;

 private:
  std::vector<std::vector<PerMinuteProbe>> seq_by_node_;
  bool mesh_ok_ = true;
};

} // namespace edge
