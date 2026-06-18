#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Configurações do MySensors estão no platformIO.ini

#include "ngm/config_utils.h"
#include "ngm/leds.h"
#include "ngm/mqtt_utils.h"
#include "ngm/webserver.h"
#include "ngm/wifi_utils.h"

#define RESET_PIN A0

// ESP8266WebServer define DEBUG_OUTPUT como Serial; MySensors redefine
// internamente. O #undef evita o warning de redefinição.
#undef DEBUG_OUTPUT
#include <MySensors.h>

// ==== VARIÁVEIS GLOBAIS PARA CONTROLE DE LEDS ====
unsigned long lastStatusBlink = 0;

// ==== OBJETOS GLOBAIS ====
WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP8266WebServer server(80);

// ==== HEARTBEAT CONTROL ====
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 60000; // 60s

// ==== RASTREAMENTO DE NÓS (NODE TRACKING) ====
#define MAX_NODES 10
#define NODE_TIMEOUT_MS 300000 // 5 minutos sem mensagem = nó perdido

struct NodeStatus {
  uint8_t nodeId;
  unsigned long lastSeen;
  bool active;
};

NodeStatus nodeRegistry[MAX_NODES];
int registeredNodesCount = 0;

// ==== CONFIG GLOBAL ====
// Declarada em config_utils.h como extern

// ==== PROTÓTIPOS ====
void sendMQTT(const MyMessage &message, bool isAck = false);
void publishHeartbeat();
void processMQTTCommand(const JsonDocument &doc);
void publishTransportEvent(const char *event, const char *details = "",
                           int nodeId = 0);
void updateNodeStatus(uint8_t nodeId);
void checkNodeTimeouts();

// ==== FUNÇÕES MYSENSORS ====
void presentation() {
  // Apresentar gateway como nó 0
  sendSketchInfo("Manejo360 Gateway MQTT", "2.0");
  Serial.println("📡 Gateway MySensors apresentado");
  publishTransportEvent("gateway_presented", "Manejo360 Gateway MQTT v2.0");
}

void receive(const MyMessage &message) {
  Serial.println("📨 Mensagem MySensors recebida:");
  ledFlicker(LED_GREEN); // Pisca LED Verde ao receber de rádio
  Serial.print("   Nó: ");
  Serial.print(message.getSender());
  Serial.print(", Sensor: ");
  Serial.print(message.getSensor());
  Serial.print(", Comando: ");
  Serial.print(message.getCommand());
  Serial.print(", Tipo: ");
  Serial.println(message.getType());

  if (message.getLength() > 0) {
    Serial.print("   Payload: ");
    Serial.println(message.getString());
  }

  // Atualizar status do nó (rastreamento)
  updateNodeStatus(message.getSender());

  // Detectar eventos especiais de transporte
  if (message.getCommand() == C_INTERNAL) {
    int nodeId = message.getSender();

    switch (message.getType()) {
    case I_PRESENTATION:
      // Novo nó se apresentando
      publishTransportEvent("node_presentation",
                            "Node presented itself to gateway", nodeId);
      break;

    case I_REGISTRATION_REQUEST:
      // Nó solicitando registro (novo nó ou reconexão)
      publishTransportEvent("node_registration_request",
                            "Node requesting registration", nodeId);
      break;

    case I_CONFIG:
      // Nó solicitando configuração
      publishTransportEvent("node_config_request",
                            "Node requesting configuration", nodeId);
      break;

    case I_HEARTBEAT_RESPONSE:
      // Resposta de heartbeat do nó
      publishTransportEvent("node_heartbeat", "Node heartbeat received",
                            nodeId);
      break;

    case I_DISCOVER_RESPONSE:
      // Resposta de descoberta de nó
      {
        String details = "Node discovered, parent: ";
        details += message.getString();
        publishTransportEvent("node_discovered", details.c_str(), nodeId);
      }
      break;

    default:
      // Outros tipos de mensagens internas
      break;
    }
  }

  // Verificar se é uma resposta/ACK a um comando
  bool isAck = message.isAck();

  // Enviar via MQTT
  sendMQTT(message, isAck);
}

