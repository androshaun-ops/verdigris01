#pragma once
#include "probe/probe.h"
#include "sim/world.h"

namespace edge {

struct Choice {
  LinkType type = LinkType::Ethernet;
  int gateway_id = -1;
};

struct Config {
  int consecutive_fail_to_mark_bad = 1;
  int min_switch_interval_min = 2;
  int min_gateway_change_interval_min = 3;
};

class ConnMgr {
 public:
  ConnMgr(int node_id, IProbe& probe, Config cfg);

  Choice step(int minute_idx, const NodeView& view);

  Choice current_choice() const { return choice_; }
  int switch_count() const { return switch_count_; }
  int gateway_change_count() const { return gateway_change_count_; }

 private:
  int node_id_;
  IProbe& probe_;
  Config cfg_;

  Choice choice_{};

  int eth_fail_streak_ = 0;
  int lte_fail_streak_ = 0;

  int last_switch_minute_ = -100000;
  int last_gateway_change_minute_ = -100000;

  int switch_count_ = 0;
  int gateway_change_count_ = 0;

  bool probe_good(const ProbeResult& r) const;
  bool in_switch_hold(int minute_idx) const;
  bool in_gateway_hold(int minute_idx) const;

  static int score(LinkType t);
};

} // namespace edge
