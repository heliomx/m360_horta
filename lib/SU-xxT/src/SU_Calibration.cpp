/*
 * SU_Calibration.cpp — Defaults de fábrica e CRC da calibração
 */

#include "SU_Calibration.h"
#include <string.h>
#include <stddef.h>   // offsetof

void SU_calibrationDefaults(SU_CalibrationData& cal)
{
	// Zerar inclui os bytes de PADDING, e isso não é detalhe: o CRC cobre a
	// struct inteira. Padding indeterminado daria CRCs diferentes para o mesmo
	// conteúdo lógico, e a calibração seria rejeitada em boots alternados.
	// cppcheck-suppress memsetClassFloat
	memset(&cal, 0, sizeof(cal));

	cal.magic         = SU_CALIB_MAGIC;
	cal.layoutVersion = SU_CALIB_LAYOUT_VERSION;

	// --- pH: eletrodo E201C nominal, calibrado a 25 °C ---
	// Valores teóricos de um eletrodo ideal com offset de 1,65 V (Vcc/2):
	// slope de Nernst a 25 °C = 59,16 mV/pH.
	cal.voltagePH7  = 1.650f;
	cal.voltagePH4  = 1.650f + 3.0f * 0.05916f;   // ácido: tensão acima do isopotencial
	cal.voltagePH10 = 1.650f - 3.0f * 0.05916f;   // alcalino: abaixo
	cal.calibTempC  = 25.0f;

	// --- EC ---
	cal.kCell           = 1.0f;
	cal.alphaTemp       = 0.0191f;   // 1,91 %/°C — padrão para solução de solo
	cal.ecOffsetVoltage = 0.0f;

	// --- Temperatura assumida nos modelos sem DS18B20 ---
	cal.assumedTempC = 25.0f;

	// --- Umidade: extremos do ADS1115 em ±4,096 V ---
	// Ar (seco) = leitura alta, água (saturado) = leitura baixa, como nas sondas
	// resistivas dos nós 1 e 2. Calibrar em campo: estes são chutes de partida.
	cal.airAdc10   = 20000;
	cal.waterAdc10 = 7000;
	cal.airAdc30   = 20000;
	cal.waterAdc30 = 7000;

	// --- Nível ---
	cal.levelThresholdAdc = 10000;
	cal.invertedLevel     = false;
	cal.levelProbeType    = SU_LEVEL_OPTICAL;

	// --- Excitação AC ---
	cal.acExciteHz     = 1000;
	cal.acExcitePulses = 16;
	cal.acSettleUs     = 2000;

	// --- Troca da reserva ---
	// 4 ciclos. Com o intervalo padrão de 30 min, são ~2 h de lâmina nova antes
	// de revalidar pH e EC. A DEFINIR POR MEDIÇÃO — ver hardware/SU-xxT §3.3.
	cal.minRechargeCycles = 4;
	cal.rechargeSettled   = false;

	cal.crc = SU_calibrationCRC(cal);
}

uint16_t SU_calibrationCRC(const SU_CalibrationData& cal)
{
	// CRC-16/CCITT sobre a struct, pulando o próprio campo `crc`.
	const uint8_t* bytes  = reinterpret_cast<const uint8_t*>(&cal);
	const size_t   crcOff = offsetof(SU_CalibrationData, crc);

	uint16_t crc = 0xFFFF;

	for (size_t i = 0; i < sizeof(SU_CalibrationData); i++) {
		if (i >= crcOff && i < crcOff + sizeof(cal.crc)) {
			continue;
		}
		crc ^= (uint16_t)bytes[i] << 8;
		for (uint8_t bit = 0; bit < 8; bit++) {
			crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
		}
	}
	return crc;
}
