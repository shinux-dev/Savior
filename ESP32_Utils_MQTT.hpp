#ifndef ESP32_UTILS_MQTT_HPP
#define ESP32_UTILS_MQTT_HPP

#include <WiFi.h> // Añadir esta línea
#include <PubSubClient.h>
#include "config.h"

extern WiFiClient espClient;
extern PubSubClient mqttClient;

void InitMqtt();
void HandleMqtt();

#endif