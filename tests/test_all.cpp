#include "common/metrics.h"
#include "probe/probe_fake.h"
#include "sim/world.h"
#include "sim/simulator.h"

#include <cstdlib>
#include <iostream>
#include <vector>

struct TestCase { const char* name; void(*fn)(); };
static std::vector<TestCase>& registry(){ static std::vector<TestCase> r; return r; }
struct Register { Register(const char* n, void(*f)()){ registry().push_back({n,f}); } };

#define TEST(name) void name(); static Register reg_##name(#name, name); void name()
#define REQUIRE(cond) do { if(!(cond)) { \
    std::cerr << "REQUIRE failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    std::exit(1);} } while(0)

using namespace edge;

static std::vector<PerMinuteProbe> make_probes(int minutes) {
  std::vector<PerMinuteProbe> seq;
  seq.reserve(minutes);
  for (int t = 0; t < minutes; t++) {
    PerMinuteProbe p;
    p.eth.tcp_ok = true;
    p.eth.ping_ok = true;
    p.eth.dns_ok = (t % 10 != 0);
    bool lte_up = ((t % 8) < 6);
    p.lte.dns_ok = lte_up;
    p.lte.tcp_ok = lte_up;
    p.lte.ping_ok = lte_up;
    seq.push_back(p);
  }
  return seq;
}

static WorldModel build_world(int minutes, int nodes = 6) {
  WorldModel world;
  std::vector<WorldStep> steps;
  steps.reserve(minutes);
  for (int t = 0; t < minutes; t++) {
    WorldStep ws;
    ws.minute_idx = t;
    ws.nodes.resize(nodes);
    int eth_cap = (t >= 20 && t < 35) ? 1 : 3;
    int lte_cap = ((t % 8) < 6) ? 2 : 0;
    for (int i = 0; i < nodes; i++) {
      NodeView nv;
      nv.node_id = i;
      nv.eth.usable = true;
      nv.eth.capacity_samples_per_min = eth_cap;
      nv.lte.usable = ((t % 8) < 6);
      nv.lte.capacity_samples_per_min = lte_cap;
      int gw1 = (i + 1) % nodes;
      int gw2 = (i + 2) % nodes;
      nv.mesh_links.push_back(MeshLink{gw1, true, 3});
      nv.mesh_links.push_back(MeshLink{gw2, true, 3});
      ws.nodes[i] = nv;
    }
    steps.push_back(ws);
  }
  world.set_steps(steps);
  return world;
}

// Fleet should maintain high sample delivery under DNS noise and throttling.
TEST(fleet_completeness_should_be_high_under_noise_and_throttle) {
  const int minutes = 60;
  FakeProbe probe;
  for (int i = 0; i < 6; i++) probe.set_node_sequences(i, make_probes(minutes));
  WorldModel world = build_world(minutes);

  SimConfig sim_cfg;
  sim_cfg.nodes = 6;
  sim_cfg.minutes = minutes;
  sim_cfg.storage_limit_samples = 120;
  sim_cfg.sample_per_minute = 1;

  Config conn_cfg;

  Simulator sim(probe, world, sim_cfg, conn_cfg);
  SimResult r = sim.run();

  std::cout << "Metrics: " << r.metrics.to_string() << std::endl;

  REQUIRE(r.metrics.fleet_completeness >= 0.90);
  REQUIRE(r.metrics.switch_count <= 60);
}

