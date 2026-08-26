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

// Habilita prints de depuração gerais (RF24 detalhado desativado para logs limpos)
#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RF24

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
void processMQTTCommandNative(const char* topic, const byte* payload, unsigned int length);
void publishTransportEvent(const char* event, const char* details = "", int nodeId = 0);
void checkNodeTimeouts();
void updateLEDStatus();

// ==== FUNÇÕES MYSENSORS ====

void presentation() {
	sendSketchInfo("Manejo360 Gateway MQTT", "2.0.0");
	Serial.println("📡 Gateway MySensors apresentado (v2.0.0)");
	// publishTransportEvent movido para setup() — MQTT não está inicializado aqui
}

void receive(const MyMessage &message) {
	uint8_t nodeId = message.getSender();
	uint8_t childId = message.getSensor();
	uint8_t cmd = message.getCommand();

	ledFlicker(LED_GREEN);

	char payloadDbg[MAX_PAYLOAD + 1] = "";
	if (message.getLength() > 0) {
		message.getString(payloadDbg);
	}

	if (gateway.registry().update(nodeId)) {
		publishTransportEvent(M360::EVT_NODE_RECONN, "Node back online or discovered", nodeId);
	}

	// Auto-Discovery: Tratar Apresentação de Childs (C_PRESENTATION)
	if (cmd == C_PRESENTATION) {
		char aliasBuf[MAX_PAYLOAD + 1] = "";
		message.getString(aliasBuf);
		gateway.registry().registerChild(nodeId, childId, message.getType(), aliasBuf);
		char details[128];
		snprintf(details, sizeof(details), "Child presentation: ID %d, Type %d, Label: %s", childId, message.getType(), aliasBuf);
		publishTransportEvent("child_presentation", details, nodeId);
	}

	// Auto-Discovery: Tratar Mensagens Internas (C_INTERNAL)
	if (cmd == C_INTERNAL) {
		switch (message.getType()) {
			case I_PRESENTATION:
				publishTransportEvent("node_presentation", "Node presented itself to gateway", nodeId);
				break;
			case I_SKETCH_NAME: {
				char nameBuf[MAX_PAYLOAD + 1] = "";
				message.getString(nameBuf);
				gateway.registry().registerSketchName(nodeId, nameBuf);
				char details[128];
				snprintf(details, sizeof(details), "Sketch Name: %s", nameBuf);
				publishTransportEvent("node_sketch_name", details, nodeId);
				break;
			}
			case I_SKETCH_VERSION: {
				char verBuf[MAX_PAYLOAD + 1] = "";
				message.getString(verBuf);
				char details[128];
				snprintf(details, sizeof(details), "Sketch Version: %s", verBuf);
				publishTransportEvent("node_sketch_version", details, nodeId);
				break;
			}
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
				char discBuf[MAX_PAYLOAD + 1] = "";
				String details = "Node discovered, parent: ";
				details += message.getString(discBuf);
				publishTransportEvent(M360::EVT_NODE_DISCOVER, details.c_str(), nodeId);
				break;
			}
		}
	}

	// Capturar intervalo de telemetria anunciado pelo nó (Child 254 / V_VAR1).
	// O nó publica isto na apresentação, no REPRESENT e como eco de todo C_SET de
	// intervalo — inclusive quando rejeita o valor, caso em que ecoa o vigente.
	//
	// Exigir cmd == C_SET não é zelo: os números de tipo se repetem entre as
	// classes de comando (V_VAR1 vale 24, e S_DUST na apresentação também).
	// Hoje escapa só porque o child 254 é apresentado como S_CUSTOM (23).
	// !isAck() descarta o ACK de transporte que o próprio gateway fabrica — ele
	// prova apenas que o próximo salto respondeu, não que o nó aplicou algo.
	if (cmd == C_SET && !message.isAck() &&
	    childId == M360::CHILD_ID_INTERVAL &&
	    (message.getType() == V_VAR1 || message.getType() == V_VAR5)) {
		uint16_t iv = (uint16_t)atoi(payloadDbg);
		if (iv > 0) {
			gateway.registry().registerInterval(nodeId, iv);
		}
	}

	// Feedback Inteligível em Linguagem Natural
	String humanLog = M360::Translator::formatHumanDescription(
		gateway.registry(), nodeId, childId, cmd, message.getType(), payloadDbg, false
	);
	Serial.printf("📨 [RECEBIDO] %s\n", humanLog.c_str());

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

	M360_PRINT_BUILD_INFO();
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
	// Um único parse: Translator::fromJSON() já desserializa. Desserializar aqui
	// para reserializar dentro de processMQTTCommand() era um round-trip duplo.
	String payloadStr;
	payloadStr.reserve(length + 1);
	for (unsigned int i = 0; i < length; i++) {
		payloadStr += (char)payload[i];
	}
	processMQTTCommand(payloadStr);
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
	// Modo nativo: publica o ACK no tópico estruturado com o campo ack=1.
	// buildNativeTopic() NÃO serve aqui — usa getSender(), que vale 0 no contexto
	// de envio. O tópico é montado com targetNodeId explícito.
	if (!mqttClient.connected()) return;
	String topic = M360::buildTopicOut(config);
	topic += '/'; topic += targetNodeId;
	topic += '/'; topic += outMsg.getSensor();
	topic += '/'; topic += (uint8_t)outMsg.getCommand();
	topic += "/1/";  // ack = 1
	topic += outMsg.getType();
	String payload = M360::Translator::toNativePayload(outMsg);
	if (mqttClient.publish(topic.c_str(), payload.c_str())) {
		Serial.printf("📤 ACK de transporte publicado — nó %d sensor %d\n",
		              targetNodeId, outMsg.getSensor());
	} else {
		Serial.println("❌ Falha ao publicar ACK de transporte");
	}
