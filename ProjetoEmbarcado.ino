#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "Hardware.h"
#include "WebServer.h"

// -- Variáveis globais
unsigned long lastRead = 0;
unsigned long lastSaveTemp;

void setup() {
  Serial.begin(115200); //Iniciando Monitor SerialS
  Hardware::init(); // Iniciando Hardware
  if (!LittleFS.begin(true)) {
    Serial.println("[AVISO] Erro ao iniciar LittleFS!");
    Hardware::logError("Erro ao iniciar LittleFS!");
    return;
  }
  Serial.println("[SUCESSO] LittleFS montado com sucesso!");
  Hardware::initWiFi(); // Iniciando WiFi
  WebServer::initWebServer(); // Iniciando Servidor Web
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastSaveTemp >= Config::SAVE_TEMP_MS) {
    lastSaveTemp = currentTime;

    File historyFile = LittleFS.open("/history.csv", "a");
    if (historyFile) {
      String row = Hardware::getTimestamp() + "," + 
                   String(Hardware::currentTemp) + "," + 
                   String(Hardware::currentHumid);
      historyFile.println(row);
      historyFile.close();
    }
  }

  if (currentTime - lastRead >= Config::TEMP_RANGE_MS) {
    lastRead = currentTime;
    Hardware::readSensor();
  }

  static bool fanState = false;

  if (Hardware::autoMode) {
    if (Hardware::currentTemp > Hardware::targetTemp ) {
    Hardware::activateFan(true);
    fanState = true;

    } else if (Hardware::currentTemp <= (Hardware::targetTemp - Config::HYSTERESIS) && fanState) {
      fanState = false;
      Hardware::activateFan(false);
    }
  }
  
}