void sendMQTT(const MyMessage &message, bool isAck) {
  // Verificar se MQTT está conectado
  if (!mqttClient.connected()) {
    Serial.println("❌ MQTT não conectado, ignorando mensagem");
    return;
  }

  // Criar documento JSON
  DynamicJsonDocument doc(512);

  doc["nodeId"] = message.getSender();
  doc["sensorId"] = message.getSensor();
  doc["command"] = message.getCommand();
  doc["ack"] = 0; // MySensors não tem getAck(), usar 0 por padrão
  doc["type"] = message.getType();
  doc["payload"] = message.getString();
  doc["timestamp"] = millis() / 1000; // Timestamp em segundos
  doc["description"] = getTypeDescription(message.getType());

  // Adicionar metadados para ACK
  if (isAck) {
    doc["direction"] = "ack";
    doc["ack"] = true;
  } else {
    doc["direction"] = "sensor";
  }

  // Serializar JSON
  String jsonString;
  serializeJson(doc, jsonString);

  // Usar tópico OUT
  String topicOut = buildTopicOut(config);

  // Publicar no MQTT
  bool success = mqttClient.publish(topicOut.c_str(), jsonString.c_str());

  if (success) {
    Serial.println("✅ MQTT publicado:");
    Serial.print("   Tópico: ");
    Serial.println(topicOut);
    Serial.print("   Payload: ");
    Serial.println(jsonString.c_str());

    // Piscar LED de transmissão (Amarelo)
    ledFlicker(LED_YELLOW);
  } else {
    Serial.println("❌ Falha ao publicar MQTT");

    // Piscar LED de erro (Vermelho)
    ledFlicker(LED_RED);
  }
}

void before() {
  Serial.begin(MY_BAUD_RATE);

  // Inicializar LEDs usando o módulo
  initLEDs(LED_RED, LED_GREEN, LED_YELLOW);
  ledBegin();

  Serial.println("🚀 Iniciando Manejo360 Gateway MQTT (estágio BEFORE)...");

  // Reservar área completa da EEPROM:
  //   0–511 : MySensors Core
  //   512–515: M360EEPROM (intervalo de nós)
  //   521+  : DeviceConfig (CONFIG_EEPROM_BASE)
  EEPROM.begin(CONFIG_EEPROM_BASE + sizeof(DeviceConfig));

  // Limpar área MySensors (0-511) apenas se houver dados residuais
  bool needsEepromClean = false;
  for (int i = 0; i < 512; i++) {
    if (EEPROM.read(i) != 0xFF) {
      needsEepromClean = true;
      break;
    }
  }
  if (needsEepromClean) {
    Serial.println("🧹 EEPROM MySensors suja — limpando (0-511)...");
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    Serial.println("✅ EEPROM MySensors limpa");
  } else {
    Serial.println("✅ EEPROM MySensors já limpa");
  }

  // Carregar configuração (apenas leitura — sem reset embutido)
  loadConfig(config);

  Serial.print("🔍 Config carregada - SSID: '");
  Serial.print(config.ssid);
  Serial.println("'");

  // DECISÃO DE MODO DE OPERAÇÃO
  // ============================================================

  if (!isValidConfig(config)) {
    // ── PRIMEIRA EXECUÇÃO ou EEPROM CORROMPIDA ──────────────────
    Serial.println(
        "⚠️ Configuração inválida (primeira execução ou EEPROM corrompida).");
    performFactoryReset(config);
    saveConfig(config);
    Serial.println("✅ Configuração padrão salva.");
    Serial.println("📡 Motivo AP: primeira execucao / config invalida");
    startConfigAP();

  } else if (isA0Low(RESET_PIN)) {
    // ── MODO MANUTENÇÃO: A0 conectado ao GND ────────────────────
    Serial.println("🔧 A0 baixo (GND) — modo manutenção solicitado.");
    Serial.println("📡 Motivo AP: A0 em GND / modo manutencao");
    startConfigAP();

  } else {
    // ── OPERAÇÃO NORMAL ──────────────────────────────────────────
    Serial.println("✅ Configuração válida e A0 livre — conectando ao WiFi...");
    setupWiFi(config);
  }

  Serial.println("📡 Configuração WiFi concluída");
  Serial.flush();
}

