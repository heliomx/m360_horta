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

namespace M360 {

	class Translator {
	public:
		// Tamanhos de buffer recomendados (baseados na arquitetura)
		static const size_t DOC_SIZE_MSG   = 512;
		static const size_t DOC_SIZE_HB	= 384;
		static const size_t DOC_SIZE_EVENT = 256;

		/*
		 * Converte uma MyMessage para o envelope JSON padrão M360.
		 */
		static String toJSON(const MyMessage& msg, bool isAck = false);

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

		// Retorna descrição amigável do tipo MySensors (V_..., I_...)
		static const char* getTypeDescription(uint8_t type);

		// Modo nativo MySensors MQTT (usar com -D M360_NATIVE_MQTT=1 no platformio.ini)
		// buildNativeTopic: monta {prefix}/{nodeId}/{sensorId}/{command}/{ack}/{type}
		// toNativePayload: retorna o payload bruto da mensagem como String
		static String buildNativeTopic(const String& prefix, const MyMessage& msg);
		static String toNativePayload(const MyMessage& msg);
	};

} // namespace M360
