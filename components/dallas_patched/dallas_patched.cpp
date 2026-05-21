#include "dallas_patched.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace dallas_patched {

static const char *TAG = "dallas_patched";

static const uint8_t CMD_CONVERT = 0x44;
static const uint8_t CMD_READ_SCRATCH = 0xBE;
static const uint8_t CMD_SKIP_ROM = 0xCC;

void DallasPatchedSensor::setup() {
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
}

void DallasPatchedSensor::dump_config() {
  LOG_SENSOR("", "DallasPatched DS18B20", this);
  LOG_PIN("  Pin: ", this->pin_);
  LOG_UPDATE_INTERVAL(this);
}

bool DallasPatchedSensor::reset() {
  pin_->digital_write(false);
  pin_->pin_mode(gpio::FLAG_OUTPUT);
  delayMicroseconds(480);
  pin_->digital_write(true);
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  delayMicroseconds(70);
  bool presence = !pin_->digital_read();
  delayMicroseconds(410);
  return presence;
}

void DallasPatchedSensor::write_bit(uint8_t v) {
  if (v) {
    pin_->digital_write(false);
    pin_->pin_mode(gpio::FLAG_OUTPUT);
    delayMicroseconds(10);
    pin_->digital_write(true);
    delayMicroseconds(55);
  } else {
    pin_->digital_write(false);
    pin_->pin_mode(gpio::FLAG_OUTPUT);
    delayMicroseconds(65);
    pin_->digital_write(true);
    delayMicroseconds(5);
  }
}

uint8_t DallasPatchedSensor::read_bit() {
  pin_->digital_write(false);
  pin_->pin_mode(gpio::FLAG_OUTPUT);
  delayMicroseconds(3);
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  delayMicroseconds(35);
  uint8_t r = pin_->digital_read();
  delayMicroseconds(25);
  return r;
}

void DallasPatchedSensor::write_byte(uint8_t v) {
  for (uint8_t i = 0; i < 8; i++) {
    write_bit(v & 1);
    v >>= 1;
  }
}

uint8_t DallasPatchedSensor::read_byte() {
  uint8_t v = 0;
  for (uint8_t i = 0; i < 8; i++) {
    v |= (read_bit() << i);
  }
  return v;
}

uint8_t DallasPatchedSensor::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t b = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ b) & 1;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      b >>= 1;
    }
  }
  return crc;
}

bool DallasPatchedSensor::read_scratchpad(uint8_t *data) {
  if (!reset()) return false;
  write_byte(CMD_SKIP_ROM);
  write_byte(CMD_READ_SCRATCH);
  for (uint8_t i = 0; i < 9; i++) {
    data[i] = read_byte();
  }
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  uint8_t crc = crc8(data, 8);
  return crc == data[8];
}

void DallasPatchedSensor::update() {
  if (!reset()) {
    ESP_LOGW(TAG, "Kein DS18B20 (Presence)");
    publish_state(NAN);
    return;
  }
  write_byte(CMD_SKIP_ROM);
  write_byte(CMD_CONVERT);
  pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
  delay(800);

  uint8_t data[9];
  if (!read_scratchpad(data)) {
    ESP_LOGW(TAG, "CRC Fehler");
    publish_state(NAN);
    return;
  }

  int16_t raw = (int16_t)((data[1] << 8) | data[0]);
  float temp = raw * 0.0625f;
  ESP_LOGD(TAG, "Temp: %.2f C", temp);
  publish_state(temp);
}

}  // namespace dallas_patched
}  // namespace esphome
