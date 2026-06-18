/*
 * M360MQTT.h — Gerenciador de conexão MQTT e Métricas para M360-DRY
 */

#pragma once

#ifndef ESP8266
#  error "M360MQTT.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "M360Config.h"

namespace M360 {

// Estrutura de métricas MQTT (mantida para compatibilidade)
struct MQTTMetrics {
    unsigned long connectionTime;      // Timestamp da última conexão
    unsigned long lastFailureTime;     // Timestamp da última falha
    unsigned int  reconnectCount;      // Número de reconexões
    int           lastErrorCode;       // Último código de erro (PubSubClient::state)
    bool          isConnected;         // Status atual
};

class MQTTManager {
public:
    MQTTManager();

    /*
     * Inicializa as configurações do Broker.
     * @param cfg     Struct com SSID/User/Pass
     * @param client  Instância do PubSubClient do sketch
     * @param cb      Ponteiro para a função callback global do MQTT
     */
    void begin(const M360DeviceConfig& cfg, ::PubSubClient& client, MQTT_CALLBACK_SIGNATURE);

    /*
     * Gerencia a manutenção da conexão com Backoff Exponencial.
     * @param cfg     Config para credenciais e UF/CAR
     * @param client  Instância do PubSubClient
     */
    void process(const M360DeviceConfig& cfg, ::PubSubClient& client);

    /*
     * Publica métricas do gateway em JSON no tópico:
     * m360/{UF}/{CAR}/gateway/status
     */
    void publishMetrics(const M360DeviceConfig& cfg, ::PubSubClient& client);

    /*
     * Auxiliares de construção de tópicos
     */
    String buildTopicIn(const M360DeviceConfig& cfg) const;
    String buildTopicOut(const M360DeviceConfig& cfg) const;
    String buildTopicStatus(const M360DeviceConfig& cfg) const;

    // Acesso às métricas
    MQTTMetrics getMetrics() const { return _metrics; }

    // Descrição de erro humana para o estado do PubSubClient
    static const char* getErrorDescription(int errorCode);

private:
    MQTTMetrics    _metrics;
    unsigned long  _lastAttempt = 0;
    unsigned long  _retryInterval = 5000;
    unsigned long  _connectionStartTime = 0;
    bool           _wasConnected = false;
    int            _currentRetryLevel = 0;

    static const unsigned long RETRY_INTERVALS[];
    static const int MAX_RETRY_LEVEL = 4;
};

} // namespace M360
