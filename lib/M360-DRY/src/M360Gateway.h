/*
 * M360Gateway.h — Motor de orquestração do loop para Gateway MySensors+MQTT
 *
 * Encapsula a lógica de temporização e despacho do loop() do gateway:
 *   - WiFi/MQTT reconnect  (injetados via begin())
 *   - mqttClient.loop()
 *   - wait(1) — pump de mensagens MySensors (síncrono, 1 ms por iteração)
 *   - Heartbeat e verificação de timeout de nós (callbacks via onHeartbeat/onNodeCheck)
 *   - WebServer e LED update (injetados via begin())
 *
 * PLATAFORMA: EXCLUSIVO ESP8266.
 *   Incluir em um nó AVR resulta em erro de compilação imediato (#error).
 *   M360.h já guarda o include com #ifdef ESP8266.
 *
 * CICLO DE VIDA DE _mqtt:
 *   O PubSubClient passado no constructor DEVE ter vida útil >= a do M360Gateway.
 *   Destruir o PubSubClient antes do gateway resulta em dangling reference.
 *
 * USO MÍNIMO:
 *
 *   WiFiClient espClient;
 *   PubSubClient mqttClient(espClient);
 *   M360::M360Gateway gateway(mqttClient);
 *   M360::M360DeviceConfig config;
 *   M360::WiFiManager wifi;
 *   M360::MQTTManager mqtt;
 *
 *   void setup() {
 *     M360::Config::load(config);
 *     gateway.begin(config, wifi, mqtt, 
 *       []() { server.handleClient(); },
 *       []() { updateLEDs(); }
 *     );
 *   }
 *
 *   void setup() {
 *     // Lambdas com captura são suportadas (std::function)
 *     gateway.begin(
 *       [&]() { handleWiFiReconnect(config); },
 *       [&]() { handleMQTTReconnect(config, mqttClient); },
 *       [&]() { server.handleClient(); },
 *       [&]() { updateLEDStatus(); updateLEDs(); }
 *     );
 *     gateway.onHeartbeat([&]() {
 *       publishHeartbeat();
 *       publishMQTTMetrics(config, mqttClient);
 *     });
 *     gateway.onNodeCheck(checkNodeTimeouts);
 *   }
 *
 *   void loop() { gateway.loop(); }
 *
 * NOTA: receive(), presentation() e gatewayTransport*() continuam como
 * funções globais exigidas pelo MySensors — não são gerenciadas por esta classe.
 */

#pragma once

// Guard explícito: falha com mensagem clara se incluído em plataforma incorreta
#ifndef ESP8266
#  error "M360Gateway.h é exclusivo do ESP8266. Não inclua este header em nós AVR ou outras plataformas."
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <functional>
#include "M360Config.h"
#include "M360WiFi.h"
#include "M360MQTT.h"
#include "M360Constants.h"
#include "M360Registry.h"
#include "M360Translator.h"

namespace M360 {

class M360Gateway {
public:
    // Intervalo padrão de heartbeat: 60 segundos
    static const unsigned long DEFAULT_HEARTBEAT_MS = 60000UL;

    /*
     * Constructor.
     *
     * Ambos os timers (_lastHeartbeat, _lastNodeCheck) são inicializados com 0,
     * o que provoca o disparo imediato na primeira iteração do loop() — comportamento
     * desejado para que o heartbeat inicial seja enviado sem esperar 60 s.
     *
     * @param mqtt         Referência ao PubSubClient já configurado.
     *                     DEVE ter vida útil >= a deste objeto (ver nota de ciclo de vida).
     * @param heartbeatMs  Intervalo de heartbeat e node-check (padrão 60 s).
     */
    explicit M360Gateway(::PubSubClient& mqtt,
                         unsigned long heartbeatMs = DEFAULT_HEARTBEAT_MS);

    // Destrutor virtual — permite herança segura sem UB na destruição via ponteiro de base
    virtual ~M360Gateway() = default;

