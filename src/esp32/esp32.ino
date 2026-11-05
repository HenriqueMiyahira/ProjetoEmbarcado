#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include "Config.h"
#include "Hardware.h"
#include "WebServer.h"

//Variáveis globais
unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200); //Iniciando Monitor Serial
  Hardware::init(); // Iniciando Hardware

  WiFi.begin(SSID, PASSWORD); //Iniciando WiFi

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println(WiFi.localIP());

  WebServer::initWebServer();
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastRead >= TEMP_RANGE_MS) {
    lastRead = currentTime;
    Hardware::readSensor();
  }

  static bool fanState = false;

  if (Hardware::autoMode) {
    if (Hardware::currentTemp > Hardware::targetTemp ) {
    Hardware::activateFan(true);
    fanState = true;

    } else if (Hardware::currentTemp <= (Hardware::targetTemp - HYSTERESIS) && fanState) {
      fanState = false;
      Hardware::activateFan(false);
    }
  }
  
}