void setup() {
  Serial.println("\n🔧 >>> ENTRANDO NO SETUP() <<<");
  Serial.flush();
  delay(100);

  // Iniciar WebServer ANTES do MQTT para garantir acesso se o MQTT travar
  setupWebServer(config, server);
  // Mensagem de servidor web inciado removida (webserver.cpp ja imprime: 🌐
  // Servidor Web iniciado em porta 80)
  Serial.flush();

  bool isInAPMode = (WiFi.getMode() == WIFI_AP);

  if (!isInAPMode) {
    // Logar detalhes da config para depuração de crash
    Serial.print("⚙️ DEBUG CONFIG: Server='");
    Serial.print(config.mqttServer);
    Serial.print("', Port=");
    Serial.print(config.mqttPort);
    Serial.print(", User='");
    Serial.print(config.mqttUser);
    Serial.println("'");
    Serial.flush();

    setupMQTT(config, mqttClient);
    Serial.println("🔌 Configuração MQTT concluída");
    Serial.flush();
  } else {
    Serial.println("🔧 MODO CONFIGURAÇÃO: MQTT desativado temporariamente");
    Serial.flush();
  }

  Serial.println("✅ Gateway iniciado integralmente e pronto para loop!");
  Serial.flush();
}

// Função para controlar LEDs baseada no status do sistema
void updateLEDStatus() {
  if (millis() - lastStatusBlink < 2000) {
    return;
  }
  lastStatusBlink = millis();

  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  bool mqttConnected = mqttClient.connected();
  bool isInAPMode = (WiFi.getMode() == WIFI_AP);

  if (isInAPMode) {
    // Modo AP ativo (configuração inicial) - LED amarelo piscando
    setLedState(LED_RED, LED_STATE_OFF);
    setLedState(LED_YELLOW, LED_STATE_BLINK, 200);
    setLedState(LED_GREEN, LED_STATE_OFF);
  } else if (wifiConnected) {
    if (mqttConnected) {
      // WiFi + MQTT OK - LED verde aceso
      setLedState(LED_RED, LED_STATE_OFF);
      setLedState(LED_YELLOW, LED_STATE_OFF);
      setLedState(LED_GREEN, LED_STATE_ON);
    } else {
      // WiFi OK, MQTT desconectado - LED amarelo piscando
      setLedState(LED_RED, LED_STATE_OFF);
      setLedState(LED_YELLOW, LED_STATE_BLINK, 400);
      setLedState(LED_GREEN, LED_STATE_OFF);
    }
  } else {
    // WiFi desconectado - LED vermelho piscando
    setLedState(LED_RED, LED_STATE_BLINK, 300);
    setLedState(LED_YELLOW, LED_STATE_OFF);
    setLedState(LED_GREEN, LED_STATE_OFF);
  }
}

void loop() {
  // Alimentar watchdog para evitar resets
  yield();

  // Verificar se está em modo AP - se sim, não fazer operações de MQTT
  bool isInAPMode = (WiFi.getMode() == WIFI_AP);

  if (!isInAPMode) {
    // Só tentar reconectar WiFi e MQTT se NÃO estiver em modo AP
    handleWiFiReconnect(config);
    handleMQTTReconnect(config, mqttClient);
    mqttClient.loop();

    // Processar mensagens MySensors (mínima latência — wait(1) é a API pública
    // de process)
    wait(1);

    // Verificar timeouts de nós periodicamente (a cada heartbeat)
    static unsigned long lastNodeCheck = 0;
    if (millis() - lastNodeCheck >= HEARTBEAT_INTERVAL_MS) {
      checkNodeTimeouts();
      lastNodeCheck = millis();
    }

    // Heartbeat periódico
    if (mqttClient.connected() &&
        (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS)) {
      publishHeartbeat();
      // Publicar métricas MQTT periodicamente junto com heartbeat
      publishMQTTMetrics(config, mqttClient);
      lastHeartbeat = millis();
    }
  }

  // Sempre processar servidor web e LEDs
  server.handleClient();
  updateLEDStatus();
  updateLEDs();

  // checkResetButton(config, RESET_PIN);  // desativado para evitar resets por
  // ruído
}

