#pragma once
#include <vector>
namespace esphome::binary_sensor {
class BinarySensor {
 public:
  void publish_state(bool value) { states.push_back(value); }
  std::vector<bool> states;
};
}
