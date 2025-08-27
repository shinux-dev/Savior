#include "ESP32_Utils_MQTT.hpp"
#include <WiFi.h> // Añadir esta línea

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void InitMqtt() {
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void HandleMqtt() {
  if (!mqttClient.connected()) {
    while (!mqttClient.connected()) {
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("✅ MQTT conectado");
      } else {
        Serial.print("❌ Fallo MQTT, rc=");
        Serial.print(mqttClient.state());
        delay(2000);
      }
    }
  }
  mqttClient.loop();
}