#else
	if (!mqttClient.connected()) return;
	// toJSON() já aceita o nodeId de destino: antes era preciso serializar,
	// reparsear, corrigir o campo e reserializar só para trocar um inteiro.
	String ackJson  = M360::Translator::toJSON(outMsg, true, targetNodeId);
	String topicOut = M360::buildTopicOut(config);
	if (mqttClient.publish(topicOut.c_str(), ackJson.c_str())) {
		Serial.printf("📤 ACK de transporte publicado — nó %d sensor %d\n",
		              targetNodeId, outMsg.getSensor());
	} else {
		Serial.println("❌ Falha ao publicar ACK de transporte");
	}
#endif
}

// Publica a rejeição de um comando no tópico de eventos. Sem isto, um comando
// malformado sumia com um println no Serial e o Node-RED só percebia pelos
// 35 s de timeout do Sincronizador — sem nunca saber a causa.
static void reportCommandRejected(const char* reason, int nodeId = 0) {
	Serial.print("❌ Comando rejeitado: ");
	Serial.println(reason);
	ledFlicker(LED_RED);
	publishTransportEvent("command_rejected", reason, nodeId);
}

// Converte um segmento do tópico em inteiro 0-255, exigindo que seja realmente
// numérico. atoi() devolvia 0 em silêncio para segmento vazio ou não numérico
// ("…/in/99/xx/1/0/2" virava sensor 0), e o comando era descartado pelo nó sem
// deixar rastro em lugar nenhum.
static bool parseTopicByte(const char* begin, const char* end, int& out) {
	if (begin >= end) {
		return false;  // segmento vazio
	}
	int value = 0;
	for (const char* p = begin; p < end; p++) {
		if (*p < '0' || *p > '9') {
			return false;
		}
		value = value * 10 + (*p - '0');
		if (value > 255) {
			return false;
		}
	}
	out = value;
	return true;
}

// Ponto único de saída para o rádio: valida, envia, e reporta o resultado.
// Usado tanto pelo caminho nativo quanto pelo envelope JSON.
static void dispatchCommand(MyMessage& outMsg, uint8_t targetNodeId) {
	const M360::M360CommandStatus status =
	    M360::Translator::validate(outMsg, targetNodeId);
	if (status != M360::M360_CMD_OK) {
		reportCommandRejected(M360::Translator::describeStatus(status), targetNodeId);
		return;
	}

	outMsg.setDestination(targetNodeId);
	// Envio direto via RF24 (com Auto-ACK de hardware pelo chip).
	// Não usa requestEcho síncrono para evitar bloqueio e colisão de pacotes.
	const bool success = send(outMsg, false);

	char payloadBuf[MAX_PAYLOAD + 1] = "";
	outMsg.getString(payloadBuf);
	String humanDesc = M360::Translator::formatHumanDescription(
		gateway.registry(), targetNodeId, outMsg.getSensor(),
		outMsg.getCommand(), outMsg.getType(), payloadBuf, true
	);
	Serial.printf("🎯 [ENVIO] %s -> %s\n", humanDesc.c_str(), success ? "✅ Sucesso" : "❌ Falha");
	ledFlicker(success ? LED_YELLOW : LED_RED);

	if (success) {
		// NÃO registrar aqui o intervalo comandado: `success` é apenas o auto-ACK
		// de hardware do RF24 do PRÓXIMO SALTO, não prova que o nó recebeu nem que
		// aceitou o valor. Um nó M360_LOW_POWER fica acordado ~3 s por ciclo e
		// perde quase todo comando; um valor fora de M360_MIN/MAX_INTERVAL é
		// rejeitado pelo nó. Registrar por otimismo estende o timeout do registro
		// (até 36 h) para um nó que segue na cadência antiga — cegando justamente
		// a detecção de nó morto a bateria.
		// A fonte da verdade é o eco do nó: M360Node::handleMessage() sempre
		// devolve o intervalo VIGENTE no child 254, inclusive quando rejeita.
		// Esse eco é capturado em receive().
		publishTransportAck(outMsg, targetNodeId);
	} else {
		char details[64];
		snprintf(details, sizeof(details), "send() falhou para child %u",
		         outMsg.getSensor());
		publishTransportEvent("command_send_failed", details, targetNodeId);
	}
}

