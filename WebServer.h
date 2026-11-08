#pragma once
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

AsyncWebServer server(80);

namespace WebServer {
  void getDhtData (AsyncWebServerRequest *request) {
    Hardware::readSensor(); //Lê o sensor DHT22

    bool errorSensor = Hardware::errorSensor;
    
    //Verifica se há erro de leitura
    if (errorSensor) {
      String dataError = "{\"message\": \"Sensor DHT22 está com problema, infelizmente não é possível calcular a temperatura atual\", \"ok\": false}";

      request->send(500, "application/json", dataError);
      
      return;
    }

    //Gerar resposta em JSON
    String sensorResponse = "{\"message\": \"Temperatura lida com sucesso!\", \"temp\": " + String(Hardware::currentTemp) + ", \"humid\": " + String(Hardware::currentHumid) + ", \"ok\": true}";
  
    request->send(200, "application/json", sensorResponse);

    return;
  }

  void switchFan (AsyncWebServerRequest *request) {
    if (!request->hasParam("action")) {
      request->send(400);
      return;
    } 

    String actionStr = request->getParam("action")->value();
    bool action = (actionStr == "true");

    Hardware::activateFan(action);

    String actionText = action ? "ligado" : "desligado";
    String relayResponse = "{\"message\": \"Ventilador " + actionText + " com sucesso\", \"ok\": true}";

    request->send(200, "application/json", relayResponse);
  }

  void autoMode(AsyncWebServerRequest *request) {
    if (!request->hasParam("action")) {
      request->send(400, "application/json", "{\"ok\": false, \"message\": \"Erro ao alterar modo\"}");
      return;
    }

    String actionStr = request->getParam("action")->value();
    bool action = (actionStr == "true" || actionStr == "1");
    
    Hardware::autoMode = action;

    String response = "{\"message\": \"Modo alterado com sucesso\", \"ok\": true}";

    request->send(200, "application/json", response);
  }

  void alterTargetTemp(AsyncWebServerRequest *request) {
    if (!request->hasParam("value")) {
      request->send(400, "application/json", "{\"message\": \"Erro ao alterar temperatura\", \"ok\": false, \"temp\": " + String(Hardware::targetTemp) + "}");
      return;
    }

    float newTargetTemp = request->getParam("value")->value().toFloat();

    Hardware::targetTemp = newTargetTemp;

    String response = "{\"message\": \"Temperatura alvo atualizada\", \"ok\": true}";

    request->send(200, "application/json", response);
  }

  inline void initWebServer() {
    if (!LittleFS.begin(true)) {
      Serial.println("Erro ao iniciar o LittleFS!");
      return;
    }

    Serial.println("LittleFS montado com sucesso!");

    //Setando rotas
    server.serveStatic("/", LittleFS, "/").setDefaultFile("thermoflow.html");

    server.on("/api/dht", HTTP_GET, getDhtData);
    server.on("/api/targettemp", HTTP_GET, alterTargetTemp);
    server.on("/api/mode", HTTP_GET, autoMode);
    server.on("/api/fan", HTTP_GET, switchFan);

    server.begin();
  }
}
