# Dallas Patched – DS18B20 mit internem ESP32 Pullup

ESPHome External Component für DS18B20 am ESP32 Pin 16 **ohne externen Pullup**.

## Problem
Der interne Pullup (ca. 45kΩ) ist zu schwach für das Standard-1-Wire-Timing.
Der DS18B20 antwortet auf den Reset-Puls, aber Datenbits werden falsch gelesen.

## Fix
`read_bit()` Wartezeit von 10μs → **30μs** verlängert, damit die Leitung
bei schwachem Pullup sicher HIGH wird.

## Verwendung

```yaml
external_components:
  - source: github://DEIN_USER/dallas-patched
    components: [dallas_patched]

sensor:
  - platform: dallas_patched
    pin:
      number: 16
      mode: input_pullup
    name: "Temperatur"
    update_interval: 15s
```

## Test
```bash
screen /dev/ttyUSB0 115200
```
Nach ~800ms sollte `[dallas_patched] Temp: 23.50 C` erscheinen.
