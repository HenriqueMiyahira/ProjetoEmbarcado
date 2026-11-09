#pragma once
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "Hardware.h"
#include "Routes.h"

AsyncWebServer server(80);
namespace WebServer {
  // -- Página HTML do AP
  const char* htmlSetupWifi = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Configurar WiFi - ThermoFlow</title>
      <style>
        body { background-color: #121214; color: #E1E1E6; font-family: 'Inter', sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .card { background: #202024; border: 1px solid #29292E; padding: 32px; border-radius: 12px; text-align: center; width: 90%; max-width: 350px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
        h2 { color: #009D71; margin-top: 0; }
        input { width: 100%; padding: 12px; margin: 10px 0; background: #121214; border: 1px solid #323238; color: white; border-radius: 6px; box-sizing: border-box; font-size: 16px; }
        button { width: 100%; padding: 14px; margin-top: 15px; background: #009D71; color: white; border: none; border-radius: 6px; font-weight: bold; font-size: 16px; cursor: pointer; transition: 0.2s; }
        button:hover { background: #00845F; }
      </style>
    </head>
    <body>
      <div class="card">
        <h2>Conectar WiFi</h2>
        <p style="font-size: 12px; color: #A8A8B3;">Insira a rede para o ThermoFlow</p>
        <form action="/api/savewifi" method="GET">
          <input type="text" name="ssid" placeholder="Nome da Rede (SSID)" required>
          <input type="password" name="pass" placeholder="Senha da Rede">
          <button type="submit">Salvar e Reiniciar</button>
        </form>
      </div>
    </body>
    </html>
  )rawliteral";

  // -- Inicia o servidor Web
  inline void initWebServer() {
    if (!LittleFS.begin(true)) {
      Serial.println("[AVISO] Erro ao iniciar LittleFS!");
      Hardware::logError("Erro ao iniciar LittleFS!");
      return;
    }
    Serial.println("[SUCESSO] LittleFS montado com sucesso!");

    
    server.serveStatic("/", LittleFS, "/").setDefaultFile("thermoflow.html");

    // -- Setando rotas
    Routes::setupRoutes(server);

    // -- Serve Página de Configuração
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", htmlSetupWifi);
    });
    
    server.begin();
  }


}