// Nodes should failover to LTE when Ethernet is completely down.
TEST(failover_to_lte_when_ethernet_down) {
  const int minutes = 40;
  const int nodes = 3;

  FakeProbe probe;
  for (int i = 0; i < nodes; i++) {
    std::vector<PerMinuteProbe> seq;
    seq.reserve(minutes);
    for (int t = 0; t < minutes; t++) {
      PerMinuteProbe p;
      p.eth = ProbeResult{false, false, false};
      bool lte_up = ((t % 8) < 6);
      p.lte = ProbeResult{lte_up, lte_up, lte_up};
      seq.push_back(p);
    }
    probe.set_node_sequences(i, seq);
  }

  WorldModel world;
  std::vector<WorldStep> steps;
  steps.reserve(minutes);
  for (int t = 0; t < minutes; t++) {
    WorldStep ws;
    ws.minute_idx = t;
    ws.nodes.resize(nodes);
    bool lte_up = ((t % 8) < 6);
    for (int i = 0; i < nodes; i++) {
      NodeView nv;
      nv.node_id = i;
      nv.eth.usable = false;
      nv.eth.capacity_samples_per_min = 0;
      nv.lte.usable = lte_up;
      nv.lte.capacity_samples_per_min = lte_up ? 2 : 0;
      ws.nodes[i] = nv;
    }
    steps.push_back(ws);
  }
  world.set_steps(steps);

  SimConfig sim_cfg;
  sim_cfg.nodes = nodes;
  sim_cfg.minutes = minutes;
  sim_cfg.storage_limit_samples = 120;
  sim_cfg.sample_per_minute = 1;

  Config conn_cfg;

  Simulator sim(probe, world, sim_cfg, conn_cfg);
  SimResult r = sim.run();

  std::cout << "Failover metrics: " << r.metrics.to_string() << std::endl;

  REQUIRE(r.metrics.fleet_completeness >= 0.92);
  REQUIRE(r.metrics.switch_count <= nodes * 2);
}

// With perfect probes and capacity well above production rate, every sample
// should be delivered and no backlog should accumulate.
TEST(no_sample_loss_under_excess_capacity) {
  const int minutes = 30;
  const int nodes = 2;

  FakeProbe probe;
  for (int i = 0; i < nodes; i++) {
    std::vector<PerMinuteProbe> seq;
    for (int t = 0; t < minutes; t++) {
      PerMinuteProbe p;
      p.eth = ProbeResult{true, true, true};
      p.lte = ProbeResult{true, true, true};
      seq.push_back(p);
    }
    probe.set_node_sequences(i, seq);
  }

  WorldModel world;
  std::vector<WorldStep> steps;
  for (int t = 0; t < minutes; t++) {
    WorldStep ws;
    ws.minute_idx = t;
    ws.nodes.resize(nodes);
    for (int i = 0; i < nodes; i++) {
      NodeView nv;
      nv.node_id = i;
      nv.eth.usable = true;
      nv.eth.capacity_samples_per_min = 5;
      nv.lte.usable = true;
      nv.lte.capacity_samples_per_min = 3;
      ws.nodes[i] = nv;
    }
    steps.push_back(ws);
  }
  world.set_steps(steps);

  SimConfig sim_cfg;
  sim_cfg.nodes = nodes;
  sim_cfg.minutes = minutes;
  sim_cfg.storage_limit_samples = 120;
  sim_cfg.sample_per_minute = 1;

  Config conn_cfg;

  Simulator sim(probe, world, sim_cfg, conn_cfg);
  SimResult r = sim.run();

  std::cout << "No-loss metrics: " << r.metrics.to_string() << std::endl;

  REQUIRE(r.metrics.fleet_completeness >= 1.0);
  REQUIRE(r.metrics.max_node_backlog == 0);
}

