#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define ONE_WIRE_BUS 16

const char* WIFI_SSID = "SSID";
const char* WIFI_PASS = "PASSWORT";
const char* MQTT_HOST = "192.168.1.100";
const int   MQTT_PORT = 1883;
const char* MQTT_TOPIC = "sensor/temperatur";
const char* MQTT_CLIENT = "esp32-ds18b20";

OneWire oneWire;
DallasTemperature sensors(&oneWire);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void connectWiFi() {
  Serial.print("WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" OK");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    if (mqtt.connect(MQTT_CLIENT)) break;
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);

  oneWire.begin(ONE_WIRE_BUS);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  sensors.begin();
  sensors.setResolution(12);

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectMQTT();
}

void loop() {
  sensors.requestTemperatures();
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  delay(800);

  float t = sensors.getTempCByIndex(0);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

  Serial.println(t);

  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  char buf[16];
  snprintf(buf, sizeof(buf), "%.2f", t);
  mqtt.publish(MQTT_TOPIC, buf);

  delay(1000);
}
