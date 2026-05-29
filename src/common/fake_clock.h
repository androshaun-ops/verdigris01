#pragma once
#include <cstdint>

namespace edge {

struct IClock {
  virtual ~IClock() = default;
  virtual int64_t now_ms() const = 0;
};

class FakeClock final : public IClock {
 public:
  int64_t now_ms() const override { return now_ms_; }
  void set_ms(int64_t ms) { now_ms_ = ms; }
  void advance_ms(int64_t delta) { now_ms_ += delta; }
 private:
  int64_t now_ms_{0};
};

} // namespace edge
