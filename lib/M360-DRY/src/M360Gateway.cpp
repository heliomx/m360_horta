/*
 * M360Gateway.cpp — Implementação do motor de loop do Gateway M360
 */

#ifdef ESP8266

#define MY_CORE_ONLY
#include "M360Gateway.h"

// Forward-declare MySensors wait function to avoid including MySensors.h in static library
extern bool wait(uint32_t ms);

namespace M360 {

M360Gateway::M360Gateway(::PubSubClient& mqtt, unsigned long heartbeatMs)
    : _mqtt(mqtt)
    , _heartbeatMs(heartbeatMs)
    , _lastHeartbeat(0)   // 0 → disparo imediato no primeiro loop() (comportamento intencional)
    , _lastNodeCheck(0)   // idem
    , _wifiReconnect(nullptr)
    , _mqttReconnect(nullptr)
    , _webHandler(nullptr)
    , _ledUpdate(nullptr)
    , _onHeartbeatCb(nullptr)
    , _onNodeCheckCb(nullptr)
    , _cfg(nullptr)
    , _wifiMan(nullptr)
    , _mqttMan(nullptr)
{
}

void M360Gateway::begin(std::function<void()> wifiReconnect,
                        std::function<void()> mqttReconnect,
                        std::function<void()> webHandler,
                        std::function<void()> ledUpdate)
{
    _wifiReconnect = wifiReconnect;
    _mqttReconnect = mqttReconnect;
    _webHandler    = webHandler;
    _ledUpdate     = ledUpdate;
}

void M360Gateway::begin(M360DeviceConfig& cfg,
                        WiFiManager& wifi,
                        MQTTManager& mqttMan,
                        MQTT_CALLBACK_SIGNATURE,
                        std::function<void()> webHandler,
                        std::function<void()> ledUpdate)
{
    _cfg = &cfg;
    _wifiMan = &wifi;
    _mqttMan = &mqttMan;

    _wifiReconnect = [this]() { if (_wifiMan && _cfg) _wifiMan->process(*_cfg); };
    _mqttReconnect = [this]() { if (_mqttMan && _cfg) _mqttMan->process(*_cfg, _mqtt); };

    _webHandler = webHandler;
    _ledUpdate  = ledUpdate;

    _wifiMan->begin(*_cfg);
    _mqttMan->begin(*_cfg, _mqtt, callback);
}

void M360Gateway::onHeartbeat(std::function<void()> cb) {
    _onHeartbeatCb = cb;
}

void M360Gateway::onNodeCheck(std::function<void()> cb) {
    _onNodeCheckCb = cb;
}

void M360Gateway::loop() {
    // Alimenta watchdog do ESP8266
    yield();

    if (!isAPMode()) {
        // Manter conectividade
        if (_wifiReconnect) _wifiReconnect();
        if (_mqttReconnect) _mqttReconnect();

        // Processar mensagens MQTT pendentes
        _mqtt.loop();

        // Pump de mensagens MySensors — síncrono (~1 ms), única API pública de process()
        wait(1);

        unsigned long now = millis();

        // Node-check: executado a cada heartbeatMs independentemente do MQTT
        if (now - _lastNodeCheck >= _heartbeatMs) {
            if (_onNodeCheckCb) _onNodeCheckCb();
            _lastNodeCheck = now;
        }

        // Heartbeat: o timer avança sempre (evita drift).
        // O callback só é invocado se o MQTT estiver conectado no momento do disparo.
        // Efeito: se o MQTT desconectar e reconectar no meio do ciclo, o próximo
        // heartbeat dispara no instante correto — não imediatamente após a reconexão.
        if (now - _lastHeartbeat >= _heartbeatMs) {
            _lastHeartbeat = now;   // avança sempre
            if (_mqtt.connected() && _onHeartbeatCb) {
                _onHeartbeatCb();
            }
        }
    }

    // Sempre executar — inclusive em modo AP
    if (_webHandler) _webHandler();
    if (_ledUpdate)  _ledUpdate();
}

bool M360Gateway::isAPMode() const {
    return ::WiFi.getMode() == WIFI_AP;
}

bool M360Gateway::isMQTTConnected() const {
    return _mqtt.connected();
}

} // namespace M360

#endif // ESP8266

