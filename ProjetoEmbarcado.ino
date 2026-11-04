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

  //setando rotas API embarcada
  server.on("/api/dht", HTTP_GET, getDhtData);
  server.on("/api/fan", HTTP_POST, switchFan);

  server.begin();
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastRead >= TEMP_RANGE_MS) {
    lastRead = currentTime;
    Hardware::readSensor();
  }

  if (Hardware::currentTemp > Hardware::targetTemp) {
    Hardware::activateFan(true);
  } else if (Hardware::currentTemp <= (Hardware::targetTemp - HYSTERESIS)) {
    Hardware::activateFan(false);
  }

}