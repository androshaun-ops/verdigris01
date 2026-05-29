# Implementation Plan - Fix Connectivity & Throughput-Aware Optimization

This implementation plan outlines the fixes to eliminate link oscillation, solve preemption state mismatches, and implement the optional **Throughput-Aware Optimization** bonus.

## User Review Required

> [!IMPORTANT]
> **Key Design Choice: Inflight-based Simulation & Dynamic Throughput Scoring**
> We are enhancing the `ConnMgr` step interface to receive the node's current backlog and the production rate. When the current link capacity is insufficient to clear the backlog or handle the production rate, `ConnMgr` dynamically inflates the score of higher-capacity uplinks to prevent sample loss.

## Open Questions

None. The bonus requirement and testing design are fully aligned.

---

## Proposed Changes

### Component: Connectivity Manager & Firmware Node

#### [MODIFY] [conn_mgr.h](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/src/conn_mgr/conn_mgr.h)
- Update `ConnMgr::step` signature to accept `backlog` and `sample_per_minute`:
  `Choice step(int minute_idx, const NodeView& view, int backlog, int sample_per_minute);`
- Introduce a dynamic scoring method that takes capacity, backlog, and base priorities into account:
  - Base priority score: Ethernet (300), Mesh (200), LTE (100).
  - Capacity modifier: If `backlog > 2` or `current_capacity < sample_per_minute`, add `capacity * 50` throughput bonus to available links to prioritize draining backlog.

#### [MODIFY] [conn_mgr.cpp](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/src/conn_mgr/conn_mgr.cpp)
- Implement the updated `step` function and the dynamic throughput-aware scoring system with hysteresis to avoid oscillation.

#### [MODIFY] [firmware_node.h](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/src/fw/firmware_node.h)
- Expose the captured upload state with getters:
  - `LinkType inflight_link() const { return inflight_link_; }`
  - `int inflight_gateway() const { return inflight_gw_; }`
- Update `upload_execute` declaration to accept an optional `gateway_remaining_cap` parameter:
  - `int upload_execute(const NodeView& view, int gateway_remaining_cap = -1);`

#### [MODIFY] [firmware_node.cpp](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/src/fw/firmware_node.cpp)
- Update `conn_mgr_step` to pass the backlog and simulated sample production rate to `ConnMgr::step`:
  `Choice c = mgr_.step(minute_idx, view, backlog(), 1);` (Note: default baseline is 1 sample per minute, but the test can set different values).
- Implement `gateway_remaining_cap` capping in `upload_execute` when the `inflight_link_` is `LinkType::Mesh`.

---

### Component: Simulator

#### [MODIFY] [simulator.cpp](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/src/sim/simulator.cpp)
- Update Phase 4 to pass backlog and `cfg_.sample_per_minute` to the firmware's step method.
- Refactor **Phase 6** to:
  - Use `nodes[i].inflight_link()` and `nodes[i].inflight_gateway()` instead of the newer `choices[i].type` and `choices[i].gateway_id`.
  - Pass the remaining gateway uplink capacity `gw_remaining[gw_id]` when executing mesh uploads.
  - Deduct successfully sent mesh client packets from the gateway node's remaining uplink capacity.
- Keep the simulator end-of-simulation Final Flush to ensure all produced samples are correctly delivered under excess capacity.

---

### Component: Verification Tests

#### [MODIFY] [test_all.cpp](file:///Users/szu-yuanlin/Downloads/Verdigris/edge_takehome_20260529/tests/test_all.cpp)
- Add a new deterministic unit test: `throughput_aware_failover_prevents_loss`.
  - Configures 1 node with production rate = 2 samples/minute.
  - Ethernet capacity is throttled to 1 sample/minute, LTE capacity is 3 samples/minute.
  - Verifies that under the baseline firmware, data would be lost due to staying on throttled Ethernet, but with our throughput-aware optimization, the node successfully failovers to LTE, achieving 100% data delivery and 0 backlog with 0 unnecessary oscillation.

---

## Verification Plan

### Automated Tests
- Run CMake configure, build, and test suite:
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build -j
  ctest --test-dir build --output-on-failure
  ```
- Confirm that all tests (including the new throughput-aware optimization test) pass cleanly.
