/*
 * sensorDrivers.cpp — Implementação dos Drivers do miniDHT (Kit Hélio)
 */

#include "sensorDrivers.h"
#include <DHT.h>

// Objeto estático do driver DHT11
static DHT dht(PIN_DHT, DHT11);

void initSensors()
{
	dht.begin();
}

float readDHTTemp()
{
	float t = dht.readTemperature();
	if (isnan(t)) {
		return NAN;
	}
	return t;
}

float readDHTHum()
{
	float h = dht.readHumidity();
	if (isnan(h)) {
		return NAN;
	}
	return h;
}

float readNodeItem(uint8_t nodeIndex)
{
	switch (nodeIndex) {
		case 0:
			return readDHTTemp();
		case 1:
			return readDHTHum();
		default:
			return NAN;
	}
}
