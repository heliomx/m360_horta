/*
 * M360Constants.h — Constantes globais e Enums para a rede M360 Horta
 */

#pragma once

#include <Arduino.h>

namespace M360 {

	// IDs de Sensores Reservados (Globais)
	static const uint8_t CHILD_ID_INTERVAL = 254; // V_VAR1 ou V_CUSTOM para intervalo
	static const uint8_t CHILD_ID_BATTERY  = 255; // V_VOLTAGE ou I_BATTERY_LEVEL

	// Comandos de Ação (Strings)
	static const char* const CMD_PUMP_ON	   = "PUMP_ON";
	static const char* const CMD_PUMP_OFF	  = "PUMP_OFF";
	static const char* const CMD_PUMP_TOGGLE   = "PUMP_TOGGLE";
	static const char* const CMD_FORCE_UPDATE  = "FORCE_UPDATE";
	static const char* const CMD_RESET		 = "RESET";

	// Tipos de Eventos de Transporte
	static const char* const EVT_GATEWAY_READY = "gateway_ready";
	static const char* const EVT_NODE_DISCOVER = "node_discovered";
	static const char* const EVT_NODE_LOST	 = "node_lost";
	static const char* const EVT_NODE_RECONN   = "node_reconnected";

} // namespace M360
