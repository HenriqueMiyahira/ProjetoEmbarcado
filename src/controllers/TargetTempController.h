#pragma once
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

namespace TargetTempController {
  // Consulta a temperatura alvo atual
  inline void index(AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"temp\": " + String(Hardware::targetTemp) + "}");
  }
  // Altera o alvo de temperatura
  inline void store(AsyncWebServerRequest *request) {
    if (!request->hasParam("value")) {
      request->send(400, "application/json", "{\"message\": \"Erro ao alterar temperatura\", \"ok\": false, \"temp\": " + String(Hardware::targetTemp) + "}");
      return;
    }

    float newTargetTemp = request->getParam("value")->value().toFloat();

    Hardware::targetTemp = newTargetTemp;

    Preferences prefs;
    prefs.begin("configs", false);
    prefs.putFloat("target", Hardware::targetTemp);
    prefs.end();

    String response = "{\"message\": \"Temperatura alvo atualizada\", \"ok\": true}";

    request->send(200, "application/json", response);
  }
}
