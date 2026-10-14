/*
 * battery.cpp — Bateria pelo A6 da M360-LP (só LP_POWER_BATTERY)
 *
 * Por que não o bandgap puro da lib (M360Power.cpp): com o MCP1700 em
 * regulação, o VCC é 3,3 V e não diz nada sobre a célula. Por que não o A6
 * contra o VCC: abaixo de ~3,31 V de bateria o LDO sai de regulação e o VCC
 * acompanha a célula — a conta (VBAT/3)/VCC devolve sempre "3,3 V". Daí a
 * correção de ARQUITETURA_FAMILIA.md §4.1:
 *
 *     VBAT = 3 × ADC_A6 × VCC_medido_no_bandgap / 1023
 *
 * Sobrescreve M360::readBatteryVoltage() (fraca na lib), então o child 255 e
 * o percentual MySensors passam a refletir a célula.
 */

#include "nodeConfig.h"
#if LP_POWER_BATTERY

#include "lpDrivers.h"
#include <M360.h>

#ifndef LP_VBAT_GAIN
#  define LP_VBAT_GAIN   1.0f   // calibração em dois pontos: VBAT_real = GAIN × VBAT + OFFSET
#endif
#ifndef LP_VBAT_OFFSET
#  define LP_VBAT_OFFSET 0.0f
#endif

// VCC em volts medido contra a referência interna de 1,1 V (mesma sequência da lib).
static float readVccBandgap()
{
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	wait(2);                           // o bandgap precisa estabilizar depois do MUX trocar
	ADCSRA |= _BV(ADSC);
	while (bit_is_set(ADCSRA, ADSC)) {
		;
	}
	uint16_t result = (uint16_t)((ADCH << 8) | ADCL);
	if (result == 0) {
		return 0.0f;
	}
	return 1125300.0f / (float)result / 1000.0f; // 1,1 × 1023 × 1000
}

float lpReadVbat()
{
	float vcc = readVccBandgap();
	if (vcc <= 0.0f) {
		return 0.0f;
	}
	analogReference(DEFAULT);          // A6 contra o VCC, nunca contra 1,1 V (satura > 3,3 V)
	analogRead(LP_PIN_VBAT);           // descarta: ADMUX acabou de mudar de canal e referência
	wait(2);
	int raw = analogRead(LP_PIN_VBAT);
	float vbat = LP_VBAT_DIVIDER * (float)raw * vcc / 1023.0f;
	return LP_VBAT_GAIN * vbat + LP_VBAT_OFFSET;
}

namespace M360
{
	float readBatteryVoltage()
	{
		return lpReadVbat();
	}
}

#endif // LP_POWER_BATTERY
