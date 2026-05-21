#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace dallas_patched {

class DallasPatchedSensor : public PollingComponent, public sensor::Sensor {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  InternalGPIOPin *pin_;

  bool reset();
  void write_bit(uint8_t v);
  uint8_t read_bit();
  void write_byte(uint8_t v);
  uint8_t read_byte();
  bool read_scratchpad(uint8_t *data);
  uint8_t crc8(const uint8_t *data, uint8_t len);
};

}  // namespace dallas_patched
}  // namespace esphome
