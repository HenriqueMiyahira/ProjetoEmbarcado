#pragma once
#include <Arduino.h>
#include <DHT.h>

namespace Hardware {
  //Temperatura desejada
  inline float targetTemp = 21.0;

  //Variável para controle de liga e desliga automático do fan;
  bool autoMode = true; 

  // Declarando variáveis pinos DHT22 & RELÉ
  const int PINO_DHT = 27;
  const int PINO_RELAY = 26;

  inline DHT sensor(PINO_DHT, DHT22); // Instância Sensor DHT22

  // Variáveis de Estados
  const float currentTemp = 0.0;
  const float currentHumid = 0.0;
  const bool errorSensor = false;
  const bool fanIsOn = true; //Lógica invertida true = off & false = on

  inline void init() {
    digitalWrite(PINO_RELAY, HIGH); // Garante que ventoinha vai estar off
    pinMode(PINO_RELAY, OUTPUT);

    sensor.begin();
  }

  inline void activateFan(bool on) {
    if(on) {
      digitalWrite(PINO_RELAY, LOW),
      fanIsOn = false;
    }
  } 

  inline void readSensor() {
    float t = sensor.readTemperature();
    float h = sensor.readHumity();

    if (isnan(t) || isnan(h)) {
      errorSensor = true;
      return;
    }

    errorSensor =  false;
    currentHumid = h;
    currentTemp = t;
  }
}