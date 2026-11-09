#pragma once
#include <ESPAsyncWebServer.h>
#include "Hardware.h"

namespace DhtController {
  // -- Consulta informações do sensor DHT22
  inline void index(AsyncWebServerRequest *request) {
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
}