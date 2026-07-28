/*
 * sensorDrivers.cpp — Implementação do driver físico para 04noodeSolarMini (Nó 4)
 *
 * Garante a estabilização do DHT11 durante o ciclo de power-up em baixo consumo.
 */

#include "sensorDrivers.h"
#include <M360.h> // fornece acesso a wait() da MySensors
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ===== OBJETOS DE SENSORES =====
static DHT dht(PIN_DHT, DHT11);
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

// ===== API PÚBLICA =====

void initSensors()
{
	// Configura o pino de controle de VCC como saída e garante que inicie desligado
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW);

	// Inicializa os barramentos de sensores
	dht.begin();
	sensors.begin();
}

void powerUpSensors()
{
	// Liga a alimentação dos sensores (DHT11 + DS18B20)
	digitalWrite(PIN_POWER_SENSORS, HIGH);

	// Reinicializa o barramento OneWire e solicita conversão de temperatura
	sensors.begin();
	sensors.requestTemperatures();

	// O DHT11 precisa de 1 a 1.5s para estabilizar e o DS18B20 necessita
	// de ~750ms para a conversão de 12 bits. Usamos wait(1500) para cobrir
	// ambos simultaneamente enquanto mantemos o stack do rádio ativo.
	wait(1500);
}

void powerDownSensors()
{
	// Desliga a alimentação dos sensores para economizar bateria
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

float readNodeItem(uint8_t nodeIndex)
{
	switch (nodeIndex) {
		case CHILD_ID_TEMP: {
			float t = dht.readTemperature();
			return isnan(t) ? NAN : t;
		}
		case CHILD_ID_HUM: {
			float h = dht.readHumidity();
			return isnan(h) ? NAN : h;
		}
		case CHILD_ID_SOIL_TEMP: {
			float tempC = sensors.getTempCByIndex(0);
			if (tempC == DEVICE_DISCONNECTED_C || tempC < -20.0f || tempC > 85.0f) {
				return NAN;
			}
			return tempC;
		}
		default:
			return NAN;
	}
}
