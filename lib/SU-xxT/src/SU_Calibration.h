/*
 * SU_Calibration.h — Calibração de campo da família SU-xxT
 *
 * Persistida na EEPROM, dentro da região de aplicação declarada pelo mapa
 * unificado da M360-DRY (M360_EEPROM_APP_BASE = 768).
 *
 * As constantes de endereço vivem AQUI, não no M360Config.h: a lib core não
 * conhece libs de aplicação. O M360Config.h declara a região; esta lib
 * reivindica a fatia e registra a ocupação no comentário daquele mapa.
 */

#pragma once
#include <Arduino.h>
#include "SU_Types.h"

// ===== EEPROM =====
//
// Mapa unificado (lib/M360-DRY/src/M360Config.h):
//   0   – 511  : MySensors Core (usa até 412; 413–511 é folga)
//   512 – 515  : M360NodeConfig — magic + interval
//   516 – 520  : reservado
//   521 – 767  : M360DeviceConfig — só ESP8266
//   768 – 1023 : Aplicação do nó (AVR)  ← estamos aqui
//
// ATENÇÃO — ESP: a EEPROM é emulada e o MySensors a abre com EEPROM.begin(512).
// Escrever acima de 511 exige reabrir com o tamanho necessário e commit().
// No AVR o acesso é direto. Ver SU_xxT.cpp.

#define SU_EEPROM_CALIB_ADDRESS  768
#define SU_EEPROM_CALIB_SIZE     64
#define SU_CALIB_MAGIC           0x5530   // "SU" — distingue bloco gravado de EEPROM virgem (0xFF)
#define SU_CALIB_LAYOUT_VERSION  1

// ===== ESTRUTURA =====
//
// Campos ordenados por tamanho decrescente para minimizar padding em ESP32
// (4 bytes de alinhamento). No AVR não há padding.

typedef struct {
	// --- pH: três pontos + a temperatura em que foram medidos ---
	//
	// calibTempC NÃO é opcional. O slope medido em campo já embute a
	// temperatura da calibração; multiplicá-lo pelo fator de Nernst cru é
	// DUPLA COMPENSAÇÃO. Sem este campo a compensação sequer é definível.
	// Ver ARCHITECTURE.md §7.
	float voltagePH4;
	float voltagePH7;      // ponto isopotencial
	float voltagePH10;
	float calibTempC;

	// --- EC ---
	float kCell;            // constante de célula
	float alphaTemp;        // coeficiente térmico (padrão 0,0191 /°C)
	float ecOffsetVoltage;

	// --- Temperatura assumida nos modelos SEM DS18B20 ---
	//
	// SU-20 e SU-30 não têm T_solo. O custo é assimétrico: ~0,10 pH contra
	// ~19 % na EC a 10 °C de desvio. Ver README.md.
	float assumedTempC;

	// --- Umidade: ADC no ar (seco) e na água (saturado) ---
	int16_t airAdc10;
	int16_t waterAdc10;
	int16_t airAdc30;
	int16_t waterAdc30;

	// --- Nível ---
	int16_t levelThresholdAdc;

	// --- Excitação AC (comum a EC e ao nível condutivo) ---
	uint16_t acExciteHz;     // padrão 1000
	uint16_t acSettleUs;     // acomodação do envelope retificado

	// --- Troca da reserva ---
	//
	// CICLOS, não milissegundos. Em AVR millis() NÃO avança durante
	// sleep()/smartSleep(): um temporizador em ms jamais venceria e pH e EC
	// nunca voltariam a ser publicados. Ver ARCHITECTURE.md §2.
	uint16_t minRechargeCycles;

	// --- Cabeçalho de persistência ---
	uint16_t magic;
	uint16_t crc;

	uint8_t layoutVersion;
	uint8_t acExcitePulses;  // padrão 16
	uint8_t levelProbeType;  // SU_LevelProbeType
	bool    invertedLevel;

	// Flag de acomodação persistida na TRANSIÇÃO, não a cada ciclo — uma
	// escrita por reenchimento, e não uma por ciclo, que consumiria os 100 mil
	// ciclos da EEPROM em poucos anos a 30 min de intervalo.
	bool    rechargeSettled;
} SU_CalibrationData;

// A região reservada tem que comportar a struct. Se esta asserção falhar, ou a
// struct encolhe, ou SU_EEPROM_CALIB_SIZE cresce — e nesse caso o mapa em
// M360Config.h precisa registrar a nova extensão da fatia.
static_assert(sizeof(SU_CalibrationData) <= SU_EEPROM_CALIB_SIZE,
              "SU_CalibrationData nao cabe na fatia reservada de EEPROM");

// ===== DEFAULTS DE FÁBRICA =====
//
// Carregados quando magic, layoutVersion ou CRC não conferem. Nunca operar com
// calibração de procedência duvidosa em silêncio: bytes de um layout antigo
// lidos como válidos produziriam pH e EC plausíveis e ERRADOS, que é o pior
// modo de falha possível aqui.
void SU_calibrationDefaults(SU_CalibrationData& cal);

// CRC-16/CCITT sobre a struct, excluindo o próprio campo `crc`.
uint16_t SU_calibrationCRC(const SU_CalibrationData& cal);
