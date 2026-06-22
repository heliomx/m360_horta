#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <M360Credentials.h>

// Mapa de memória EEPROM unificado M360:
//   0   – 511  : MySensors Core (reservado pela lib, nunca escrever aqui)
//   512 – 515  : M360EEPROM — magic (2 B) + interval (2 B)  [M360_EEPROM_INTERVAL_ADDRESS]
//   516 – 520  : Reservado para expansão futura de config de nó
//   521+       : DeviceConfig — WiFi / MQTT / UF / CAR + CRC
// Credenciais do Access Point de configuração (único ponto de definição)
#define CONFIG_EEPROM_BASE 521

struct DeviceConfig {
  uint8_t version;
  char ssid[32];
  char password[32];
  char mqttServer[32];
  int mqttPort;
  char mqttUser[32];
  char mqttPassword[32];
  char uf[4];           // UF (ex: "DF")
  char carNumber[8];    // Número do CAR (ex: "0001")
  uint16_t crc;
};

extern DeviceConfig config;

void loadConfig(DeviceConfig &cfg);
void saveConfig(const DeviceConfig &cfg);
void performFactoryReset(DeviceConfig &cfg);
bool isValidConfig(const DeviceConfig &cfg);
uint16_t calcCRC(const DeviceConfig &cfg);
void checkResetButton(DeviceConfig &cfg, int pin);

// Funções auxiliares para tópicos MQTT
String buildTopicOut(const DeviceConfig &cfg);
String buildTopicIn(const DeviceConfig &cfg);
String buildTopicGatewayStatus(const DeviceConfig &cfg);

// Funções auxiliares MySensors
const char* getTypeDescription(int type);
int getRSSI();
int getBatteryLevel();

// Utilitário: lê A0 duas vezes e retorna true se a média estiver abaixo do threshold
bool isA0Low(int pin, int threshold = 400);
