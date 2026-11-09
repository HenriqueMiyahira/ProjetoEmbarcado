#ifndef HARDWARE_H
#define HARDWARE_H
#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Config.h"
#include <LittleFS.h>
#include <time.h>


namespace Hardware {
  // -- Temperatura desejada
  inline float targetTemp;

  // -- Variável para controle de liga e desliga automático do fan;
  bool autoMode;
  
  inline DHT sensor(Config::DHT_PIN, DHT22); // Instância Sensor DHT22

  // -- Variáveis de Estados
  inline float currentTemp = 0.0;
  inline float currentHumid = 0.0;
  inline bool errorSensor = false;

  // -- Configuração Horário para UTC-3
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = -10800; // -3 horas em millis
  const int daylightOffset_sec = 0;

  // -- Captura o horário atual
  inline String getTimestamp() {
    struct tm timeinfo;
    
    // Tenta pegar a hora. Se falhar (sem internet), devolve o tempo de atividade
    if (!getLocalTime(&timeinfo)) {
        return String(millis() / 1000) + "s (uptime)";
    }
    
    // Formata para "YYYY-MM-DD HH:MM:SS"
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStringBuff);
  }

  // -- Inicia o Hardware
  inline void init() {
    Preferences prefs;
    prefs.begin("configs", false);
    targetTemp = prefs.getFloat("target", 21.0);
    autoMode = prefs.getBool("automode", false);
    prefs.end();

    pinMode(Config::RELAY_PIN, OUTPUT);
    digitalWrite(Config::RELAY_PIN, LOW); // Garante que ventoinha vai estar off

    sensor.begin();
  }
  
  // -- Gerar logs de erro no arquivo logs.txt
  inline void logError(String error) {
    File logFile = LittleFS.open("/logs.txt", "a");
    if (logFile) {
      unsigned long temp = millis() / 1000;
      logFile.println("[" + getTimestamp() + "s] ERRO: "+ error);
      logFile.close();
    }
  }

  // -- Inicia o WiFi
  inline void initWiFi() {
    Preferences prefs;
    prefs.begin("wifi", false);
    String savedSSID = prefs.getString("ssid", "");
    String savedPass = prefs.getString("pass", "");
    prefs.end();

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
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
  }

  // -- Ativa a fan
  inline void activateFan(bool on) {
    digitalWrite(Config::RELAY_PIN, on ? HIGH : LOW);
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
#endif