void processMQTTCommandNative(const char* topic, const byte* payload, unsigned int length) {
	// Tópico esperado: m360/{UF}/{CAR}/in/{nodeId}/{sensorId}/{cmd}/{ack}/{type}
	// Índice da barra:      0   1     2   3        4          5     6     7
	const char* slashes[9];
	uint8_t slashCount = 0;
	for (const char* p = topic; *p; p++) {
		if (*p != '/') {
			continue;
		}
		if (slashCount >= 9) {
			slashCount = 9;  // já passou do formato — basta saber que excedeu
			break;
		}
		slashes[slashCount++] = p;
	}
	// Exatamente 8: nem menos (tópico truncado) nem mais (níveis extras que a
	// assinatura com '#' também entrega aqui).
	if (slashCount != 8) {
		Serial.printf("❌ Tópico nativo com %u barras (esperado 8): %s\n",
		              slashCount, topic);
		reportCommandRejected("topico nativo com numero de niveis invalido");
		return;
	}

	const char* topicEnd = topic + strlen(topic);
	int nodeId, sensorId, cmd, ack, type;
	if (!parseTopicByte(slashes[3] + 1, slashes[4], nodeId)   ||
	    !parseTopicByte(slashes[4] + 1, slashes[5], sensorId) ||
	    !parseTopicByte(slashes[5] + 1, slashes[6], cmd)      ||
	    !parseTopicByte(slashes[6] + 1, slashes[7], ack)      ||
	    !parseTopicByte(slashes[7] + 1, topicEnd,   type)) {
		Serial.printf("❌ Segmento não numérico no tópico: %s\n", topic);
		reportCommandRejected("segmento nao numerico no topico nativo");
		return;
	}
	(void)ack;  // o campo ack do tópico de entrada não é usado no envio

	// Buffer do tamanho exato que o rádio comporta. Copiar 128 bytes para depois
	// truncar em 25 dentro de MyMessage::set() só escondia payloads inválidos.
	if (length > MAX_PAYLOAD_SIZE) {
		Serial.printf("❌ Payload de %u bytes (máx %u)\n", length, MAX_PAYLOAD_SIZE);
		reportCommandRejected(
		    M360::Translator::describeStatus(M360::M360_CMD_ERR_PAYLOAD_SIZE), nodeId);
		return;
	}
	char payloadBuf[MAX_PAYLOAD_SIZE + 1];
	memcpy(payloadBuf, payload, length);
	payloadBuf[length] = '\0';

	Serial.printf("📦 Nativo: nó=%d sensor=%d cmd=%d tipo=%d payload='%s'\n",
	              nodeId, sensorId, cmd, type, payloadBuf);

	// Direto para MyMessage: sem JsonDocument intermediário, sem serializar e
	// reparsear. Os campos já vieram decodificados do tópico.
	MyMessage outMsg;
	uint8_t   targetNodeId;
	const M360::M360CommandStatus status = M360::Translator::fromNative(
	    nodeId, sensorId, cmd, type, payloadBuf, outMsg, targetNodeId);

	if (status != M360::M360_CMD_OK) {
		reportCommandRejected(M360::Translator::describeStatus(status), nodeId);
		return;
	}

	dispatchCommand(outMsg, targetNodeId);
}

// Caminho do envelope JSON (gateway compilado sem M360_NATIVE_MQTT).
void processMQTTCommand(const String& payloadStr) {
	uint8_t   targetNodeId;
	MyMessage outMsg;

	if (!M360::Translator::fromJSON(payloadStr, outMsg, targetNodeId)) {
		reportCommandRejected("JSON invalido, incompleto ou acao desconhecida");
		return;
	}

	dispatchCommand(outMsg, targetNodeId);
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