// Mesh client's throughput should be strictly limited by the gateway's
// remaining uplink capacity. If the gateway has no remaining capacity,
// the mesh client cannot upload.
TEST(mesh_gateway_capacity_limit_enforced) {
  const int minutes = 10;
  const int nodes = 2;

  FakeProbe probe;
  for (int i = 0; i < nodes; i++) {
    std::vector<PerMinuteProbe> seq;
    for (int t = 0; t < minutes; t++) {
      PerMinuteProbe p;
      p.eth = ProbeResult{true, true, true};
      p.lte = ProbeResult{false, false, false};
      seq.push_back(p);
    }
    probe.set_node_sequences(i, seq);
  }

  WorldModel world;
  std::vector<WorldStep> steps;
  for (int t = 0; t < minutes; t++) {
    WorldStep ws;
    ws.minute_idx = t;
    ws.nodes.resize(nodes);

    // Node 0 (Gateway)
    NodeView nv0;
    nv0.node_id = 0;
    nv0.eth.usable = true;
    nv0.eth.capacity_samples_per_min = 1; // Can upload 1 sample total (consumed by itself)
    nv0.lte.usable = false;
    nv0.lte.capacity_samples_per_min = 0;
    ws.nodes[0] = nv0;

    // Node 1 (Mesh client)
    NodeView nv1;
    nv1.node_id = 1;
    nv1.eth.usable = false;
    nv1.eth.capacity_samples_per_min = 0;
    nv1.lte.usable = false;
    nv1.lte.capacity_samples_per_min = 0;
    nv1.mesh_links.push_back(MeshLink{0, true, 5}); // Mesh link capacity is 5
    ws.nodes[1] = nv1;

    steps.push_back(ws);
  }
  world.set_steps(steps);

  SimConfig sim_cfg;
  sim_cfg.nodes = nodes;
  sim_cfg.minutes = minutes;
  sim_cfg.storage_limit_samples = 120;
  sim_cfg.sample_per_minute = 1;

  Config conn_cfg;

  Simulator sim(probe, world, sim_cfg, conn_cfg);
  SimResult r = sim.run();

  std::cout << "Mesh capacity limit metrics: " << r.metrics.to_string() << std::endl;

  // Expected behavior:
  // Node 0 produces 1 sample/min, uploads 1/min over Ethernet (delivered = 10). Remaining capacity = 0.
  // Node 1 produces 1 sample/min, wants to upload over Mesh to Node 0.
  // Remaining capacity of Node 0 is 0. So Node 1 uploads 0.
  // Node 0 delivered: 10
  // Node 1 delivered: 0
  // Fleet completeness: 10 / 20 = 0.5
  // Node 1 backlog: 10
  REQUIRE(r.metrics.fleet_completeness == 0.5);
  REQUIRE(r.metrics.max_node_backlog == 10);
}

// When production rate exceeds Ethernet capacity, the throughput-aware
// optimization should dynamically switch the node to LTE to prevent
// ring buffer backlog accumulation and data loss.
TEST(throughput_aware_failover_prevents_loss) {
  const int minutes = 10;
  const int nodes = 1;

  FakeProbe probe;
  for (int i = 0; i < nodes; i++) {
    std::vector<PerMinuteProbe> seq;
    for (int t = 0; t < minutes; t++) {
      PerMinuteProbe p;
      p.eth = ProbeResult{true, true, true};
      p.lte = ProbeResult{true, true, true};
      seq.push_back(p);
    }
    probe.set_node_sequences(i, seq);
  }

  WorldModel world;
  std::vector<WorldStep> steps;
  for (int t = 0; t < minutes; t++) {
    WorldStep ws;
    ws.minute_idx = t;
    ws.nodes.resize(nodes);

    NodeView nv;
    nv.node_id = 0;
    nv.eth.usable = true;
    nv.eth.capacity_samples_per_min = 1; // Throttled to 1 sample/min
    nv.lte.usable = true;
    nv.lte.capacity_samples_per_min = 3; // LTE can handle 3 samples/min
    ws.nodes[0] = nv;

    steps.push_back(ws);
  }
  world.set_steps(steps);

  SimConfig sim_cfg;
  sim_cfg.nodes = nodes;
  sim_cfg.minutes = minutes;
  sim_cfg.storage_limit_samples = 120;
  sim_cfg.sample_per_minute = 2; // Production rate is 2 samples/min

  Config conn_cfg;

  Simulator sim(probe, world, sim_cfg, conn_cfg);
  SimResult r = sim.run();

  std::cout << "Throughput-aware optimization metrics: " << r.metrics.to_string() << std::endl;

  // Expected behavior:
  // With optimization: switches to LTE and delivers 100% of samples (20 samples).
  // Without optimization: would stay on Ethernet and only deliver 10 samples (completeness = 0.5).
  REQUIRE(r.metrics.fleet_completeness >= 1.0);
  REQUIRE(r.metrics.max_node_backlog == 0);
  REQUIRE(r.metrics.switch_count == 1); // Only 1 switch (Ethernet -> LTE)
}

int main() {
  std::cout << "Running " << registry().size() << " tests" << std::endl;
  for (auto& tc : registry()) {
    std::cout << "[ RUN      ] " << tc.name << std::endl;
    tc.fn();
    std::cout << "[       OK ] " << tc.name << std::endl;
  }
  return 0;
}
