#pragma once
#include <Arduino.h>

//Credênciais WiFi
const char* SSID = "";
const char* PASSWORD = "";

//Pinos lógicos do embarcado
const uint8_t DHT_PIN = 27;
const uint8_t RELAY_PIN = 26;

const unsigned long TEMP_RANGE_MS = 300000; // 5 Minutos

const float HYSTERESIS = 1.0;
