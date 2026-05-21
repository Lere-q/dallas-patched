# Dallas Patched – DS18B20 mit internem ESP32 Pullup

DS18B20 am ESP32 Pin 16 **ohne externen Pullup**. Patched read_bit von 10μs → **30μs**.

## Home Assistant / ESPHome

```yaml
external_components:
  - source: github://Lere-q/dallas-patched
    components: [dallas_patched]

sensor:
  - platform: dallas_patched
    pin:
      number: 16
      mode: input_pullup
    name: "Temperatur"
    update_interval: 15s
```

Komplette Config: [`example.yaml`](example.yaml)

## Hardware

```
ESP32 Pin 16 ─── DS18B20 DATA
ESP32 3.3V   ─── DS18B20 VCC
ESP32 GND    ─── DS18B20 GND
```
