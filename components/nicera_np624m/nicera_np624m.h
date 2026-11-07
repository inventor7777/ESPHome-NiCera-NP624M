#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome::nicera_np624m {

class NiceraNP624M : public binary_sensor::BinarySensor, public Component {
 public:
  void set_serial_in_pin(InternalGPIOPin *pin) { serial_in_pin_ = pin; }
  void set_direct_link_pin(InternalGPIOPin *pin) { direct_link_pin_ = pin; }
  void set_sensitivity(uint8_t value) { sensitivity_ = value; }
  void set_ignore_time(uint8_t value) { ignore_time_ = value; }
  void set_pulse_count(uint8_t value) { pulse_count_ = value; }
  void set_window_time(uint8_t value) { window_time_ = value; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void configure_();
  void verify_();
  uint32_t read_configuration_();
  void acknowledge_();
  void drive_direct_low_();
  uint32_t configuration_() const {
    // CamThink's PIR driver confirms the field layout; keep Govee's motion-mode bit.
    return (uint32_t(sensitivity_) << 17) | (uint32_t(ignore_time_) << 13) |
           (uint32_t(pulse_count_) << 11) | (uint32_t(window_time_) << 9) | 0x0100;
  }

  InternalGPIOPin *serial_in_pin_{};
  InternalGPIOPin *direct_link_pin_{};
  uint8_t sensitivity_{15};
  uint8_t ignore_time_{0};
  uint8_t pulse_count_{1};
  uint8_t window_time_{1};
  uint8_t attempts_{0};
  bool ready_{false};
};

}  // namespace esphome::nicera_np624m
