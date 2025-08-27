#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"  // Header para checkForBeat de la biblioteca SparkFun
#include "spo2_algorithm.h"  // Header para maxim_heart_rate_and_oxygen_saturation
#include "config.h"
#include "ESP32_Utils.hpp"
#include "ESP32_Utils_MQTT.hpp"

MAX30105 particleSensor;

#define SDA_PIN 22
#define SCL_PIN 21
#define LM35_PIN 34 // Sensor de temperatura LM35

// Para cálculo de BPM
const byte RATE_SIZE = 4; // Aumentar para más promediado
byte rates[RATE_SIZE]; // Array de rates
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

// Para SpO2
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int32_t bufferLength = BUFFER_SIZE;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate; // Usado en SpO2, pero priorizamos el de BPM
int8_t validHeartRate;
int sampleRate = 100; // Definido aquí para uso global

void setup(void) {
  Serial.begin(115200);

  ConnectWiFi_STA(true);
  InitMqtt();

  // Inicializar I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializar sensor MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("❌ Sensor MAX30105 no detectado.");
    while (1);
  }

  // Configuración del sensor
  byte ledBrightness = 60; // 0-255
  byte sampleAverage = 4; // 1,2,4,8,16,32
  byte ledMode = 3; // 1=Red, 2=Red+IR, 3=Red+IR+Green (si aplica)
  int pulseWidth = 411; // 69,118,215,411
  int adcRange = 4096; // 2048,4096,8192,16384
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  Serial.println("✅ Sensor MAX30105 inicializado.");
}

void PublishMqtt(const String& payload) {
  mqttClient.publish("vitals", payload.c_str());
}

void loop() {
  HandleMqtt();

  long ir = particleSensor.getIR();

  if (ir < 50000) {
    Serial.println("👉 No hay dedo detectado...");
    delay(1000);
    return;
  }

  // Leer LM35
  int lm35Raw = analogRead(LM35_PIN);
  float voltaje = lm35Raw * (3.3 / 4095.0);
  float temperaturaLM35 = voltaje * 100.0;

  // Cálculo real de BPM usando la función de la biblioteca
  if (checkForBeat(ir) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Cálculo de SpO2 usando la función de la biblioteca
  for (int i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  int bpmFinal = beatAvg; // Usamos el promedio para estabilidad
  int spo2Final = (validSPO2 == 1) ? spo2 : -999; // -999 si no válido

  Serial.printf("💓 BPM: %d | 🩸 SpO₂: %d%% | 🌡 Temp(LM35): %.1f °C\n", bpmFinal, spo2Final, temperaturaLM35);

  // Publicar por MQTT (JSON)
  String mqttPayload = String("{\"bpm\":") + bpmFinal +
                       ",\"spo2\":" + spo2Final +
                       ",\"temp\":" + temperaturaLM35 + "}";
  PublishMqtt(mqttPayload);

  delay(1000); // Actualiza cada segundo
}