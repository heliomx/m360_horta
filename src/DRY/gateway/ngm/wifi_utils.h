#pragma once
#include <ESP8266WiFi.h>
#include "config_utils.h"

void setupWiFi(const DeviceConfig &cfg);
void handleWiFiReconnect(const DeviceConfig &cfg);

// Ativa o AP de configuração (AP_SSID / AP_PASSWORD) e imprime o IP.
// Ponto único de toda a lógica de softAP — use sempre esta função.
void startConfigAP();
