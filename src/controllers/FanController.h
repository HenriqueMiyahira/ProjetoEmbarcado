#pragma once
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

namespace FanController {
  // -- Consulta o estado atual da fan (HIGH ou LOW)
  inline void index(AsyncWebServerRequest *request) {
    bool isFanOn = (digitalRead(Config::RELAY_PIN) == HIGH);
    String response = "{\"message\": " + String(isFanOn) + ", \"ok\": true}";
    request ->send(200, "application/json", response);
  }

  // -- Altera status da fan (HIGH ou LOW)
  inline void store(AsyncWebServerRequest *request) {
    if (!request->hasParam("action", true)) {
      request->send(400);
      return;
    } 

    String actionStr = request->getParam("action", true)->value();
    bool action = (actionStr == "true");

    Hardware::activateFan(action);

    String actionText = action ? "ligado" : "desligado";
    String relayResponse = "{\"message\": \"Ventilador " + actionText + " com sucesso\", \"ok\": true}";

    request->send(200, "application/json", relayResponse);
  }
}