#include "probe/probe_fake.h"

namespace edge {

void FakeProbe::set_node_sequences(int node_id, const std::vector<PerMinuteProbe>& seq) {
  if ((int)seq_by_node_.size() <= node_id) seq_by_node_.resize(node_id + 1);
  seq_by_node_[node_id] = seq;
}

ProbeResult FakeProbe::probe(int node_id, LinkType link, int /*gateway_id*/, int minute_idx) {
  if (link == LinkType::Mesh) {
    ProbeResult r;
    r.dns_ok = mesh_ok_;
    r.tcp_ok = mesh_ok_;
    r.ping_ok = mesh_ok_;
    return r;
  }
  if (node_id < 0 || node_id >= (int)seq_by_node_.size()) return ProbeResult{};
  const auto& seq = seq_by_node_[node_id];
  if (seq.empty()) return ProbeResult{};
  if (minute_idx < 0) minute_idx = 0;
  if (minute_idx >= (int)seq.size()) minute_idx = (int)seq.size() - 1;
  if (link == LinkType::Ethernet) return seq[minute_idx].eth;
  return seq[minute_idx].lte;
}

} // namespace edge
