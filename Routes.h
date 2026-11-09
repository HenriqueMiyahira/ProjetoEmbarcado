#pragma once
#include <ESPAsyncWebServer.h>
#include "src/controllers/FanController.h"
#include "src/controllers/DhtController.h"
#include "src/controllers/AutoModeController.h"
#include "src/controllers/TargetTempController.h"
#include "src/controllers/LogController.h"
#include "src/controllers/ConectionController.h"


namespace Routes {
  inline void setupRoutes(AsyncWebServer &server) {
    // -- Rotas Fan
    server.on("/api/fan", HTTP_GET, FanController::index);
    server.on("/api/fan", HTTP_POST, FanController::store);
    // -- Rotas DHT22
    server.on("/api/dht", HTTP_GET, DhtController::index);
    // -- Rota Modo Automático
    server.on("/api/mode", HTTP_GET, AutoModeController::index);
    server.on("/api/mode", HTTP_POST, AutoModeController::store);
    // -- Rota Temperatura alvo
    server.on("/api/temp", HTTP_GET, TargetTempController::index);
    server.on("/api/temp", HTTP_POST, TargetTempController::store);
    // -- Rota Logs
    server.on("/api/logs", HTTP_GET, LogController::index);
    // -- Rota Conexão
    server.on("/api/conn", HTTP_GET, ConnectionController::index);
    server.on("/api/conn", HTTP_POST, ConnectionController::store);

  }
}