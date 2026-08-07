/*
 * M360Constants.h — Constantes globais e Enums para a rede M360 Horta
 */

#pragma once

#include <Arduino.h>

namespace M360 {

	// IDs de Sensores Reservados (Globais)
	static const uint8_t CHILD_ID_STATUS   = 0;   // Status / Bateria / Saúde do Nó
	static const uint8_t CHILD_ID_INTERVAL = 254; // V_VAR1 ou V_CUSTOM para intervalo
	static const uint8_t CHILD_ID_BATTERY  = 255; // V_VOLTAGE ou I_BATTERY_LEVEL (legado)
	static const uint8_t CHILD_ID_DEBUG    = 253; // V_TEXT — debug remoto

	// Faixas Normativas de Node IDs (Camadas de Campo)
	static const uint8_t NODE_ID_GATEWAY      = 0;
	static const uint8_t NODE_ID_CLIMA_MIN    = 1;
	static const uint8_t NODE_ID_CLIMA_MAX    = 50;
	static const uint8_t NODE_ID_SOLO_MIN     = 51;
	static const uint8_t NODE_ID_SOLO_MAX     = 150;
	static const uint8_t NODE_ID_ACTUATOR_MIN = 151;
	static const uint8_t NODE_ID_ACTUATOR_MAX = 200;
	static const uint8_t NODE_ID_WATER_MIN    = 201;
	static const uint8_t NODE_ID_WATER_MAX    = 254;

	// Faixas Normativas de Child IDs (Funcionalidades de Campo)
	static const uint8_t CHILD_ID_SOLO_MIN     = 1;
	static const uint8_t CHILD_ID_SOLO_MAX     = 10;
	static const uint8_t CHILD_ID_CLIMA_MIN    = 11;
	static const uint8_t CHILD_ID_CLIMA_MAX    = 20;
	static const uint8_t CHILD_ID_FLOW_MIN     = 21;
	static const uint8_t CHILD_ID_FLOW_MAX     = 30;
	static const uint8_t CHILD_ID_ACTUATOR_MIN = 31;
	static const uint8_t CHILD_ID_ACTUATOR_MAX = 40;


	// Comandos de Ação (Strings)
	// Usados pelo M360Translator (gateway) para ações simplificadas do MQTT
	// PUMP_ON/OFF → traduzidos para C_SET+V_STATUS (tratados por handleMessage)
	// PUMP_TOGGLE → traduzido para C_SET+V_CUSTOM (deve ser tratado pelo receive() do nó)
	static const char* const CMD_PUMP_ON      = "PUMP_ON";
	static const char* const CMD_PUMP_OFF     = "PUMP_OFF";
	static const char* const CMD_PUMP_TOGGLE  = "PUMP_TOGGLE";

	// Tratados por M360Node::handleMessage
	static const char* const CMD_FORCE_UPDATE = "FORCE_UPDATE";
	static const char* const CMD_REPRESENT    = "REPRESENT";
	static const char* const CMD_DEBUG_NET    = "DEBUG_NET"; // solicita diagnóstico de rede via CHILD_ID_DEBUG

	// Tipos de Eventos de Transporte
	static const char* const EVT_GATEWAY_READY = "gateway_ready";
	static const char* const EVT_NODE_DISCOVER = "node_discovered";
	static const char* const EVT_NODE_LOST	 = "node_lost";
	static const char* const EVT_NODE_RECONN   = "node_reconnected";

	// Impressão estática de informações do firmware (Arquivo Fonte, Data e Hora)
	#define M360_PRINT_BUILD_INFO() do { \
		Serial.print(F("Arquivo: ")); \
		Serial.println(F(__FILE__)); \
		Serial.print(F("Compilado em: ")); \
		Serial.print(F(__DATE__)); \
		Serial.print(F(" ")); \
		Serial.println(F(__TIME__)); \
	} while(0)

} // namespace M360
