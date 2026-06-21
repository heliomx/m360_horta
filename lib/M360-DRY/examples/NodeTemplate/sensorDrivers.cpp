#include "sensorDrivers.h"

// Exemplo de variáveis globais locais do driver
static bool relayState = false;

void initSensors()
{
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW); // Sensores desligados em repouso
	
	pinMode(PIN_RELAY, OUTPUT);
	digitalWrite(PIN_RELAY, LOW);
}

void powerUpSensors()
{
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	wait(2000); // Aguarda estabilização de sensores (ex: DHT22); wait() processa rádio, delay() não

	// Inicializar bibliotecas de sensores aqui
	// Ex: dht.begin();
}

void powerDownSensors()
{
	digitalWrite(PIN_POWER_SENSORS, LOW);
	
	// Aterrar pinos de sinal para evitar vazamento elétrico (parasitic powering)
	pinMode(PIN_TEMP_HUM, OUTPUT);
	digitalWrite(PIN_TEMP_HUM, LOW);
}

float readNodeItem(uint8_t itemIndex)
{
	// Mapeado baseado no NODE_ITEMS[] em NodeTemplate.cpp:
	// Index 0: Temperatura
	// Index 1: Umidade
	
	switch (itemIndex) {
		case 0:
			// Retornar leitura do sensor real
			// Ex: return dht.readTemperature();
			return 25.5f;
			
		case 1:
			// Retornar leitura do sensor real
			// Ex: return dht.readHumidity();
			return 60.0f;
			
		default:
			return NAN;
	}
}

void writeNodeItem(uint8_t nodeIndex, bool state)
{
	// nodeIndex é a posição no NODE_ITEMS[], NÃO o childId MySensors.
	// Ex: se CHILD_RELAY for o item de índice 2 em NODE_ITEMS[], o motor chama writeNodeItem(2, …)
	// independentemente do valor de CHILD_RELAY.

	if (nodeIndex == 2) {
		relayState = state;
		digitalWrite(PIN_RELAY, state ? HIGH : LOW);
	}
}
