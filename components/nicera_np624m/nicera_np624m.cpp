#include "nicera_np624m.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#if defined(USE_LIBRETINY) && LT_RTL8720C
#include <ArduinoPrivate.h>
#endif

namespace esphome::nicera_np624m {
static const char *const TAG = "nicera_np624m";

void NiceraNP624M::setup() {
  serial_in_pin_->setup();
  serial_in_pin_->digital_write(false);
  direct_link_pin_->setup();
  direct_link_pin_->digital_write(false);
  configure_();
}

void NiceraNP624M::drive_direct_low_() {
  // Preload LOW before enabling output, or the stale HIGH latch adds a clock edge.
#if defined(USE_LIBRETINY) && LT_RTL8720C
  // LibreTiny digital_write on an input changes its pull instead of its latch.
  gpio_write(pinData(pinInfo(direct_link_pin_->get_pin()))->gpio, 0);
#else
  direct_link_pin_->digital_write(false);
#endif
  direct_link_pin_->pin_mode(gpio::FLAG_OUTPUT);
  direct_link_pin_->digital_write(false);
}

void NiceraNP624M::configure_() {
  if (attempts_ < 5)
    ++attempts_;
  const uint32_t config = configuration_();
  // Keep DIRECT LINK low during configuration; only the timed bit stream needs the lock.
  drive_direct_low_();
  delay_microseconds_safe(1000);
  {
    InterruptLock lock;
    for (uint32_t mask = 1UL << 24; mask != 0; mask >>= 1) {
      serial_in_pin_->digital_write(false);
      delay_microseconds_safe(1);
      serial_in_pin_->digital_write(true);
      delay_microseconds_safe(1);
      serial_in_pin_->digital_write((config & mask) != 0);
      delay_microseconds_safe(80);
    }
    serial_in_pin_->digital_write(false);
  }
  // Let configuration latch before forcing a read, without blocking the main loop.
  set_timeout("verify", 5, [this]() { verify_(); });
}

uint32_t NiceraNP624M::read_configuration_() {
  uint32_t config = 0;
  {
    InterruptLock lock;
    drive_direct_low_();
#if defined(USE_LIBRETINY) && LT_RTL8720C
    // Match the stock GPIO turnaround without reconfiguring the pad on every bit.
    auto *direct = pinData(pinInfo(direct_link_pin_->get_pin()))->gpio;
    gpio_mode(direct, PullUp);
    const auto settle = []() { asm volatile("nop\nnop\nnop" ::: "memory"); };
    gpio_write(direct, 1);
#else
    direct_link_pin_->digital_write(true);
#endif
    delay_microseconds_safe(224);
    for (uint8_t bit = 0; bit < 40; ++bit) {
#if defined(USE_LIBRETINY) && LT_RTL8720C
      gpio_write(direct, 0);
      settle();
      gpio_write(direct, 1);
      settle();
      gpio_dir(direct, PIN_INPUT);
      gpio_write(direct, 0);
      settle();
      const bool value = gpio_read(direct) != 0;
      gpio_dir(direct, PIN_OUTPUT);
#else
      direct_link_pin_->digital_write(false);
      delay_microseconds_safe(1);
      direct_link_pin_->digital_write(true);
      delay_microseconds_safe(1);
      direct_link_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
      delay_microseconds_safe(2);
      const bool value = direct_link_pin_->digital_read();
      drive_direct_low_();
#endif
      if (bit >= 15)
        config = (config << 1) | value;
    }
    delay_microseconds_safe(182);
  }
  direct_link_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  return config;
}

void NiceraNP624M::verify_() {
  const uint32_t actual = read_configuration_();
  if (actual != configuration_()) {
    const uint32_t retry_delay = attempts_ < 5 ? 500 : 30000;
    ESP_LOGW(TAG, "Configuration mismatch: expected 0x%07lX, received 0x%07lX; retrying in %lu ms",
             static_cast<unsigned long>(configuration_()), static_cast<unsigned long>(actual),
             static_cast<unsigned long>(retry_delay));
    if (attempts_ == 5)
      this->status_set_warning();
    set_timeout("configure", retry_delay, [this]() { configure_(); });
    return;
  }
  this->status_clear_warning();
  acknowledge_();
  ready_ = true;
  publish_state(false);
  ESP_LOGI(TAG, "PIR configuration verified (NP624M-F model assumed)");
}

void NiceraNP624M::acknowledge_() {
  drive_direct_low_();
  delay_microseconds_safe(2000);
  direct_link_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
}

void NiceraNP624M::loop() {
  if (!ready_)
    return;
  const bool motion = direct_link_pin_->digital_read();
  if (motion)
    acknowledge_();
  publish_state(motion);
}

void NiceraNP624M::dump_config() {
  LOG_BINARY_SENSOR("", "NiCera PIR", this);
  LOG_PIN("  Serial In: ", serial_in_pin_);
  LOG_PIN("  Direct Link: ", direct_link_pin_);
  ESP_LOGCONFIG(TAG, "  Sensitivity threshold: %u (lower is more sensitive)", sensitivity_);
  ESP_LOGCONFIG(TAG, "  Ignore time: %u ms", (ignore_time_ + 1) * 500);
  ESP_LOGCONFIG(TAG, "  Pulse count: %u", pulse_count_ + 1);
  ESP_LOGCONFIG(TAG, "  Window time: %u s", (window_time_ + 1) * 2);
  ESP_LOGCONFIG(TAG, "  Experimental H5127 protocol; NP624M-F model assumed");
}

}  // namespace esphome::nicera_np624m
