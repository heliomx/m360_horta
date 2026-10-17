/*
 * soilMux.cpp — 74HC4051: 8 sondas resistivas de solo em A0 (M360-LP)
 *
 * Divisor por canal já na placa: 10 kΩ para o VSW + RC 1 kΩ / 100 nF. Pull-up
 * e referência do ADC são o mesmo trilho de 3,3 V, então a escala ADC bruta
 * 0–1023 (alto = seco) é a mesma dos Nós 01/02 no Nano a 5 V.
 *
 * Nunca amostrar o A0 durante uma transação SPI com o rádio (§9, achado 8): a
 * lib lê e envia em sequência no mesmo contexto, então basta não chamar isto
 * de dentro de receive().
 */

#include "nodeConfig.h"
#if LP_HAS_SOIL

#include "lpDrivers.h"
#include <M360.h>

static void soilMuxSelect(uint8_t channel)
{
	digitalWrite(LP_PIN_MUX_S0, (channel & 0x01) ? HIGH : LOW);
	digitalWrite(LP_PIN_MUX_S1, (channel & 0x02) ? HIGH : LOW);
	digitalWrite(LP_PIN_MUX_S2, (channel & 0x04) ? HIGH : LOW);
}

void soilMuxInit()
{
	pinMode(LP_PIN_MUX_S0, OUTPUT);
	pinMode(LP_PIN_MUX_S1, OUTPUT);
	pinMode(LP_PIN_MUX_S2, OUTPUT);
	soilMuxIdle();
	pinMode(LP_PIN_MUX_OUT, INPUT);
}

void soilMuxIdle()
{
	soilMuxSelect(0); // S0–S2 em LOW: com o VSW cortado, nível alto alimentaria o CI
}

float soilMuxRead(uint8_t channel)
{
	if (channel > 7) {
		return NAN;
	}
	soilMuxSelect(channel);
	wait(5);                      // acomodação do canal (Ron do 4051 + RC)
	analogRead(LP_PIN_MUX_OUT);   // descarta: purga o sample-and-hold
	wait(3);
	int raw = analogRead(LP_PIN_MUX_OUT);

	Serial.print(F("SOIL ch"));
	Serial.print(channel);
	Serial.print(F(" ADC:"));
	Serial.println(raw);

	return (float)raw;
}

#endif // LP_HAS_SOIL
