#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

AsyncWebServer server(80);

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
  String sensorResponse = "{\"message\": \"Temperatura lida com sucesso!\", \"temp\": " + String(Hardware::currentTemp) + ", \"humid\": " + String(Hardware::currentHumid) + "}";
 
  request->send(200, "application/json", sensorResponse);

  return;
}

void switchFan (AsyncWebServerRequest *request) {
  if (!request->hasParam("action")) {
    request->send(400);
    return;
  } 

  String action = request->getParam("action")->value();

  if (action != "ligado" && action != "desligado") {
    request->send(400);
    return;
  }

  bool turnOn = (action == "ligado");

  Hardware::activateFan(turnOn);

  String relayResponse = "{\"message\": \"Ventilador " + action + " com sucesso\", \"ok\": true}";

  request->send(200, "application/json", relayResponse);
}