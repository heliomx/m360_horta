#pragma once
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config_utils.h"

// Estrutura de métricas MQTT
struct MQTTMetrics {
  unsigned long connectionTime;      // Timestamp da última conexão
  unsigned long lastFailureTime;     // Timestamp da última falha
  unsigned int reconnectCount;       // Número de reconexões
  int lastErrorCode;                 // Último código de erro
  bool isConnected;                  // Status atual
};

void setupMQTT(const DeviceConfig &cfg, PubSubClient &client);
void handleMQTTReconnect(const DeviceConfig &cfg, PubSubClient &client, unsigned long timeoutMs = 5000);
void mqttCallback(char* topic, byte* payload, unsigned int length);

// Funções de métricas
MQTTMetrics getMQTTMetrics();
void publishMQTTMetrics(const DeviceConfig &cfg, PubSubClient &client);
const char* getMQTTErrorDescription(int errorCode);

// Declaração externa da função de processamento do arquivo principal
void processMQTTCommand(const JsonDocument& doc);
