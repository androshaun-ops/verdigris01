#include "conn_mgr/conn_mgr.h"

namespace edge {

ConnMgr::ConnMgr(int node_id, IProbe& probe, Config cfg)
  : node_id_(node_id), probe_(probe), cfg_(cfg) {}

bool ConnMgr::probe_good(const ProbeResult& r) const {
  return r.dns_ok && r.tcp_ok && r.ping_ok;
}

bool ConnMgr::in_switch_hold(int minute_idx) const {
  return (minute_idx - last_switch_minute_) < cfg_.min_switch_interval_min;
}

bool ConnMgr::in_gateway_hold(int minute_idx) const {
  return (minute_idx - last_gateway_change_minute_) < cfg_.min_gateway_change_interval_min;
}

int ConnMgr::score(LinkType t) {
  if (t == LinkType::Ethernet) return 300;
  if (t == LinkType::Mesh) return 200;
  return 100;
}

Choice ConnMgr::step(int minute_idx, const NodeView& view) {
  {
    ProbeResult pr = probe_.probe(node_id_, LinkType::Ethernet, -1, minute_idx);
    bool good = probe_good(pr);
    eth_fail_streak_ = good ? 0 : (eth_fail_streak_ + 1);
  }
  {
    ProbeResult pr = probe_.probe(node_id_, LinkType::LTE, -1, minute_idx);
    bool good = probe_good(pr);
    lte_fail_streak_ = good ? 0 : (lte_fail_streak_ + 1);
  }

  bool eth_usable = view.eth.usable && (eth_fail_streak_ < cfg_.consecutive_fail_to_mark_bad);
  bool lte_usable = view.lte.usable && (lte_fail_streak_ < cfg_.consecutive_fail_to_mark_bad);

  int first_gw = -1;
  for (const auto& ml : view.mesh_links) {
    if (!ml.link_ok) continue;
    first_gw = ml.gateway_id;
    break;
  }
  bool mesh_usable = (first_gw >= 0);

  Choice best = choice_;
  int best_score = -1000000;

  auto consider = [&](LinkType t, int gw) {
    int s = score(t);
    if (s > best_score) {
      best_score = s;
      best.type = t;
      best.gateway_id = gw;
    }
  };

  if (eth_usable) consider(LinkType::Ethernet, -1);
  if (mesh_usable) consider(LinkType::Mesh, first_gw);
  if (lte_usable) consider(LinkType::LTE, -1);

  if (in_switch_hold(minute_idx) && (best.type != choice_.type)) {
    best = choice_;
  }

  if (best.type != choice_.type) {
    switch_count_++;
    last_switch_minute_ = minute_idx;
  }

  if (best.type == LinkType::Mesh && choice_.type == LinkType::Mesh && best.gateway_id != choice_.gateway_id) {
    if (!in_gateway_hold(minute_idx)) {
      gateway_change_count_++;
      last_gateway_change_minute_ = minute_idx;
    } else {
      best.gateway_id = choice_.gateway_id;
    }
  }

  choice_ = best;
  return choice_;
}

} // namespace edge
