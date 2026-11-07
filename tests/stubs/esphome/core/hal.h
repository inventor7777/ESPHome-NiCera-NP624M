#pragma once
#include <vector>
namespace esphome {
inline std::vector<unsigned> delays;
inline void delay_microseconds_safe(unsigned us) { delays.push_back(us); }
}
