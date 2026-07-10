/*
 * sensorDrivers.cpp — Implementação do driver físico para 04noodeSolarMini (Nó 4)
 *
 * Garante a estabilização do DHT11 durante o ciclo de power-up em baixo consumo.
 */

#include "sensorDrivers.h"
#include <M360.h> // fornece acesso a wait() da MySensors
#include <DHT.h>

// ===== OBJETO DHT11 =====
static DHT dht(PIN_DHT, DHT11);

// ===== API PÚBLICA =====

void initSensors()
{
	// Configura o pino de controle de VCC como saída e garante que inicie desligado
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW);

	// Inicializa o barramento / configuração da biblioteca DHT
	dht.begin();
}

void powerUpSensors()
{
	// Liga a alimentação dos sensores
	digitalWrite(PIN_POWER_SENSORS, HIGH);

	// O DHT11 precisa de pelo menos 1 a 1.5 segundos para estabilização
	// após ser alimentado. Usamos wait() em vez de delay() para continuar
	// processando o stack de rádio MySensors em background.
	wait(1500);
}

void powerDownSensors()
{
	// Desliga a alimentação do sensor para economizar bateria
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
		default:
			return NAN;
	}
}
