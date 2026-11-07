#pragma once
namespace esphome {
namespace gpio { enum Flags { FLAG_INPUT = 1, FLAG_OUTPUT = 2, FLAG_PULLUP = 4 };
inline Flags operator|(Flags a, Flags b) { return Flags(int(a) | int(b)); } }
class InternalGPIOPin {
 public:
  virtual ~InternalGPIOPin() = default;
  virtual void setup() = 0;
  virtual void pin_mode(gpio::Flags mode) = 0;
  virtual bool digital_read() = 0;
  virtual void digital_write(bool value) = 0;
};
}
