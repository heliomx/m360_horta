/*
 * 04noodeSolarMini.cpp — Nó 4: Monitoramento Solar de Clima (DHT11)
 *
 * Hardware: Arduino Pro Mini 3.3V/8MHz + DHT11 + nRF24L01+
 * Alimentação: Baixo Consumo (Bateria / Solar)
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_ID_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, "Temperatura", false, 0 },
	{ CHILD_ID_HUM,  M360::M360_SENSOR, S_HUM,  V_HUM,  -1, 1, 3, "Umidade",     false, 0 }
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS ESTÁTICOS (M360Node) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2]; // +2 para intervalo e bateria
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_LOW_POWER);

// ===== CALLBACKS DE ENERGIA =====
namespace M360
{
	void powerUp()
	{
		powerUpSensors();
	}

	void powerDown()
	{
		powerDownSensors();
	}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void before()
{
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation()
{
	node.begin("SolarMini", "1.0");
}

void setup()
{
	node.onRead(readNodeItem);
}

void loop()
{
	node.process();
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
