#pragma once
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "Hardware.h"

namespace AutoModeController {
  // -- Verifica qual é o modo atual (Ligado / Desligado)
  inline void index(AsyncWebServerRequest *request) {
    request->send(200, "application/json", String(Hardware::autoMode));
  }
  // -- Altera o modo automático (Ligado / Desligado)
  inline void store(AsyncWebServerRequest *request) {
    if (!request->hasParam("action", true)) {
      request->send(400, "application/json", "{\"ok\": false, \"message\": \"Erro ao alterar modo\"}");
      return;
    }

    String actionStr = request->getParam("action", true)->value();
    bool action = (actionStr == "true" || actionStr == "1");
    
    Hardware::autoMode = action;

    Preferences prefs;
    prefs.begin("configs", false);
    prefs.putBool("automode", Hardware::autoMode);
    prefs.end();

    String response = "{\"message\": \"Modo alterado com sucesso\", \"ok\": true}";

    request->send(200, "application/json", response);
  }
}