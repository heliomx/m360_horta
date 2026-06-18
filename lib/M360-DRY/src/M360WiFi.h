/*
 * M360WiFi.h — Gerenciador de conexão WiFi para M360-DRY
 */

#pragma once

#ifndef ESP8266
#  error "M360WiFi.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "M360Config.h"

namespace M360 {

class WiFiManager {
public:
    /*
     * Inicializa o WiFi.
     * Tenta conectar em modo STATION (STA). Se o SSID for "ManualConfig" 
     * ou se 3 tentativas falharem, entra em modo ACCESS POINT (AP).
     */
    void begin(const M360DeviceConfig& cfg);

    /*
     * Gerencia a manutenção da conexão.
     * Deve ser chamado periodicamente (ex: dentro do loop() do gateway).
     * Tenta reconectar se a conexão cair, ou entra em modo AP após falhas repetidas.
     */
    void process(const M360DeviceConfig& cfg);

    // Retorna true se estiver em modo AP (Portal de Configuração)
    bool isAPMode() const;

private:
    unsigned long _lastCheck = 0;
    int           _reconnectAttempts = 0;
    const int     _maxReconnectAttempts = 3;
    const unsigned long _checkInterval = 15000; // 15 segundos
};

} // namespace M360
