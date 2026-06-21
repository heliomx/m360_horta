/*
 * M360Config.h — Constantes globais da biblioteca M360-DRY
 *
 * Substitui config.h + powerProfile.h dos nós legados.
 * Inclua este header nos nós em vez dos arquivos shared/.
 */

#pragma once
#include <Arduino.h>

// ===== EEPROM — MAPA DE MEMÓRIA UNIFICADO =====
//
//   0   – 511  : MySensors Core          (reservado pela lib — nunca escrever)
//   512 – 515  : M360NodeConfig          magic (2 B) + interval (2 B) [M360_EEPROM_INTERVAL_ADDRESS]
//   516 – 520  : M360NodeConfig          Reservado para expansão futura
//   521+       : M360DeviceConfig        WiFi / MQTT / UF / CAR + CRC [M360_EEPROM_DEVICE_CONFIG_ADDRESS]
//
// Nós AVR usam apenas 512–515. O gateway ESP8266 usa apenas 521+.
// Não há sobreposição.

#define M360_EEPROM_MAGIC            0x36D1  // Identificador da lib M360-DRY
#define M360_EEPROM_INTERVAL_ADDRESS 512
#define M360_EEPROM_DEVICE_BASE      521

namespace M360 {

	// Configuração básica de nó (presente em AVR e ESP)
	typedef struct {
		uint16_t magicBuffer;
		uint16_t interval;
	} M360NodeConfig;

#ifdef ESP8266
	// Configuração de Infraestrutura (exclusiva ESP8266)
	typedef struct {
		uint8_t  version;
		char     ssid[32];
		char     password[32];
		char     mqttServer[32];
		int      mqttPort;
		char     mqttUser[32];
		char     mqttPassword[32];
		char     uf[4];           // UF (ex: "DF")
		char     carNumber[8];    // Número do CAR (ex: "0001")
		uint16_t crc;
	} M360DeviceConfig;

	// Gerenciador de Configuração (exclusivo ESP8266)
	class Config {
	public:
		static void load(M360DeviceConfig& cfg);
		static void save(const M360DeviceConfig& cfg);
		static void reset(M360DeviceConfig& cfg);
		static bool isValid(const M360DeviceConfig& cfg);
		static uint16_t calculateCRC(const M360DeviceConfig& cfg);
	};
#endif

} // namespace M360

// ===== INTERVALO (minutos) =====

#ifndef M360_DEFAULT_INTERVAL
#  define M360_DEFAULT_INTERVAL 1
#endif

#ifndef M360_MIN_INTERVAL
#  define M360_MIN_INTERVAL 1
#endif

#ifndef M360_MAX_INTERVAL
#  define M360_MAX_INTERVAL 1440
#endif

// ===== CHILD IDs RESERVADOS =====

#define M360_CHILD_ID_INTERVAL 254  // V_VAR1
#define M360_CHILD_ID_BATTERY  255  // V_VOLTAGE

// ===== TIMING =====

#define M360_MIN_AWAKE_MS 3000UL

// ===== BATERIA =====

#ifndef M360_MIN_VOLTAGE
#  define M360_MIN_VOLTAGE 3.0f
#endif

#ifndef M360_MAX_VOLTAGE
#  define M360_MAX_VOLTAGE 4.2f
#endif

// ===== LEITURA INVÁLIDA =====

#define M360_SENSOR_INVALID -32768.0f

// ===== COMANDOS REMOTOS =====

// ===== PERFIL DE ENERGIA =====
// Selecione o perfil no constructor do nó via M360PowerProfile:
//   M360::M360_LOW_POWER   — bateria: usa smartSleep()
//   M360::M360_ALWAYS_ON   — fonte fixa / debug: timer por millis, sem sleep
