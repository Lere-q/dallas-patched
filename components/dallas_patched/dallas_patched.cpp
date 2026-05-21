#include "dallas_patched.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <driver/gpio.h>

namespace esphome {
namespace dallas_patched {

static const char *TAG = "dallas_patched";
static const uint8_t CMD_CONVERT = 0x44;
static const uint8_t CMD_READ_SCRATCH = 0xBE;
static const uint8_t CMD_SKIP_ROM = 0xCC;

static gpio_num_t gpio;
static uint32_t pin_mask;

void DallasPatchedSensor::setup() {
  gpio = (gpio_num_t)pin_->get_pin();
  pin_mask = (1ULL << (uint8_t)gpio);

  gpio_config_t cfg = {};
  cfg.pin_bit_mask = pin_mask;
  cfg.mode = GPIO_MODE_INPUT_OUTPUT_OD;
  cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);
  gpio_set_level(gpio, 1);
}

void DallasPatchedSensor::dump_config() {
  LOG_SENSOR("", "DallasPatched DS18B20", this);
  LOG_PIN("  Pin: ", this->pin_);
  LOG_UPDATE_INTERVAL(this);
}

static inline void IRAM_ATTR drive_low() {
  GPIO.out_w1tc = pin_mask;
}

static inline void IRAM_ATTR release() {
  GPIO.out_w1ts = pin_mask;
}

static inline bool IRAM_ATTR read_pin() {
  return (GPIO.in & pin_mask) != 0;
}

bool DallasPatchedSensor::reset() {
  drive_low();
  delayMicroseconds(480);
  release();
  delayMicroseconds(70);
  bool presence = !read_pin();
  delayMicroseconds(410);
  return presence;
}

void DallasPatchedSensor::write_bit(uint8_t v) {
  if (v) {
    drive_low();
    delayMicroseconds(10);
    release();
    delayMicroseconds(55);
  } else {
    drive_low();
    delayMicroseconds(65);
    release();
    delayMicroseconds(5);
  }
}

uint8_t DallasPatchedSensor::read_bit() {
  drive_low();
  delayMicroseconds(3);
  release();
  delayMicroseconds(35);
  uint8_t r = read_pin();
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
  return crc8(data, 8) == data[8];
}

void DallasPatchedSensor::update() {
  if (!reset()) {
    ESP_LOGW(TAG, "Kein DS18B20 (Presence)");
    publish_state(NAN);
    return;
  }
  write_byte(CMD_SKIP_ROM);
  write_byte(CMD_CONVERT);
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
