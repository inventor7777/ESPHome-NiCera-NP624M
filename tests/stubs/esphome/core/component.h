#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>
namespace esphome {
namespace setup_priority { constexpr float HARDWARE = 800; }
class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual void loop() {}
  virtual void dump_config() {}
  virtual float get_setup_priority() const { return 0; }
  void set_timeout(const char *, unsigned delay, std::function<void()> fn) {
    pending.emplace_back(delay, std::move(fn));
  }
  void mark_failed() { failed = true; }
  void run_next_timeout() {
    auto fn = std::move(pending.front().second);
    pending.erase(pending.begin());
    fn();
  }
  bool failed = false;
  std::vector<std::pair<unsigned, std::function<void()>>> pending;
};
}
