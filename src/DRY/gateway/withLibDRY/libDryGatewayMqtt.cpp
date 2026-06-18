/*
 * libDryGatewayMqtt.cpp — Gateway Manejo360 usando M360::M360Gateway
 *
 * Versão refatorada de newGatewayMqtt.cpp que delega a orquestração do loop()
 * à classe M360::M360Gateway da biblioteca M360-DRY.
 *
 * DIFERENÇAS em relação a newGatewayMqtt.cpp:
 *   + #include <M360Gateway.h>
 *   + Instância global:  M360::M360Gateway gateway(mqttClient)
 *   + setup() final:	 gateway.begin(...) + gateway.onHeartbeat(...) + gateway.onNodeCheck(...)
 *   + loop():			substituído por gateway.loop()
 *   - Removido: controle manual de lastHeartbeat, lastNodeCheck e isAPMode inline no loop()
 *
 * Tudo mais (before, presentation, receive, sendMQTT, helpers) é idêntico ao original.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Configurações do MySensors

// Habilitar modo gateway para permitir Node ID 0 e evitar !TSF:SID:FAIL

#include "../ngm/config_utils.h"
#include "../ngm/wifi_utils.h"
#include "../ngm/mqtt_utils.h"
#include "../ngm/webserver.h"
#include "../ngm/leds.h"

// ESP8266WebServer define DEBUG_OUTPUT como Serial; MySensors redefine internamente.
#undef DEBUG_OUTPUT
#include <MySensors.h>

// ── NOVO: motor de loop da LibDRY ────────────────────────────────────────────
#include <M360Gateway.h>
#include <M360Translator.h>
#include <M360Registry.h>
#include <M360Constants.h>
// ─────────────────────────────────────────────────────────────────────────────

// ==== OBJETOS GLOBAIS ====
WiFiClient	 espClient;
PubSubClient   mqttClient(espClient);
ESP8266WebServer server(80);

// ── NOVO: instância do motor de gateway ──────────────────────────────────────
M360::M360Gateway gateway(mqttClient);
// ─────────────────────────────────────────────────────────────────────────────

// ==== VARIÁVEIS GLOBAIS PARA CONTROLE DE LEDS ====
unsigned long lastStatusBlink = 0;

// ==== CONFIG GLOBAL (declarada em config_utils.h como extern) ====

// ==== PROTÓTIPOS ====
void sendMQTT(const MyMessage &message, bool isAck = false);
void publishHeartbeat();
void processMQTTCommand(const String& payloadStr);
void processMQTTCommand(const JsonDocument& doc);
void publishTransportEvent(const char* event, const char* details = "", int nodeId = 0);
void checkNodeTimeouts();
void updateLEDStatus();

// ==== FUNÇÕES MYSENSORS ====

void presentation() {
	sendSketchInfo("Manejo360 Gateway MQTT", "2.0");
	Serial.println("📡 Gateway MySensors apresentado");
	publishTransportEvent("gateway_presented", "Manejo360 Gateway MQTT v2.0");
}

void receive(const MyMessage &message) {
	Serial.println("📨 Mensagem MySensors recebida:");
	ledFlicker(LED_GREEN);
	Serial.print("   Nó: ");	  Serial.print(message.getSender());
	Serial.print(", Sensor: ");   Serial.print(message.getSensor());
	Serial.print(", Comando: ");  Serial.print(message.getCommand());
	Serial.print(", Tipo: ");	 Serial.println(message.getType());

	if (message.getLength() > 0) {
		Serial.print("   Payload: ");
		Serial.println(message.getString());
	}

	// Rastreamento de nó (novo padrão)
	if (gateway.registry().update(message.getSender())) {
		publishTransportEvent(M360::EVT_NODE_RECONN, "Node back online or discovered", message.getSender());
	}

	if (message.getCommand() == C_INTERNAL) {
		int nodeId = message.getSender();
		switch (message.getType()) {
			case I_PRESENTATION:
				publishTransportEvent("node_presentation", "Node presented itself to gateway", nodeId);
				break;
			case I_REGISTRATION_REQUEST:
				publishTransportEvent("node_registration_request", "Node requesting registration", nodeId);
				break;
			case I_CONFIG:
				publishTransportEvent("node_config_request", "Node requesting configuration", nodeId);
				break;
			case I_HEARTBEAT_RESPONSE:
				publishTransportEvent("node_heartbeat", "Node heartbeat received", nodeId);
				break;
			case I_DISCOVER_RESPONSE: {
				String details = "Node discovered, parent: ";
				details += message.getString();
				publishTransportEvent(M360::EVT_NODE_DISCOVER, details.c_str(), nodeId);
				break;
			}
			default:
				break;
		}
	}

	sendMQTT(message, message.isAck());
}

// ==== ENVIO MQTT ====

void sendMQTT(const MyMessage &message, bool isAck) {
	if (!mqttClient.connected()) {
		Serial.println("❌ MQTT não conectado, ignorando mensagem");
		return;
	}

	String jsonString = M360::Translator::toJSON(message, isAck);
	String topicOut = buildTopicOut(config);
	bool   success  = mqttClient.publish(topicOut.c_str(), jsonString.c_str());

	if (success) {
		Serial.println("✅ MQTT publicado.");
		ledFlicker(LED_YELLOW);
	} else {
		Serial.println("❌ Falha ao publicar MQTT");
		ledFlicker(LED_RED);
	}
}

// ==== BEFORE ====

void before() {
	Serial.begin(115200);
	initLEDs(LED_RED, LED_GREEN, LED_YELLOW);
	ledBegin();

	Serial.println("🚀 Iniciando Manejo360 Gateway MQTT (estágio BEFORE)...");

	EEPROM.begin(CONFIG_EEPROM_BASE + sizeof(DeviceConfig));

	// Limpar área MySensors (0-511) apenas se houver dados residuais
	bool needsEepromClean = false;
	for (int i = 0; i < 512; i++) {
		if (EEPROM.read(i) != 0xFF) { needsEepromClean = true; break; }
	}
	if (needsEepromClean) {
		Serial.println("🧹 EEPROM MySensors suja — limpando (0-511)...");
		for (int i = 0; i < 512; i++) EEPROM.write(i, 0xFF);
		EEPROM.commit();
		Serial.println("✅ EEPROM MySensors limpa");
	} else {
		Serial.println("✅ EEPROM MySensors já limpa");
	}

	loadConfig(config);
	Serial.print("🔍 Config carregada - SSID: '");
	Serial.print(config.ssid);
	Serial.println("'");

	auto startAP = [](const char* reason) {
		Serial.print("📡 Iniciando AP de configuração — motivo: ");
		Serial.println(reason);
		WiFi.disconnect(true);
		WiFi.softAPdisconnect(true);
		WiFi.mode(WIFI_OFF);
		delay(200);
		WiFi.mode(WIFI_AP);
		delay(100);

		IPAddress apIP(192, 168, 4, 1);
		bool result = WiFi.softAP(AP_SSID, AP_PASSWORD, 1);
		delay(100);
		WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
		if (result) {
			Serial.print("✅ AP iniciado — SSID: " AP_SSID " | IP: ");
			Serial.println(WiFi.softAPIP());
		} else {
			Serial.println("❌ FALHA ao iniciar AP!");
		}
		delay(500);
	};

	if (!isValidConfig(config)) {
		Serial.println("⚠️ Configuração inválida (primeira execução ou EEPROM corrompida).");
		performFactoryReset(config);
		saveConfig(config);
		Serial.println("✅ Configuração padrão salva.");
		startAP("primeira execucao / config invalida");
	} else if (isA0Low(RESET_PIN)) {
		Serial.println("🔧 A0 baixo (GND) — modo manutenção solicitado.");
		startAP("A0 em GND / modo manutencao");
	} else {
		Serial.println("✅ Configuração válida e A0 livre — conectando ao WiFi...");
		setupWiFi(config);
	}

	Serial.println("📡 Configuração WiFi concluída");
	Serial.flush();
}

// ==== SETUP ====

void setup() {
	Serial.println("\n🔧 >>> ENTRANDO NO SETUP() <<<");
	Serial.flush();
	delay(100);

	setupWebServer(config, server);
	Serial.flush();

	bool isInAPMode = (WiFi.getMode() == WIFI_AP);

	if (!isInAPMode) {
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

	// ── NOVO: wiring do motor de gateway ─────────────────────────────────────
	gateway.begin(
		[&]() { handleWiFiReconnect(config); },
		[&]() { handleMQTTReconnect(config, mqttClient); },
		[&]() { server.handleClient(); },
		[&]() { updateLEDStatus(); updateLEDs(); }
	);
	gateway.onHeartbeat([&]() {
		publishHeartbeat();
		publishMQTTMetrics(config, mqttClient);
	});
	gateway.onNodeCheck(checkNodeTimeouts);
	// ─────────────────────────────────────────────────────────────────────────

	Serial.println("✅ Gateway iniciado integralmente e pronto para loop!");
	Serial.flush();
}

// ==== LOOP ====

void loop() {
	gateway.loop();  // M360::M360Gateway orquestra tudo
}

// ==== CONTROLE DE LEDS ====

void updateLEDStatus() {
	if (millis() - lastStatusBlink < 2000) return;
	lastStatusBlink = millis();

	bool wifiConnected = (WiFi.status() == WL_CONNECTED);
	bool mqttConnected = mqttClient.connected();
	bool isInAPMode	= (WiFi.getMode() == WIFI_AP);

	if (isInAPMode) {
		setLedState(LED_RED,	LED_STATE_OFF);
		setLedState(LED_YELLOW, LED_STATE_BLINK, 200);
		setLedState(LED_GREEN,  LED_STATE_OFF);
	} else if (wifiConnected) {
		if (mqttConnected) {
			setLedState(LED_RED,	LED_STATE_OFF);
			setLedState(LED_YELLOW, LED_STATE_OFF);
			setLedState(LED_GREEN,  LED_STATE_ON);
		} else {
			setLedState(LED_RED,	LED_STATE_OFF);
			setLedState(LED_YELLOW, LED_STATE_BLINK, 400);
			setLedState(LED_GREEN,  LED_STATE_OFF);
		}
	} else {
		setLedState(LED_RED,	LED_STATE_BLINK, 300);
		setLedState(LED_YELLOW, LED_STATE_OFF);
		setLedState(LED_GREEN,  LED_STATE_OFF);
	}
}

// ==== HEARTBEAT ====

void publishHeartbeat() {
	if (WiFi.getMode() == WIFI_AP || !mqttClient.connected()) {
		Serial.println("[HB] MQTT não conectado ou em AP, abortando heartbeat");
		return;
	}

	M360::M360DeviceConfig m360Cfg;
	memcpy(&m360Cfg, &config, sizeof(M360::M360DeviceConfig));

	String jsonString = M360::Translator::buildHeartbeat(m360Cfg, WiFi.RSSI());
	String topicOut = buildTopicOut(config);

	Serial.print("[HB] Publicando heartbeat: "); Serial.println(topicOut);
	bool ok = mqttClient.publish(topicOut.c_str(), jsonString.c_str());
	if (ok) {
		Serial.println("[HB] Heartbeat publicado com sucesso");
		ledFlicker(LED_YELLOW);
	} else {
		Serial.println("[HB] Falha ao publicar heartbeat");
		ledFlicker(LED_RED);
	}
}

// ==== PROCESSAR COMANDO MQTT ====

void processMQTTCommand(const JsonDocument& doc) {
	String payloadStr;
	serializeJson(doc, payloadStr);
	processMQTTCommand(payloadStr);
}

void processMQTTCommand(const String& payloadStr) {
	uint8_t   targetNodeId;
	MyMessage outMsg;

	if (M360::Translator::fromJSON(payloadStr, outMsg, targetNodeId)) {
		outMsg.setDestination(targetNodeId);
		bool success = send(outMsg);
		Serial.print("🎯 Comando enviado para Nó "); Serial.print(targetNodeId);
		Serial.println(success ? " ✅" : " ❌");
		ledFlicker(success ? LED_YELLOW : LED_RED);
	} else {
		Serial.println("❌ Erro ao decodificar JSON ou comando inválido");
	}
}

// ==== RASTREAMENTO DE NÓS ====

// Removido updateNodeStatus manual (utilizando gateway.registry())

void checkNodeTimeouts() {
	gateway.registry().checkTimeouts([&](uint8_t nodeId, const char* reason) {
		char details[64];
		snprintf(details, sizeof(details), "Inactivity detected: %s", reason);
		publishTransportEvent(M360::EVT_NODE_LOST, details, nodeId);
		Serial.print("⚠️ Nó "); Serial.print(nodeId);
		Serial.print(" perdido: "); Serial.println(reason);
	});
}

// ==== EVENTOS DE TRANSPORTE ====

void publishTransportEvent(const char* event, const char* details, int nodeId) {
	if (!mqttClient.connected()) {
		Serial.print("⚠️ MQTT desconectado, evento não publicado: ");
		Serial.println(event);
		return;
	}

	String jsonString = M360::Translator::buildEvent(event, nodeId, details, WiFi.RSSI());
	String topicEvents = buildTopicOut(config) + "/events";
	bool   success	 = mqttClient.publish(topicEvents.c_str(), jsonString.c_str());

	if (success) {
		ledFlicker(LED_YELLOW);
		Serial.print("📢 Evento: "); Serial.println(event);
	} else {
		Serial.print("❌ Falha ao publicar evento: "); Serial.println(event);
	}
}

// ==== CALLBACKS MYSENSORS (LINKER) ====

void receiveTime(unsigned long ts) {
	Serial.print("⏰ Time request received: "); Serial.println(ts);
}

bool gatewayTransportInit()					   { return true;  }
bool gatewayTransportSend(MyMessage &/*message*/) { return true;  }
bool gatewayTransportAvailable()				  { return false; }
