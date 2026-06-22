/*
 * M360MQTT.cpp — Gerenciador de conexão MQTT e Métricas para M360-DRY
 */

#ifdef ESP8266

#include "M360MQTT.h"

namespace M360 {

const unsigned long MQTTManager::RETRY_INTERVALS[] = {5000, 10000, 20000, 40000, 60000};

MQTTManager::MQTTManager()
    : _lastAttempt(0)
    , _retryInterval(5000)
    , _connectionStartTime(0)
    , _wasConnected(false)
    , _currentRetryLevel(0)
{
    _metrics = {0, 0, 0, 0, false};
}

void MQTTManager::begin(const M360DeviceConfig& cfg, ::PubSubClient& client, MQTT_CALLBACK_SIGNATURE) {
    client.setServer(cfg.mqttServer, cfg.mqttPort);
    client.setCallback(callback);

    _metrics.isConnected = false;
    _metrics.reconnectCount = 0;
    _metrics.connectionTime = 0;
    _metrics.lastFailureTime = 0;
    _metrics.lastErrorCode = 0;
    _wasConnected = false;
    _currentRetryLevel = 0;
    _retryInterval = RETRY_INTERVALS[0];
}

void MQTTManager::process(const M360DeviceConfig& cfg, ::PubSubClient& client) {
    bool currentlyConnected = client.connected();

    // Detectar mudança de estado
    if (currentlyConnected && !_wasConnected) {
        _metrics.isConnected = true;
        _metrics.connectionTime = millis();
        _connectionStartTime = millis();
        _metrics.lastErrorCode = 0;
        _currentRetryLevel = 0;
        _retryInterval = RETRY_INTERVALS[0];
        
        Serial.println("✅ MQTT: Conectado com sucesso.");
        
        // Assinar tópico IN
        String topicIn = buildTopicIn(cfg);
        if (client.subscribe(topicIn.c_str())) {
            Serial.print("📡 MQTT: Assinado tópico IN: ");
            Serial.println(topicIn);
        }
    } else if (!currentlyConnected && _wasConnected) {
        _metrics.isConnected = false;
        _metrics.lastFailureTime = millis();
        _metrics.reconnectCount++;
        Serial.printf("⚠️ MQTT: Desconectado. Reconexões: %d\n", _metrics.reconnectCount);
    }

    _wasConnected = currentlyConnected;
    _metrics.isConnected = currentlyConnected;

    // Tentar reconectar
    unsigned long now = millis();
    if (!currentlyConnected && (now - _lastAttempt >= _retryInterval)) {
        _lastAttempt = now;
        Serial.printf("🔌 MQTT: Reconectando... (interv: %lds)\n", _retryInterval / 1000);

        String clientId = "M360-";
        clientId += cfg.uf;
        clientId += "-";
        clientId += cfg.carNumber;

        if (client.connect(clientId.c_str(), cfg.mqttUser, cfg.mqttPassword)) {
            Serial.println("✅ MQTT: Reconectado.");
            String topicIn = buildTopicIn(cfg);
            if (client.subscribe(topicIn.c_str())) {
                Serial.print("📡 MQTT: Assinado tópico IN: ");
                Serial.println(topicIn);
            }
        } else {
            int err = client.state();
            _metrics.lastErrorCode = err;
            _metrics.lastFailureTime = now;
            
            if (_currentRetryLevel < MAX_RETRY_LEVEL) {
                _currentRetryLevel++;
                _retryInterval = RETRY_INTERVALS[_currentRetryLevel];
            }
            Serial.printf("❌ MQTT: Erro %d (%s). Próximo em %lds\n", 
                          err, getErrorDescription(err), _retryInterval / 1000);
        }
    }
}

void MQTTManager::publishMetrics(const M360DeviceConfig& cfg, ::PubSubClient& client) {
    if (!client.connected()) return;

    // Atualizar tempo de conexão
    if (_metrics.isConnected) {
        _metrics.connectionTime = millis() - _connectionStartTime;
    }

    String topicStatus = buildTopicStatus(cfg);

    // Documento JSON (v6/v7 compatível se JsonDocument for usado, mas aqui fixamos static buffer v6)
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis() / 1000;
    doc["isConnected"] = _metrics.isConnected;
    doc["connectionTime"] = _metrics.connectionTime / 1000;
    doc["reconnectCount"] = _metrics.reconnectCount;
    doc["lastErrorCode"] = _metrics.lastErrorCode;
    
    if (_metrics.lastErrorCode != 0) {
        doc["lastError"] = getErrorDescription(_metrics.lastErrorCode);
    }
    
    doc["uptime"] = millis() / 1000;
    doc["source"] = "gateway";
    doc["type"] = "mqtt_metrics";

    String jsonString;
    serializeJson(doc, jsonString);
    
    if (client.publish(topicStatus.c_str(), jsonString.c_str())) {
        Serial.print("📊 MQTT: Status enviado em: ");
        Serial.println(topicStatus);
    }
}

String MQTTManager::buildTopicIn(const M360DeviceConfig& cfg) const {
    String t = "m360/"; t += cfg.uf; t += "/"; t += cfg.carNumber; t += "/in";
    return t;
}

String MQTTManager::buildTopicOut(const M360DeviceConfig& cfg) const {
    String t = "m360/"; t += cfg.uf; t += "/"; t += cfg.carNumber; t += "/out";
    return t;
}

String MQTTManager::buildTopicStatus(const M360DeviceConfig& cfg) const {
    String t = "m360/"; t += cfg.uf; t += "/"; t += cfg.carNumber; t += "/gateway/status";
    return t;
}

const char* MQTTManager::getErrorDescription(int errorCode) {
    switch (errorCode) {
        case -4: return "CONNECTION_TIMEOUT";
        case -3: return "CONNECTION_LOST";
        case -2: return "CONNECT_FAILED";
        case -1: return "DISCONNECTED";
        case 0:  return "CONNECTED";
        case 1:  return "CONNECT_BAD_PROTOCOL";
        case 2:  return "CONNECT_BAD_CLIENT_ID";
        case 3:  return "CONNECT_UNAVAILABLE";
        case 4:  return "CONNECT_BAD_CREDENTIALS";
        case 5:  return "CONNECT_UNAUTHORIZED";
        default: return "UNKNOWN";
    }
}

} // namespace M360

#endif // ESP8266
