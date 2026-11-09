#pragma once
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

namespace LogController {
  inline void index(AsyncWebServerRequest *request) {
    if (LittleFS.exists("/logs.txt")) {
      request->send(LittleFS, "/logs.txt", "text/plain");
    } else {
      request->send(404, "text/plain", "Nenhum log encontrado.");
  }
}
}