// Envia heartbeat no mesmo tópico config.mqttTopic usando o envelope JSON
// existente
void publishHeartbeat() {
  if (WiFi.getMode() == WIFI_AP) {
    return;
  }
  if (!mqttClient.connected()) {
    Serial.println("[HB] MQTT não conectado, abortando heartbeat");
    return;
  }

  DynamicJsonDocument doc(384);

  // Padrão base (MySensors-like envelope)
  doc["nodeId"] = 0;     // gateway
  doc["sensorId"] = 255; // 255 usado para meta/status
  doc["command"] = 3;    // C_INTERNAL
  doc["ack"] = 0;
  doc["type"] = 22; // I_HEARTBEAT_RESPONSE
  doc["payload"] = "";
  doc["timestamp"] = millis() / 1000;

  // Métricas adicionais (não quebram consumidores existentes)
  int wifiRssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  doc["rssi"] = wifiRssi;    // mantém campo já usado
  doc["batteryLevel"] = 100; // gateway alimentado
  doc["description"] = "heartbeat";
  doc["event"] = "heartbeat";
  doc["uptime"] = millis() / 1000;
  doc["wifiRssi"] = wifiRssi;
  doc["ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString()
                                              : String("0.0.0.0");
  doc["version"] = "2.0";
  doc["source"] = "gateway";

  String jsonString;
  serializeJson(doc, jsonString);
  String topicOut = buildTopicOut(config);
  Serial.print("[HB] Publicando heartbeat no tópico: ");
  Serial.println(topicOut);
  Serial.print("[HB] Payload: ");
  Serial.println(jsonString);

  // Publicar no tópico OUT
  bool ok = mqttClient.publish(topicOut.c_str(), jsonString.c_str());
  if (!ok) {
    Serial.println(
        "[HB] Falha ao publicar heartbeat (provável tamanho/pacote)");
    ledFlicker(LED_RED);
  } else {
    Serial.println("[HB] Heartbeat publicado com sucesso");
    ledFlicker(LED_YELLOW); // Pisca Amarelo no sucesso do HB
  }
}

// Função para processar comandos MQTT recebidos
void processMQTTCommand(const JsonDocument &doc) {
  // Verificar se tem nodeId
  if (!doc.containsKey("nodeId")) {
    Serial.println("❌ Comando inválido: falta nodeId");
    return;
  }

  int nodeId = doc["nodeId"];
  Serial.print("🎯 Processando comando para nó: ");
  Serial.println(nodeId);

  // Formato 1: MySensors completo
  if (doc.containsKey("sensorId") && doc.containsKey("command") &&
      doc.containsKey("type")) {
    int sensorId = doc["sensorId"];
    int command = doc["command"];
    int type = doc["type"];
    String payload = doc["payload"] | "";

    Serial.println("📋 Formato MySensors completo detectado");
    Serial.print("   Sensor: ");
    Serial.print(sensorId);
    Serial.print(", Comando: ");
    Serial.print(command);
    Serial.print(", Tipo: ");
    Serial.print(type);
    Serial.print(", Payload: ");
    Serial.println(payload);

    // Criar e enviar mensagem MySensors
    MyMessage msg(sensorId, (mysensors_data_t)type);
    msg.setDestination(nodeId);
    msg.setCommand((mysensors_command_t)command);

    if (payload.length() > 0) {
      msg.set(payload.c_str());
    } else if (doc.containsKey("value")) {
      if (doc["value"].is<bool>()) {
        msg.set(doc["value"].as<bool>());
      } else if (doc["value"].is<int>()) {
        msg.set(doc["value"].as<int>());
      } else if (doc["value"].is<float>()) {
        msg.set(doc["value"].as<float>(), 2); // 2 casas decimais
      }
    }

    bool success = _sendRoute(msg);
    Serial.println(success ? "✅ Comando enviado"
                           : "❌ Falha ao enviar comando");
    ledFlicker(success ? LED_YELLOW : LED_RED);

  }
  // Formato 2: Simplificado
  else if (doc.containsKey("action")) {
    String action = doc["action"];
    Serial.print("📋 Formato simplificado detectado: ");
    Serial.println(action);

    // Mapear ações para comandos MySensors
    if (action == "PUMP_ON" || action == "1") {
      MyMessage msg(0, V_STATUS);
      msg.setDestination(nodeId);
      msg.setCommand(C_SET);
      msg.set(true);
      bool success = _sendRoute(msg);
      Serial.println(success ? "✅ PUMP_ON enviado"
                             : "❌ Falha ao enviar PUMP_ON");
      ledFlicker(success ? LED_YELLOW : LED_RED);

    } else if (action == "PUMP_OFF" || action == "0") {
      MyMessage msg(0, V_STATUS);
      msg.setDestination(nodeId);
      msg.setCommand(C_SET);
      msg.set(false);
      bool success = _sendRoute(msg);
      Serial.println(success ? "✅ PUMP_OFF enviado"
                             : "❌ Falha ao enviar PUMP_OFF");
      ledFlicker(success ? LED_YELLOW : LED_RED);

    } else if (action == "PUMP_TOGGLE") {
      MyMessage msg(0, V_CUSTOM);
      msg.setDestination(nodeId);
      msg.setCommand(C_SET);
      msg.set("PUMP_TOGGLE");
      bool success = _sendRoute(msg);
      Serial.println(success ? "✅ PUMP_TOGGLE enviado"
                             : "❌ Falha ao enviar PUMP_TOGGLE");
      ledFlicker(success ? LED_YELLOW : LED_RED);

    } else if (action == "FORCE_UPDATE") {
      MyMessage msg(0, V_CUSTOM);
      msg.setDestination(nodeId);
      msg.setCommand(C_SET);
      msg.set("FORCE_UPDATE");
      bool success = _sendRoute(msg);
      Serial.println(success ? "✅ FORCE_UPDATE enviado"
                             : "❌ Falha ao enviar FORCE_UPDATE");
      ledFlicker(success ? LED_YELLOW : LED_RED);

    } else if (action == "SET_INTERVAL" && doc.containsKey("interval")) {
      int interval = doc["interval"];
      MyMessage msg(254, V_VAR1); // CHILD_ID_INTERVAL (Padrão 254 node_engine)
      msg.setDestination(nodeId);
      msg.setCommand(C_SET);
      msg.set(interval);
      bool success = _sendRoute(msg);
      Serial.print(success ? "✅ SET_INTERVAL enviado: "
                           : "❌ Falha ao enviar SET_INTERVAL: ");
      Serial.println(interval);
      ledFlicker(success ? LED_YELLOW : LED_RED);

    } else if (action == "REBOOT_GATEWAY") {
      Serial.println("🔄 Comando de reboot recebido via MQTT. Reiniciando o Gateway...");
      ledFlicker(LED_YELLOW);
      delay(500);
      ESP.restart();

    } else {
      Serial.print("❌ Ação não reconhecida: ");
      Serial.println(action);
    }
  } else {
    Serial.println("❌ Formato de comando não reconhecido");
  }
}

// ==== FUNÇÕES DE RASTREAMENTO DE NÓS ====
void updateNodeStatus(uint8_t nodeId) {
  if (nodeId == 0)
    return; // Ignorar gateway

  // Procurar nó existente
  for (int i = 0; i < registeredNodesCount; i++) {
    if (nodeRegistry[i].nodeId == nodeId) {
      nodeRegistry[i].lastSeen = millis();

      // Se estava inativo e voltou
      if (!nodeRegistry[i].active) {
        nodeRegistry[i].active = true;
        publishTransportEvent("node_reconnected", "Node came back online",
                              nodeId);
      }
      return;
    }
  }

  // Nó novo - adicionar ao registro
  if (registeredNodesCount < MAX_NODES) {
    nodeRegistry[registeredNodesCount].nodeId = nodeId;
    nodeRegistry[registeredNodesCount].lastSeen = millis();
    nodeRegistry[registeredNodesCount].active = true;
    registeredNodesCount++;

    publishTransportEvent("node_discovered", "New node added to registry",
                          nodeId);
  } else {
    Serial.println("⚠️ Limite de nós atingido!");
  }
}

void checkNodeTimeouts() {
  unsigned long now = millis();

  for (int i = 0; i < registeredNodesCount; i++) {
    if (nodeRegistry[i].active) {
      // Verificar se passou do timeout
      if (now - nodeRegistry[i].lastSeen > NODE_TIMEOUT_MS) {
        nodeRegistry[i].active = false;

        char details[50];
        snprintf(details, sizeof(details), "Node timeout after %d seconds",
                 (int)(NODE_TIMEOUT_MS / 1000));
        publishTransportEvent("node_lost", details, nodeRegistry[i].nodeId);

        Serial.print("⚠️ Nó ");
        Serial.print(nodeRegistry[i].nodeId);
        Serial.println(" perdido (timeout)");
      }
    }
  }
}

// ==== FUNÇÃO PARA PUBLICAR EVENTOS DE TRANSPORTE ====
void publishTransportEvent(const char *event, const char *details, int nodeId) {
  if (!mqttClient.connected()) {
    Serial.print("⚠️ MQTT desconectado, evento não publicado: ");
    Serial.println(event);
    return;
  }

  DynamicJsonDocument doc(256);
  doc["event"] = event;
  doc["nodeId"] = nodeId;
  doc["timestamp"] = millis() / 1000;
  doc["rssi"] = getRSSI();

  if (strlen(details) > 0) {
    doc["details"] = details;
  }

  String jsonString;
  serializeJson(doc, jsonString);

  // Publicar no tópico de eventos
  String topicEvents = buildTopicOut(config) + "/events";

  bool success = mqttClient.publish(topicEvents.c_str(), jsonString.c_str());

  if (success) {
    ledFlicker(LED_YELLOW); // Pisca ao enviar evento
    Serial.print("📢 Evento publicado: ");
    Serial.print(event);
    if (nodeId > 0) {
      Serial.print(" (Node ");
      Serial.print(nodeId);
      Serial.print(")");
    }
    Serial.println();
  } else {
    Serial.print("❌ Falha ao publicar evento: ");
    Serial.println(event);
  }
}

// Callback quando um novo nó é descoberto/registrado
void receiveTime(unsigned long ts) {
  // Este callback é chamado quando o gateway envia tempo para um nó
  // Podemos usar para detectar novos nós se eles solicitarem tempo
  Serial.print("⏰ Time request received: ");
  Serial.println(ts);
}

// ==== FUNÇÕES ADICIONAIS PARA SATISFAZER O LINKER (MY_GATEWAY_FEATURE) ====
// Quando MY_GATEWAY_FEATURE é definido sem um cliente de gateway padrão (como
// Serial ou MQTT da lib), o MySensors espera que definamos o transporte
// manualmente. Como este projeto gerencia o MQTT de forma independente na
// função receive(), estas funções podem ser vazias.

bool gatewayTransportInit() {
  // Inicialização do transporte do gateway (não necessária aqui)
  return true;
}

bool gatewayTransportSend(MyMessage &message) {
  // O transporte de saída (Gateway -> MQTT) já é tratado manualmente pela
  // função receive()
  return true;
}

bool gatewayTransportAvailable() { return false; }

MyMessage _dummyMsg;
MyMessage& gatewayTransportReceive(void) {
  return _dummyMsg;
}

