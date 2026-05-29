#pragma once
#include "common/metrics.h"
#include "conn_mgr/conn_mgr.h"
#include "fw/firmware_node.h"
#include "probe/probe.h"
#include "sim/world.h"
#include <vector>

namespace edge {

struct SimConfig {
  int nodes = 6;
  int minutes = 60;
  int storage_limit_samples = 120;
  int sample_per_minute = 1;
};

struct SimResult {
  FleetMetrics metrics;
};

class Simulator {
 public:
  Simulator(IProbe& probe, WorldModel world, SimConfig cfg, Config conn_cfg);
  SimResult run();

 private:
  IProbe& probe_;
  WorldModel world_;
  SimConfig cfg_;
  Config conn_cfg_;
};

} // namespace edge
