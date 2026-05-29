#include "sim/simulator.h"
#include <algorithm>
#include <vector>

namespace edge {

Simulator::Simulator(IProbe& probe, WorldModel world, SimConfig cfg, Config conn_cfg)
  : probe_(probe), world_(world), cfg_(cfg), conn_cfg_(conn_cfg) {}

SimResult Simulator::run() {
  std::vector<FirmwareNode> nodes;
  nodes.reserve(cfg_.nodes);
  for (int i = 0; i < cfg_.nodes; i++) {
    nodes.emplace_back(i, probe_, conn_cfg_);
  }

  int expected_total = cfg_.nodes * cfg_.minutes * cfg_.sample_per_minute;

  for (int t = 0; t < cfg_.minutes; t++) {
    WorldStep ws = world_.step_at(t);

    // Phase 1: ISR fires — allocate ring buffer slot
    for (int i = 0; i < cfg_.nodes; i++) {
      for (int s = 0; s < cfg_.sample_per_minute; s++) {
        nodes[i].isr_begin_sample();
      }
    }

    // Phase 2: Upload task begins — capture current active link
    for (int i = 0; i < cfg_.nodes; i++) {
      nodes[i].upload_begin();
    }

    // Phase 3: Probe task runs — update ETH probes
    for (int i = 0; i < cfg_.nodes; i++) {
      nodes[i].probe_update_eth(t);
    }

    // Phase 4: ConnMgr task runs — select link using current probe snapshot
    std::vector<Choice> choices(cfg_.nodes);
    for (int i = 0; i < cfg_.nodes; i++) {
      choices[i] = nodes[i].conn_mgr_step(t, ws.nodes[i]);
    }

    // Phase 5: Probe task resumes — update LTE probes
    for (int i = 0; i < cfg_.nodes; i++) {
      nodes[i].probe_update_lte(t);
    }

    // Phase 6: Upload task resumes — drain ring buffer over captured link
    std::vector<int> gw_remaining(cfg_.nodes, 0);

    for (int i = 0; i < cfg_.nodes; i++) {
      if (choices[i].type == LinkType::Mesh) continue;
      int sent = nodes[i].upload_execute(ws.nodes[i]);
      int cap = 0;
      if (choices[i].type == LinkType::Ethernet && ws.nodes[i].eth.usable) {
        cap = ws.nodes[i].eth.capacity_samples_per_min;
      } else if (choices[i].type == LinkType::LTE && ws.nodes[i].lte.usable) {
        cap = ws.nodes[i].lte.capacity_samples_per_min;
      }
      gw_remaining[i] = std::max(0, cap - sent);
    }

    for (int i = 0; i < cfg_.nodes; i++) {
      if (choices[i].type != LinkType::Mesh) continue;
      nodes[i].upload_execute(ws.nodes[i]);
    }

    // Phase 7: ISR bottom-half — commit sample data to ring buffer
    for (int i = 0; i < cfg_.nodes; i++) {
      for (int s = 0; s < cfg_.sample_per_minute; s++) {
        nodes[i].isr_commit_sample(t);
      }
    }
  }

  int received_total = 0;
  int max_backlog = 0;
  int switches = 0;
  int gw_changes = 0;

  for (int i = 0; i < cfg_.nodes; i++) {
    received_total += nodes[i].delivered();
    max_backlog = std::max(max_backlog, nodes[i].backlog());
    switches += nodes[i].switch_count();
    gw_changes += nodes[i].gateway_change_count();
  }

  SimResult r;
  r.metrics.fleet_completeness = expected_total == 0 ? 0.0
      : static_cast<double>(received_total) / static_cast<double>(expected_total);
  r.metrics.max_node_backlog = max_backlog;
  r.metrics.switch_count = switches;
  r.metrics.gateway_change_count = gw_changes;
  return r;
}

} // namespace edge
