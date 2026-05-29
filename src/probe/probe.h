#pragma once
#include <cstdint>

namespace edge {

enum class LinkType {
  Ethernet = 0,
  LTE = 1,
  Mesh = 2,
};

struct ProbeResult {
  bool dns_ok = false;
  bool tcp_ok = false;
  bool ping_ok = false;
};

struct IProbe {
  virtual ~IProbe() = default;
  virtual ProbeResult probe(int node_id, LinkType link, int gateway_id, int minute_idx) = 0;
};

} // namespace edge
