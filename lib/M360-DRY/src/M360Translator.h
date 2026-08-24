/*
 * M360Translator.h — Tradutor de mensagens MySensors <-> JSON Envelope M360
 */

#pragma once

#ifndef ESP8266
#  error "M360Translator.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>
#include <core/MyMessage.h>
#include <ArduinoJson.h>
#include "M360Config.h"
#include "M360Registry.h"

namespace M360 {

	// Resultado da decodificação de um comando vindo do broker.
	// Existe para que o gateway possa dizer no MQTT *por que* rejeitou — antes,
	// todo comando malformado sumia com um println no Serial.
	typedef enum {
		M360_CMD_OK = 0,
		M360_CMD_ERR_NODE_ID,
		M360_CMD_ERR_SENSOR_ID,
		M360_CMD_ERR_COMMAND,
		M360_CMD_ERR_TYPE,
		M360_CMD_ERR_PAYLOAD_SIZE,
		M360_CMD_ERR_PAYLOAD_VALUE
	} M360CommandStatus;

	class Translator {
	public:
		// Tamanhos de buffer recomendados (baseados na arquitetura)
		static const size_t DOC_SIZE_MSG   = 512;
		static const size_t DOC_SIZE_HB	= 384;
		static const size_t DOC_SIZE_EVENT = 256;

		/*
		 * Converte uma MyMessage para o envelope JSON padrão M360.
		 *
		 * @param nodeIdOverride  Se >= 0, substitui msg.getSender() no campo
		 *        "nodeId". Necessário para ACKs: em mensagens *enviadas* pelo
		 *        gateway getSender() vale 0 (o próprio gateway), não o destino.
		 */
		static String toJSON(const MyMessage& msg, bool isAck = false,
		                     int nodeIdOverride = -1);

		/*
		 * Decodifica um comando no formato nativo MySensors, cujos campos já
		 * vieram do tópico. Não passa por JSON: o caminho antigo montava um
		 * JsonDocument, serializava e reparseava só para chegar aqui — três
		 * alocações de heap por comando, e um doc subdimensionado que descartava
		 * o payload em silêncio quando estourava.
		 *
		 * @param payload  String NUL-terminada com o payload bruto do MQTT.
		 * @return M360_CMD_OK, ou o motivo da rejeição.
		 */
		static M360CommandStatus fromNative(int nodeId, int sensorId,
		                                    int command, int type,
		                                    const char* payload,
		                                    MyMessage& outMsg,
		                                    uint8_t& targetNode);

		/*
		 * Valida uma MyMessage já montada, antes de ir para o rádio.
		 * Ponto único de checagem: vale tanto para o caminho nativo quanto para
		 * o envelope JSON.
		 */
		static M360CommandStatus validate(const MyMessage& msg,
		                                  uint8_t targetNode);

		// Descrição legível do status, para log Serial e evento MQTT.
		static const char* describeStatus(M360CommandStatus status);

		/*
		 * Constrói o JSON de Heartbeat do gateway.
		 */
		static String buildHeartbeat(const M360DeviceConfig& cfg, int rssi = 0, const char* version = "2.0");

		/*
		 * Constrói o JSON de Evento de Transporte.
		 */
		static String buildEvent(const char* event, int nodeId = 0, const char* details = "", int rssi = 0);

		/*
		 * Constrói o JSON de Métricas MQTT.
		 */
		static String buildMetrics(const M360DeviceConfig& cfg, const struct MQTTMetrics& metrics);

		/*
		 * Decodifica um JSON vindo do MQTT para uma MyMessage ou Ação Simplificada.
		 * Retorna true se a mensagem foi preenchida com sucesso.
		 * @param json	   String JSON recebida.
		 * @param outMsg	 Mensagem MySensors a ser preenchida.
		 * @param targetNode Nó destino extraído do JSON.
		 */
		static bool fromJSON(const String& json, MyMessage& outMsg, uint8_t& targetNode);

		// Retorna descrição amigável do tipo MySensors (V_... para C_SET/C_REQ,
		// I_... para C_INTERNAL — os dois enums compartilham o mesmo espaço
		// numérico, então o command é obrigatório para desambiguar).
		static const char* getTypeDescription(uint8_t command, uint8_t type);

		/*
		 * Gera descrição humanizada da mensagem (em português).
		 * Exemplo Envio: "Nó 99 (Central de Relés): Ligar Sol.CanteiroA [Child 31]"
		 * Exemplo Recebido: "Do Nó 99 (Central de Relés): Sol.CanteiroA ligada [Child 31]"
		 */
		static String formatHumanDescription(const NodeRegistry& registry,
		                                    uint8_t nodeId,
		                                    uint8_t childId,
		                                    uint8_t command,
		                                    uint8_t type,
		                                    const char* payload,
		                                    bool isSending);

		// Modo nativo MySensors MQTT (usar com -D M360_NATIVE_MQTT=1 no platformio.ini)
		// buildNativeTopic: monta {prefix}/{nodeId}/{sensorId}/{command}/{ack}/{type}
		// toNativePayload: retorna o payload bruto da mensagem como String
		static String buildNativeTopic(const String& prefix, const MyMessage& msg);
		static String toNativePayload(const MyMessage& msg);
	};

} // namespace M360
