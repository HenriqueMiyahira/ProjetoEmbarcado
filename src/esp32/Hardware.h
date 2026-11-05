#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "Config.h"

namespace Hardware {
  //Temperatura desejada
  inline float targetTemp = 21.0;

  //Variável para controle de liga e desliga automático do fan;
  bool autoMode = true; 
  
  inline DHT sensor(DHT_PIN, DHT22); // Instância Sensor DHT22

  // Variáveis de Estados
  inline float currentTemp = 0.0;
  inline float currentHumid = 0.0;
  inline bool errorSensor = false;

  inline void init() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Garante que ventoinha vai estar off

    sensor.begin();
  }

  inline void activateFan(bool on) {
    Serial.println(on);
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  } 

  inline void readSensor() {
    float t = sensor.readTemperature();
    float h = sensor.readHumidity();

    Serial.print("Temperatura lida: "); Serial.println(t);

    if (isnan(t) || isnan(h)) {
      errorSensor = true;
      return;
    }

    errorSensor =  false;
    currentHumid = h;
    currentTemp = t;
  }
};
