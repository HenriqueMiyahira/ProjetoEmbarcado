#pragma once
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

namespace ConnectionController {
  // -- Coleta informações da conexão atual
  inline void index(AsyncWebServerRequest *request) {
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    
    String statusText = isConnected ? "Conectado" : "Modo de Configuração";
    String connectionMode = isConnected ? "WiFi (STA)" : "Ponto de Acesso (AP)";
    
    
    String currentIP = isConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    
    // Pega o endereço MAC físico da placa
    String macAddress = WiFi.macAddress();

    String json = "{";
    json += "\"ok\": true, ";
    json += "\"status\": \"" + statusText + "\", ";
    json += "\"mode\": \"" + connectionMode + "\", ";
    json += "\"ip\": \"" + currentIP + "\", ";
    json += "\"mac\": \"" + macAddress + "\"";
    json += "}";

    request->send(200, "application/json", json);
  }

  // -- Salva informações do WiFi
  inline void store(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid") && request->hasParam("pass")) {
        Preferences prefs; 
        prefs.begin("wifi", false);
        prefs.putString("ssid", request->getParam("ssid")->value());
        prefs.putString("pass", request->getParam("pass")->value());
        prefs.end();

        String msg = "<body style='background:#121214; color:#009D71; text-align:center;'><h2 style='margin-top:50px;'>Rede Salva! Reiniciando o sistema...</h2></body>";
        request->send(200, "text/html", msg);
        
        delay(2000);
        ESP.restart(); 
      } else {
        request->send(400, "text/plain", "Preencha a rede e a senha!");
      }
  }
}