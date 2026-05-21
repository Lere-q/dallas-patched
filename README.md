# Dallas Patched – DS18B20 mit internem ESP32 Pullup

DS18B20 am ESP32 Pin 16 **ohne externen 4,7kΩ Pullup**. Der interne Pullup (ca. 45kΩ)
ist eigentlich zu schwach – dieses Repository patcht das read_bit-Timing von 10μs auf **30μs**,
damit die Leitung sicher HIGH wird.

## ESPHome (Home Assistant)

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

Komplette Beispiel-Config: [`example-esp32-ds18b20.yaml`](example-esp32-ds18b20.yaml)

## Alternativ: Arduino + MQTT

Firmware: [`arduino/esp32_pin16.ino`](arduino/esp32_pin16.ino) (WiFi + MQTT + DS18B20)

### Flashen
```bash
arduino-cli lib install "DallasTemperature" "OneWire" "PubSubClient"
arduino-cli compile --fqbn esp32:esp32:esp32 arduino/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 arduino/
```

### Wichtig: OneWire patchen
```bash
# read_bit Timing von 10μs → 30μs ändern:
sed -i 's/delayMicroseconds(10);/delayMicroseconds(30);/' ~/Arduino/libraries/OneWire/OneWire.cpp
```

## Desktop-GUI (Alternative zu HA)

```bash
pip install pyserial matplotlib
python3 tempgraph.py -p /dev/ttyUSB0
```
Darkmode Live-Graph mit aktuell/min/max/mittel.

## Hardware

```
ESP32 Pin 16 ─── DS18B20 DATA
ESP32 3.3V   ─── DS18B20 VCC
ESP32 GND    ─── DS18B20 GND

(Kein externer Pullup nötig!)
```

## Lizenz

MIT
