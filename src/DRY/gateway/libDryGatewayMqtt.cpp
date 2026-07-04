/*
 * libDryGatewayMqtt.cpp — Gateway Manejo360 usando M360::M360Gateway
 *
 * Usa exclusivamente a biblioteca M360-DRY para config/WiFi/MQTT:
 *   M360::Config, M360::WiFiManager, M360::MQTTManager, M360::M360Gateway
 *
 * Dependências ngm/ restantes (sem equivalente na lib):
 *   - ngm/webserver  — portal web de configuração captivo
 *   - ngm/leds       — controle de LEDs de status (D0/D1/D2)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Habilita prints de depuração gerais e depuração detalhada do rádio (RF24)
#define MY_DEBUG
#define MY_DEBUG_VERBOSE_RF24

// ESP8266WebServer define DEBUG_OUTPUT como Serial; MySensors redefine internamente.
#undef DEBUG_OUTPUT
#include <MySensors.h>

#include <M360Credentials.h>
#include <M360Leds.h>
#include <M360Webserver.h>
#include <M360Gateway.h>
#include <M360Translator.h>
#include <M360Registry.h>
#include <M360Constants.h>

// ==== HELPER LOCAL ====
// Lê A0 duas vezes com delay para filtrar ruído; retorna true se abaixo do limiar
static bool isA0Low(int pin, int threshold = 400) {
    int a1 = analogRead(pin);
    delay(10);
    int a2 = analogRead(pin);
    return ((a1 + a2) / 2) < threshold;
}

// ==== OBJETOS GLOBAIS ====
WiFiClient          espClient;
PubSubClient        mqttClient(espClient);
ESP8266WebServer    server(80);

M360::M360DeviceConfig  config;
M360::WiFiManager       wifiManager;
M360::MQTTManager       mqttManager;
M360::M360Gateway       gateway(mqttClient);

// ==== VARIÁVEIS GLOBAIS PARA CONTROLE DE LEDS ====
unsigned long lastStatusBlink = 0;

// ==== PROTÓTIPOS ====
void sendMQTT(const MyMessage &message, bool isAck = false);
void publishHeartbeat();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void processMQTTCommand(const String& payloadStr);
void processMQTTCommand(const JsonDocument& doc);
void processMQTTCommandNative(const char* topic, const byte* payload, unsigned int length);
void publishTransportEvent(const char* event, const char* details = "", int nodeId = 0);
void checkNodeTimeouts();
void updateLEDStatus();

// ==== FUNÇÕES MYSENSORS ====

void presentation() {
	sendSketchInfo("Manejo360 Gateway MQTT", "2.0");
	Serial.println("📡 Gateway MySensors apresentado");
	// publishTransportEvent movido para setup() — MQTT não está inicializado aqui
}

void receive(const MyMessage &message) {
	Serial.println("📨 Mensagem MySensors recebida:");
	ledFlicker(LED_GREEN);
	Serial.print("   Nó: ");     Serial.print(message.getSender());
	Serial.print(", Sensor: ");  Serial.print(message.getSensor());
	Serial.print(", Comando: "); Serial.print(message.getCommand());
	Serial.print(", Tipo: ");    Serial.println(message.getType());

	if (message.getLength() > 0) {
		char payloadDbg[MAX_PAYLOAD + 1];
		Serial.print("   Payload: ");
		Serial.println(message.getString(payloadDbg));
	}

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
				char discBuf[MAX_PAYLOAD + 1];
				String details = "Node discovered, parent: ";
				details += message.getString(discBuf);
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

#ifdef M360_NATIVE_MQTT
	String topic   = M360::Translator::buildNativeTopic(M360::buildTopicOut(config), message);
	String payload = M360::Translator::toNativePayload(message);
#else
	String topic   = M360::buildTopicOut(config);
	String payload = M360::Translator::toJSON(message, isAck);
#endif

	bool success = mqttClient.publish(topic.c_str(), payload.c_str());
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

	EEPROM.begin(M360_EEPROM_DEVICE_BASE + sizeof(M360::M360DeviceConfig) + 4);

	// Limpar área MySensors (0-511) se houver dados residuais
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

	M360::Config::load(config);
	Serial.print("🔍 Config carregada - SSID: '");
	Serial.print(config.ssid);
	Serial.println("'");

	// Reset completo do stack WiFi antes de criar AP (evita estado residual de tentativa STA)
	auto startAP = [](const char* reason) {
		Serial.print("📡 Iniciando AP de configuração — motivo: ");
		Serial.println(reason);
		WiFi.disconnect(true);
		WiFi.softAPdisconnect(true);
		WiFi.mode(WIFI_OFF);
		delay(200);
		WiFi.mode(WIFI_AP);
		delay(100);

		IPAddress apIP(M360_AP_IP_OCTETS);
		bool result = WiFi.softAP(M360_AP_SSID, M360_AP_PASSWORD, 1);
		delay(100);
		WiFi.softAPConfig(apIP, apIP, IPAddress(M360_AP_NETMASK_OCTETS));
		if (result) {
			Serial.print("✅ AP iniciado — SSID: " M360_AP_SSID " | IP: ");
			Serial.println(WiFi.softAPIP());
		} else {
			Serial.println("❌ FALHA ao iniciar AP!");
		}
		delay(500);
	};

	if (!M360::Config::isValid(config)) {
		Serial.println("⚠️ Configuração inválida (primeira execução ou EEPROM corrompida).");
		M360::Config::reset(config);
		M360::Config::save(config);
		Serial.println("✅ Configuração padrão salva.");
		startAP("primeira execucao / config invalida");
	} else if (isA0Low(RESET_PIN)) {
		Serial.println("🔧 A0 baixo (GND) — modo manutenção solicitado.");
		startAP("A0 em GND / modo manutencao");
	} else {
		Serial.println("✅ Configuração válida e A0 livre — conectando ao WiFi...");
		wifiManager.begin(config);
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

		mqttManager.begin(config, mqttClient, mqttCallback);
		Serial.println("🔌 Configuração MQTT concluída");
		Serial.flush();
	} else {
		Serial.println("🔧 MODO CONFIGURAÇÃO: MQTT desativado temporariamente");
		Serial.flush();
	}

	gateway.begin(
		[&]() { wifiManager.process(config); },
		[&]() { mqttManager.process(config, mqttClient); },
		[&]() { server.handleClient(); },
		[&]() { updateLEDStatus(); updateLEDs(); }
	);
	gateway.onHeartbeat([&]() {
		publishHeartbeat();
		mqttManager.publishMetrics(config, mqttClient);
	});
	gateway.onNodeCheck(checkNodeTimeouts);
	publishTransportEvent("gateway_presented", "Manejo360 Gateway MQTT v2.0");

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
	bool isInAPMode    = (WiFi.getMode() == WIFI_AP);

	if (isInAPMode) {
		setLedState(LED_RED,    LED_STATE_OFF);
		setLedState(LED_YELLOW, LED_STATE_BLINK, 200);
		setLedState(LED_GREEN,  LED_STATE_OFF);
	} else if (wifiConnected) {
		if (mqttConnected) {
			setLedState(LED_RED,    LED_STATE_OFF);
			setLedState(LED_YELLOW, LED_STATE_OFF);
			setLedState(LED_GREEN,  LED_STATE_ON);
		} else {
			setLedState(LED_RED,    LED_STATE_OFF);
			setLedState(LED_YELLOW, LED_STATE_BLINK, 400);
			setLedState(LED_GREEN,  LED_STATE_OFF);
		}
	} else {
		setLedState(LED_RED,    LED_STATE_BLINK, 300);
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

	String jsonString = M360::Translator::buildHeartbeat(config, WiFi.RSSI());
	String topicOut   = M360::buildTopicOut(config);

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

// ==== CALLBACK MQTT ====

void mqttCallback(char* topic, byte* payload, unsigned int length) {
	ledFlicker(LED_GREEN);
	Serial.println("📨 Comando MQTT recebido:");
	Serial.print("   Tópico: ");
	Serial.println(topic);

#ifdef M360_NATIVE_MQTT
	processMQTTCommandNative(topic, payload, length);
#else
	DynamicJsonDocument doc(512);
	DeserializationError error = deserializeJson(doc, payload, length);

	if (error) {
		Serial.print("❌ Erro ao parsear JSON: ");
		Serial.println(error.c_str());
		return;
	}

	processMQTTCommand(doc);
#endif
}

// ==== PROCESSAR COMANDO MQTT ====

// Publica ACK no MQTT após confirmação de transporte MySensors.
// receive() NÃO é chamado para ACKs de transporte — o ACK é consumido internamente
// pela camada MySensors. Por isso publicamos manualmente quando send() retorna true.
//
// ATENÇÃO: outMsg.getSender() retorna 0 (gateway), não o nó de destino.
// Usar Translator::toJSON como base e corrigir nodeId antes de publicar.
static void publishTransportAck(const MyMessage& outMsg, uint8_t targetNodeId) {
#ifdef M360_NATIVE_MQTT
	// Modo nativo: ACK de transporte é implícito no RF24 — broker não espera publicação separada
	(void)outMsg; (void)targetNodeId;
#else
	if (!mqttClient.connected()) return;
	String base = M360::Translator::toJSON(outMsg, true);
	DynamicJsonDocument doc(512);
	deserializeJson(doc, base);
	doc["nodeId"] = targetNodeId;   // corrige 0 → ID real do nó de destino
	String ackJson;
	serializeJson(doc, ackJson);
	String topicOut = M360::buildTopicOut(config);
	if (mqttClient.publish(topicOut.c_str(), ackJson.c_str())) {
		Serial.printf("📤 ACK de transporte publicado — nó %d sensor %d\n",
		              targetNodeId, outMsg.getSensor());
	} else {
		Serial.println("❌ Falha ao publicar ACK de transporte");
	}
#endif
}

void processMQTTCommandNative(const char* topic, const byte* payload, unsigned int length) {
	// Tópico esperado: m360/{UF}/{CAR}/in/{nodeId}/{sensorId}/{cmd}/{ack}/{type}
	// Slashes: 0     1   2    3    4        5          6       7    8
	const char* p = topic;
	const char* slashes[9];
	int slashCount = 0;
	while (*p && slashCount < 9) {
		if (*p == '/') slashes[slashCount++] = p;
		p++;
	}
	if (slashCount < 8) {
		Serial.println("❌ Tópico nativo inválido (partes insuficientes)");
		return;
	}

	int nodeId   = atoi(slashes[3] + 1);
	int sensorId = atoi(slashes[4] + 1);
	int cmd      = atoi(slashes[5] + 1);
	int type     = atoi(slashes[7] + 1);

	if (nodeId < 1 || nodeId > 255) {
		Serial.println("❌ nodeId inválido no tópico nativo");
		return;
	}

	char payloadBuf[129];
	unsigned int len = length < 128 ? length : 128;
	memcpy(payloadBuf, payload, len);
	payloadBuf[len] = '\0';

	Serial.printf("📦 Nativo: nó=%d sensor=%d cmd=%d tipo=%d payload='%s'\n",
	              nodeId, sensorId, cmd, type, payloadBuf);

	DynamicJsonDocument doc(256);
	doc["nodeId"]   = nodeId;
	doc["sensorId"] = sensorId;
	doc["command"]  = cmd;
	doc["type"]     = type;
	doc["payload"]  = payloadBuf;

	processMQTTCommand(doc);
}

void processMQTTCommand(const JsonDocument& doc) {
	String payloadStr;
	serializeJson(doc, payloadStr);

	uint8_t   targetNodeId;
	MyMessage outMsg;

	if (M360::Translator::fromJSON(payloadStr, outMsg, targetNodeId)) {
		outMsg.setDestination(targetNodeId);
		// ACK de transporte apenas para V_STATUS (relés) — comandos administrativos
		// (V_CUSTOM, V_VAR1, etc.) usam fire-and-forget; confirmação semântica vem
		// pela resposta do nó (estados re-enviados ao broker).
		bool withAck = (outMsg.getType() == V_STATUS);
		bool success = send(outMsg, withAck);
		Serial.print("🎯 Comando enviado para Nó "); Serial.print(targetNodeId);
		Serial.println(success ? " ✅" : " ❌");
		ledFlicker(success ? LED_YELLOW : LED_RED);
		if (success && withAck) publishTransportAck(outMsg, targetNodeId);
	} else {
		Serial.println("❌ Erro ao decodificar JSON ou comando inválido");
	}
}

void processMQTTCommand(const String& payloadStr) {
	uint8_t   targetNodeId;
	MyMessage outMsg;

	if (M360::Translator::fromJSON(payloadStr, outMsg, targetNodeId)) {
		outMsg.setDestination(targetNodeId);
		bool withAck = (outMsg.getType() == V_STATUS);
		bool success = send(outMsg, withAck);
		Serial.print("🎯 Comando enviado para Nó "); Serial.print(targetNodeId);
		Serial.println(success ? " ✅" : " ❌");
		ledFlicker(success ? LED_YELLOW : LED_RED);
		if (success && withAck) publishTransportAck(outMsg, targetNodeId);
	} else {
		Serial.println("❌ Erro ao decodificar JSON ou comando inválido");
	}
}

// ==== RASTREAMENTO DE NÓS ====

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

	String jsonString  = M360::Translator::buildEvent(event, nodeId, details, WiFi.RSSI());
	String topicEvents = M360::buildTopicOut(config) + "/events";
	bool   success     = mqttClient.publish(topicEvents.c_str(), jsonString.c_str());

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

bool gatewayTransportInit()                        { return true;  }
bool gatewayTransportSend(MyMessage &/*message*/)  { return true;  }
bool gatewayTransportAvailable()                   { return false; }
