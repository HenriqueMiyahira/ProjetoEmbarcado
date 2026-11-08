#pragma once
#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Config.h"
#include <LittleFS.h>


namespace Hardware {
  // -- Temperatura desejada
  inline float targetTemp = 21.0;

  // -- Variável para controle de liga e desliga automático do fan;
  bool autoMode = true; 
  
  inline DHT sensor(DHT_PIN, DHT22); // Instância Sensor DHT22

  // -- Variáveis de Estados
  inline float currentTemp = 0.0;
  inline float currentHumid = 0.0;
  inline bool errorSensor = false;

  // -- Inicia o Hardware
  inline void init() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Garante que ventoinha vai estar off

    sensor.begin();
  }
  
  // -- Gerar logs de erro no arquivo logs.txt
  inline void logError(String error) {
    File logFile = LittleFS.open("/logs.txt", "a");
    if (logFile) {
      unsigned long temp = millis() / 1000;
      logFile.println("[" + String(temp) + "s] ERRO: "+ error);
      logFile.close();
    }
  }

  // -- Inicia o WiFi
  inline void initWiFi() {
    Preferences preferences;
    preferences.begin("wifi", false);
    String savedSSID = preferences.getString("ssid", "");
    String savedPass = preferences.getString("pass", "");

    // 1. Liga os dois modos simultaneamente (Cliente e Roteador)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ThermoFlow_Setup", "12345678");

    // 2. Mágica: Pede pro ESP32 tentar reconectar sozinho em background se a rede cair
    WiFi.setAutoReconnect(true);

    if (savedSSID != "") {
      Serial.println("Tentando conectar ao WiFi: " + savedSSID);
      WiFi.begin(savedSSID.c_str(), savedPass.c_str());

      int tentativas = 0;
      // Espera só 5 segundos. Se não der, o código continua!
      while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
      }
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n[AVISO] Falha ao conectar. AP ativo: ");
      Serial.println(WiFi.softAPIP());
      logError("Falha de conexão inicial. AP ativado.");
    } else {
      Serial.println("\n[SUCESSO] Conectado! IP Local: ");
      Serial.println(WiFi.localIP());
      
     // Desliga AP
      WiFi.mode(WIFI_STA); 
    }
  }

  // -- Ativa a fan
  inline void activateFan(bool on) {
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  } 

  // -- Lê as informações do ambiente coletadas pelo sensor
  inline void readSensor() {
    float t = sensor.readTemperature();
    float h = sensor.readHumidity();

    if (isnan(t) || isnan(h)) {
      errorSensor = true;
      logError("Erro ao ler Temperatura");
      return;
    }

    errorSensor =  false;
    currentHumid = h;
    currentTemp = t;
  }
};
