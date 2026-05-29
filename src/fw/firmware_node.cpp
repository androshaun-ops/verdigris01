#include "fw/firmware_node.h"

namespace edge {

FirmwareNode::FirmwareNode(int node_id, IProbe& probe, Config conn_cfg)
    : node_id_(node_id),
      real_probe_(probe),
      mgr_(node_id, snap_probe_, conn_cfg)
{
    snap_probe_.snap = &probe_snap_;
}

void FirmwareNode::isr_begin_sample() {
    head_ = (head_ + 1) % kRingCapacity;
}

void FirmwareNode::isr_commit_sample(int minute_idx) {
    int slot = seq_ % kRingCapacity;
    ring_[slot].sequence_num = ++seq_;
    ring_[slot].value = static_cast<int16_t>(minute_idx & 0x7FFF);
    ring_[slot].valid = 1;
}

void FirmwareNode::probe_update_eth(int minute_idx) {
    probe_snap_.eth = real_probe_.probe(node_id_, LinkType::Ethernet, -1, minute_idx);
}

void FirmwareNode::probe_update_lte(int minute_idx) {
    probe_snap_.lte = real_probe_.probe(node_id_, LinkType::LTE, -1, minute_idx);
}

Choice FirmwareNode::conn_mgr_step(int minute_idx, const NodeView& view, int sample_per_minute) {
    Choice c = mgr_.step(minute_idx, view, backlog(), sample_per_minute);
    active_link_ = c.type;
    active_gateway_ = c.gateway_id;
    return c;
}

void FirmwareNode::upload_begin() {
    inflight_link_ = active_link_;
    inflight_gw_ = active_gateway_;
}

int FirmwareNode::upload_execute(const NodeView& view, int gateway_remaining_cap) {
    int cap = 0;
    bool ok = false;

    if (inflight_link_ == LinkType::Ethernet) {
        ok = view.eth.usable;
        cap = view.eth.capacity_samples_per_min;
    } else if (inflight_link_ == LinkType::LTE) {
        ok = view.lte.usable;
        cap = view.lte.capacity_samples_per_min;
    } else if (inflight_link_ == LinkType::Mesh) {
        for (const auto& ml : view.mesh_links) {
            if (ml.gateway_id == inflight_gw_ && ml.link_ok) {
                ok = true;
                cap = ml.capacity_samples_per_min;
                if (gateway_remaining_cap >= 0) {
                    cap = std::min(cap, gateway_remaining_cap);
                }
                break;
            }
        }
    }

    if (!ok || cap <= 0) return 0;

    int sent = 0;
    while (sent < cap && tail_ != head_) {
        if (ring_[tail_].valid != 1) {
            break;
        }
        sent++;
        ring_[tail_].valid = 0;
        tail_ = (tail_ + 1) % kRingCapacity;
    }

    delivered_ += sent;
    return sent;
}

int FirmwareNode::backlog() const {
    return (head_ - tail_ + kRingCapacity) % kRingCapacity;
}

Choice FirmwareNode::current_choice() const {
    return mgr_.current_choice();
}

} // namespace edge
