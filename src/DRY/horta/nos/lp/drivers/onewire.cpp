/*
 * onewire.cpp — DS18B20 no A1 da M360-LP (porta J2, pino 1)
 *
 * Pull-up de 4,7 kΩ para o VSW está na placa. Só o primeiro sensor do
 * barramento é lido (getTempCByIndex(0)), como no Nó 04.
 */

#include "nodeConfig.h"
#if LP_HAS_ONEWIRE

#include "lpDrivers.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire oneWire(LP_PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

void ds18PowerUp()
{
	sensors.begin();
	sensors.requestTemperatures(); // a conversão (750 ms) corre durante o wait() de powerUpSensors()
}

void ds18PowerDown()
{
	pinMode(LP_PIN_ONEWIRE, INPUT);
}

float ds18Read()
{
	float tempC = sensors.getTempCByIndex(0);
	if (tempC == DEVICE_DISCONNECTED_C || tempC < -20.0f || tempC > 85.0f) {
		return NAN;
	}
	return tempC;
}

#endif // LP_HAS_ONEWIRE
