#include "ESP32_Utils.hpp"

void ConnectWiFi_STA(bool useStaticIP) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado");
  Serial.println(WiFi.localIP());
}