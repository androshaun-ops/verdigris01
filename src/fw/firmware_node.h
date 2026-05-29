#pragma once
#include "conn_mgr/conn_mgr.h"
#include "probe/probe.h"
#include "sim/world.h"
#include <array>
#include <cstdint>

namespace edge {

struct SensorSample {
    uint32_t sequence_num = 0;
    int16_t  value = 0;
    uint8_t  valid = 0;          // 0=uninitialized/consumed, 1=written
};

static constexpr int kRingCapacity = 16;

struct ProbeSnapshot {
    ProbeResult eth{};
    ProbeResult lte{};
};

class FirmwareNode {
 public:
    FirmwareNode(int node_id, IProbe& probe, Config conn_cfg);

    // ISR context: allocate ring slot (advance head)
    void isr_begin_sample();
    // ISR context: write sample data into allocated slot, mark valid
    void isr_commit_sample(int minute_idx);

    // Probe task: update ETH portion of snapshot
    void probe_update_eth(int minute_idx);
    // Probe task: update LTE portion of snapshot
    void probe_update_lte(int minute_idx);

    // ConnMgr task: run link selection using current snapshot
    Choice conn_mgr_step(int minute_idx, const NodeView& view);

    // Upload task: capture current active link for this upload cycle
    void upload_begin();
    // Upload task: drain ring buffer over captured link, return samples sent
    int upload_execute(const NodeView& view);

    int delivered() const { return delivered_; }
    int backlog() const;
    Choice current_choice() const;
    int switch_count() const { return mgr_.switch_count(); }
    int gateway_change_count() const { return mgr_.gateway_change_count(); }

 private:
    int node_id_;
    IProbe& real_probe_;

    // Adapter: ConnMgr reads from probe_snap_ via this
    class SnapshotProbe : public IProbe {
     public:
        ProbeSnapshot* snap = nullptr;
        ProbeResult probe(int, LinkType link, int, int) override {
            if (!snap) return ProbeResult{};
            if (link == LinkType::Ethernet) return snap->eth;
            if (link == LinkType::LTE) return snap->lte;
            return ProbeResult{true, true, true};
        }
    };
    SnapshotProbe snap_probe_;
    ConnMgr mgr_;

    // Ring buffer (ISR writes, upload reads)
    std::array<SensorSample, kRingCapacity> ring_{};
    int head_ = 0;
    int tail_ = 0;
    uint32_t seq_ = 0;

    // Active link (ConnMgr writes, upload reads)
    LinkType active_link_ = LinkType::Ethernet;
    int      active_gateway_ = -1;

    // Probe snapshot (probe task writes, ConnMgr reads via adapter)
    ProbeSnapshot probe_snap_{};

    // Upload in-flight captured state
    LinkType inflight_link_ = LinkType::Ethernet;
    int      inflight_gw_ = -1;

    int delivered_ = 0;
};

} // namespace edge
