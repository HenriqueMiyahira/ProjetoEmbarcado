#pragma once
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "Config.h"
#include "Hardware.h"

AsyncWebServer server(80);
namespace WebServer {
  // Consulta informações do sensor DHT22
  void getDhtData (AsyncWebServerRequest *request) {
    Hardware::readSensor(); // Lê o sensor DHT22

    bool errorSensor = Hardware::errorSensor;
    
    // -- Verifica se há erro de leitura
    if (errorSensor) {
      String dataError = "{\"message\": \"Sensor DHT22 está com problema, infelizmente não é possível calcular a temperatura atual\", \"ok\": false}";

      request->send(500, "application/json", dataError);
      return;
    }

    // -- Gera resposta em JSON
    String sensorResponse = "{\"message\": \"Temperatura lida com sucesso!\", \"temp\": " + String(Hardware::currentTemp) + ", \"humid\": " + String(Hardware::currentHumid) + ", \"ok\": true}";
  
    request->send(200, "application/json", sensorResponse);

    return;
  }

  // -- Altera status da fan (HIGH ou LOW)
  void switchFan(AsyncWebServerRequest *request) {
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

  // -- Altera o modo automático
  void autoMode(AsyncWebServerRequest *request) {
    if (!request->hasParam("action", true)) {
      request->send(400, "application/json", "{\"ok\": false, \"message\": \"Erro ao alterar modo\"}");
      return;
    }

    String actionStr = request->getParam("action", true)->value();
    bool action = (actionStr == "true" || actionStr == "1");
    
    Hardware::autoMode = action;

    Preferences prefs;
    prefs.begin("configs", false);
    prefs.putBool("automode", Hardware::automode);
    prefs.end();

    String response = "{\"message\": \"Modo alterado com sucesso\", \"ok\": true}";

    request->send(200, "application/json", response);
  }

  // Altera o alvo de temperatura
  void alterTargetTemp(AsyncWebServerRequest *request) {
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

    // -- Setando rotas
    server.serveStatic("/", LittleFS, "/").setDefaultFile("thermoflow.html");

    server.on("/api/dht", HTTP_GET, getDhtData);// DHT22

    server.on("/api/targettemp", HTTP_GET, alterTargetTemp);// Temperatura Alvo

    // -- GET - mode
    server.on("/api/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
      String response = "{\"message\": " + String(Hardware::autoMode) + ", \"ok\": true}";
      request ->send(200, "application/json", response);
    });

    // -- POST - mode
    server.on("/api/mode", HTTP_POST, autoMode);

    // -- GET - fan
    server.on("/api/fan", HTTP_GET, [](AsyncWebServerRequest *request) {
      bool isFanOn = (digitalRead(Config::RELAY_PIN) == HIGH);
      String response = "{\"message\": " + String(isFanOn) + ", \"ok\": true}";
      request ->send(200, "application/json", response);
    });

    // -- POST - fan
    server.on("/api/fan", HTTP_POST, swtichFan); 

    // -- Consultar Logs do sistema
    server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request){
      if (LittleFS.exists("/logs.txt")) {
        request->send(LittleFS, "/logs.txt", "text/plain");
      } else {
        request->send(404, "text/plain", "Nenhum log encontrado.");
      }
    });

    // -- Serve Página de Configuração
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", htmlSetupWifi);
    });

    // -- Salvar configurações novas de WiFi
    server.on("/api/savewifi", HTTP_GET, [](AsyncWebServerRequest *request){
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
    });

    server.begin();
  }
}
