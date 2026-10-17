/*
 * dht.cpp — DHT11/DHT22 no A2 da M360-LP (porta J2, pino 2)
 *
 * Pull-up de 10 kΩ para o VSW está na placa. O begin() da lib Adafruit põe o
 * pino em INPUT_PULLUP; com o VSW desligado isso é um nível alto fraco que
 * alimenta o sensor pelo diodo de proteção, então dhtPowerDown() devolve o
 * pino a INPUT puro.
 */

#include "nodeConfig.h"
#if LP_HAS_DHT

#include "lpDrivers.h"
#include <DHT.h>

#ifndef LP_DHT_TYPE
#  define LP_DHT_TYPE DHT11
#endif

static DHT dht(LP_PIN_DHT, LP_DHT_TYPE);

void dhtPowerUp()
{
	dht.begin();
}

void dhtPowerDown()
{
	pinMode(LP_PIN_DHT, INPUT);
}

float dhtReadTemp()
{
	float t = dht.readTemperature();
	return isnan(t) ? NAN : t;
}

float dhtReadHum()
{
	float h = dht.readHumidity();
	return isnan(h) ? NAN : h;
}

#endif // LP_HAS_DHT