    /*
     * Registra os handlers de infraestrutura.
     *
     * Aceita lambdas com ou sem captura (std::function).
     * Parâmetros nulos são silenciosamente ignorados no loop() — a infraestrutura
     * correspondente simplesmente não será chamada. Passe nullptr apenas se tiver
     * certeza de que o subsistema não é necessário (ex: sem LEDs).
     *
     * @param wifiReconnect  Ex: [&]() { handleWiFiReconnect(config); }
     * @param mqttReconnect  Ex: [&]() { handleMQTTReconnect(config, mqttClient); }
     * @param webHandler     Ex: [&]() { server.handleClient(); }
     * @param ledUpdate      Ex: [&]() { updateLEDStatus(); updateLEDs(); }
     */
    /*
     * Registra os handlers de infraestrutura manualmente (via lambdas).
     */
    void begin(std::function<void()> wifiReconnect,
               std::function<void()> mqttReconnect,
               std::function<void()> webHandler,
               std::function<void()> ledUpdate);

    /*
     * Registra os handlers usando os gerenciadores integrados da biblioteca.
     * Simplifica radicalmente o setup do gateway.
     *
     * @param cfg         Configuração carregada da EEPROM
     * @param wifi        Instância do WiFiManager
     * @param mqttMan     Instância do MQTTManager
     * @param webHandler  Callback para o webserver (opcional)
     * @param ledUpdate   Callback para atualização de LEDs (opcional)
     */
    void begin(M360DeviceConfig& cfg,
               WiFiManager& wifi,
               MQTTManager& mqttMan,
               std::function<void()> webHandler = nullptr,
               std::function<void()> ledUpdate = nullptr);

    /*
     * Callback chamado a cada heartbeatMs, SOMENTE se MQTT estiver conectado.
     * O timer avança independentemente da conexão MQTT para evitar drift:
     * se MQTT desconectar e reconectar no meio do ciclo, o próximo heartbeat
     * dispara no tempo correto (não imediatamente após a reconexão).
     *
     * Usar para: publishHeartbeat(), publishMQTTMetrics().
     */
    void onHeartbeat(std::function<void()> cb);

    /*
     * Callback chamado a cada heartbeatMs (mesmo ciclo do heartbeat).
     * Executado independentemente do estado do MQTT.
     *
     * Usar para: checkNodeTimeouts().
     */
    void onNodeCheck(std::function<void()> cb);

    /*
     * Corpo do loop do gateway — chamar diretamente de loop().
     *
     * Sequência executada a cada iteração:
     *  1. yield()               — alimenta watchdog do ESP8266
     *  2. Se NÃO em modo AP:
     *     a. wifiReconnect()    — manter conectividade WiFi
     *     b. mqttReconnect()    — manter conectividade MQTT (com backoff)
     *     c. mqttClient.loop()  — pump de mensagens MQTT pendentes
     *     d. wait(1)            — pump MySensors (síncrono, ~1 ms)
     *     e. A cada heartbeatMs: onNodeCheck() e, se MQTT conectado, onHeartbeat()
     *  3. Sempre (inclusive em modo AP): webHandler() e ledUpdate()
     *
     * Nota sobre wait(1): é a API pública de process() do MySensors e bloqueia
     * por ~1 ms. Em gateways de altíssima frequência isso pode ser relevante,
     * mas é a única forma suportada de alimentar o rádio RF24 sem acesso interno.
     */
    void loop();

    // ----- Accessors -----

    // Retorna true se o ESP8266 está em modo AP (portal de configuração)
    bool isAPMode() const;

    // Retorna true se o MQTT está conectado
    bool isMQTTConnected() const;

    // Retorna o intervalo de heartbeat configurado
    unsigned long heartbeatMs() const { return _heartbeatMs; }

    // Rastreamento de nós
    NodeRegistry& registry() { return _registry; }
    const NodeRegistry& registry() const { return _registry; }

private:
    ::PubSubClient&       _mqtt;
    unsigned long         _heartbeatMs;
    unsigned long         _lastHeartbeat;
    unsigned long         _lastNodeCheck;

    NodeRegistry          _registry;

    std::function<void()> _wifiReconnect;
    std::function<void()> _mqttReconnect;
    std::function<void()> _webHandler;
    std::function<void()> _ledUpdate;

    std::function<void()> _onHeartbeatCb;
    std::function<void()> _onNodeCheckCb;

    // Gerenciadores integrados (opcionais)
    M360DeviceConfig* _cfg;
    WiFiManager*      _wifiMan;
    MQTTManager*      _mqttMan;
};

} // namespace M360
