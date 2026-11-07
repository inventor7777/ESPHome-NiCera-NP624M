#include "components/nicera_np624m/nicera_np624m.h"
#include "esphome/core/hal.h"
#include <algorithm>
#include <cassert>
#include <deque>
#include <vector>

using namespace esphome;

struct Pin : InternalGPIOPin {
  gpio::Flags mode = gpio::FLAG_OUTPUT;
  bool latch = false;
  unsigned extra_edges = 0;
  std::vector<bool> writes;
  std::deque<bool> reads;
  void setup() override {}
  void pin_mode(gpio::Flags value) override {
    if ((mode & gpio::FLAG_INPUT) && value == gpio::FLAG_OUTPUT && latch)
      ++extra_edges;
    mode = value;
  }
  bool digital_read() override {
    assert(mode == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP));
    assert(!reads.empty());
    bool value = reads.front();
    reads.pop_front();
    return value;
  }
  void digital_write(bool value) override {
    // The generic backend preloads LOW; the RTL backend uses gpio_write directly.
    assert(mode == gpio::FLAG_OUTPUT || !value);
    latch = value;
    writes.push_back(value);
  }
  void readback(uint32_t config, uint16_t sample = 0x4567) {
    for (int i = 14; i >= 0; --i) reads.push_back((sample >> i) & 1);
    for (int i = 24; i >= 0; --i) reads.push_back((config >> i) & 1);
  }
};

void check_written_frame(const Pin &pin, uint32_t expected) {
  assert(pin.writes.size() == 77);  // Initial low, 25 clock/data triplets, final low.
  uint32_t actual = 0;
  for (unsigned bit = 0; bit < 25; ++bit) {
    assert(!pin.writes[1 + bit * 3]);
    assert(pin.writes[2 + bit * 3]);
    actual = (actual << 1) | pin.writes[3 + bit * 3];
  }
  assert(!pin.writes.back());
  assert(actual == expected);
}

int main() {
  struct Settings {
    uint8_t threshold, ignore, pulse, window;
    uint32_t expected;
  };
  for (const auto &settings : {
           Settings{15, 0, 1, 1, 0x1E0B00},  // Stock template, unchanged.
           Settings{1, 0, 0, 0, 0x020100},
           Settings{255, 15, 3, 3, 0x1FFFF00},
           Settings{15, 15, 1, 1, 0x1FEB00},
           Settings{15, 0, 3, 1, 0x1E1B00},
           Settings{15, 0, 1, 3, 0x1E0F00},
       }) {
    nicera_np624m::NiceraNP624M pir;
    Pin serial, direct;
    pir.set_serial_in_pin(&serial);
    pir.set_direct_link_pin(&direct);
    pir.set_sensitivity(settings.threshold);
    pir.set_ignore_time(settings.ignore);
    pir.set_pulse_count(settings.pulse);
    pir.set_window_time(settings.window);
    pir.setup();
    check_written_frame(serial, settings.expected);
    direct.readback(settings.expected);
    pir.run_next_timeout();
    assert(!pir.failed && pir.states == std::vector<bool>{false});
  }
  for (uint8_t threshold : {1, 15, 255}) {
    nicera_np624m::NiceraNP624M pir;
    Pin serial, direct;
    pir.set_serial_in_pin(&serial);
    pir.set_direct_link_pin(&direct);
    pir.set_sensitivity(threshold);
    uint32_t expected = (uint32_t(threshold) << 17) | 0x0B00;
    delays.clear();
    pir.setup();
    check_written_frame(serial, expected);
    assert(std::count(delays.begin(), delays.end(), 80) == 25);
    assert(pir.states.empty());
    assert(pir.pending.front().first == 5);
    direct.readback(expected);
    pir.run_next_timeout();
    assert(direct.reads.empty());
    assert(direct.mode == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP));
    assert(direct.extra_edges == 0);
    assert(!pir.failed && pir.states == std::vector<bool>{false});
    direct.reads = {true, false, true, false};
    for (unsigned i = 0; i < 4; ++i) pir.loop();
    assert(std::count(delays.begin(), delays.end(), 2000) == 3);
    assert((pir.states == std::vector<bool>{false, true, false, true, false}));
    assert(direct.mode == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP));
    assert(direct.extra_edges == 0);
  }
  {
    nicera_np624m::NiceraNP624M pir;
    Pin serial, direct;
    pir.set_serial_in_pin(&serial);
    pir.set_direct_link_pin(&direct);
    pir.setup();
    for (unsigned attempt = 0; attempt < 5; ++attempt) {
      direct.readback(attempt % 2 ? 0x1FFFFFF : 0);
      pir.run_next_timeout();
      assert(pir.states.empty());
      if (attempt < 4) {
        assert(!pir.failed && pir.pending.front().first == 500);
        pir.run_next_timeout();
      }
    }
    assert(pir.failed && pir.pending.empty());
    assert(direct.mode == (gpio::FLAG_INPUT | gpio::FLAG_PULLUP));
    assert(direct.extra_edges == 0);
    pir.loop();  // Failure must not produce a false motion reading.
    assert(pir.states.empty());
  }
  {
    nicera_np624m::NiceraNP624M pir;
    Pin serial, direct;
    pir.set_serial_in_pin(&serial);
    pir.set_direct_link_pin(&direct);
    pir.setup();
    direct.readback(0);
    pir.run_next_timeout();
    pir.run_next_timeout();
    direct.readback(0x1E0B00);
    pir.run_next_timeout();
    assert(!pir.failed && pir.states == std::vector<bool>{false});
  }
}
