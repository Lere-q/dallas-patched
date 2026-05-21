#include "dallas_patched.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <soc/gpio_struct.h>
#include <driver/gpio.h>

namespace esphome {
namespace dallas_patched {

static const char *TAG = "dallas_patched";
static const uint8_t CMD_CONVERT = 0x44;
static const uint8_t CMD_READ_SCRATCH = 0xBE;
static const uint8_t CMD_SKIP_ROM = 0xCC;

static uint8_t pin_num;
static uint32_t pin_bit;

void DallasPatchedSensor::setup() {
  pin_num = pin_->get_pin();
  pin_bit = (1 << pin_num);

  gpio_set_direction((gpio_num_t)pin_num, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_pull_mode((gpio_num_t)pin_num, GPIO_PULLUP_ONLY);
  GPIO.out_w1ts = pin_bit;
  GPIO.enable_w1tc = pin_bit;
}

void DallasPatchedSensor::dump_config() {
  LOG_SENSOR("", "DallasPatched DS18B20", this);
  LOG_PIN("  Pin: ", this->pin_);
  LOG_UPDATE_INTERVAL(this);
}

bool DallasPatchedSensor::reset() {
  GPIO.out_w1tc = pin_bit;
  GPIO.enable_w1ts = pin_bit;
  delayMicroseconds(480);
  GPIO.enable_w1tc = pin_bit;
  delayMicroseconds(70);
  bool p = !(GPIO.in & pin_bit);
  delayMicroseconds(410);
  return p;
}

void DallasPatchedSensor::write_bit(uint8_t v) {
  if (v) {
    GPIO.out_w1tc = pin_bit;
    GPIO.enable_w1ts = pin_bit;
    delayMicroseconds(10);
    GPIO.out_w1ts = pin_bit;
    delayMicroseconds(55);
  } else {
    GPIO.out_w1tc = pin_bit;
    GPIO.enable_w1ts = pin_bit;
    delayMicroseconds(65);
    GPIO.out_w1ts = pin_bit;
    delayMicroseconds(5);
  }
}

uint8_t DallasPatchedSensor::read_bit() {
  GPIO.enable_w1ts = pin_bit;
  GPIO.out_w1tc = pin_bit;
  delayMicroseconds(3);
  GPIO.enable_w1tc = pin_bit;
  delayMicroseconds(30);
  uint8_t r = (GPIO.in >> pin_num) & 1;
  delayMicroseconds(27);
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
  return crc8(data, 8) == data[8];
}

void DallasPatchedSensor::update() {
  if (!reset()) {
    return;
  }
  write_byte(CMD_SKIP_ROM);
  write_byte(CMD_CONVERT);
  delay(800);

  uint8_t data[9];
  if (!read_scratchpad(data)) {
    ESP_LOGD(TAG, "RAW: %02X %02X %02X %02X %02X %02X %02X %02X CRC=%02X calc=%02X",
             data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], crc8(data, 8));
    return;
  }

  int16_t raw = (int16_t)((data[1] << 8) | data[0]);
  publish_state(raw * 0.0625f);
}

}  // namespace dallas_patched
}  // namespace esphome
