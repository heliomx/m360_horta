/*
 * 80nodeAqua.cpp — Nó 80: Monitoramento da Caixa de Água, pH, EC e Vazão
 *
 * Hardware: Arduino Nano + nRF24L01+ + HC-SR04 + pH-4502C + EC + YF-S201
 * Alimentação: 12V contínuo (ALWAYS_ON)
 * Gestão de VCC: Pino D3 chaveia a energia dos sensores
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== CHILD IDs =====
#define CHILD_ID_LEVEL      0
#define CHILD_ID_PH         1
#define CHILD_ID_EC         2
#define CHILD_ID_FLOW       3
#define CHILD_ID_WATER_TEMP 4

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_ID_LEVEL,      M360::M360_SENSOR, S_MOISTURE,      V_LEVEL, -1, 1, 3, "Nivel Caixa", false, 0 },
	{ CHILD_ID_PH,         M360::M360_SENSOR, S_WATER_QUALITY, V_PH,    -1, 1, 3, "pH",          false, 0 },
	{ CHILD_ID_EC,         M360::M360_SENSOR, S_WATER_QUALITY, V_EC,    -1, 1, 3, "EC",          false, 0 },
	{ CHILD_ID_FLOW,       M360::M360_SENSOR, S_WATER,         V_FLOW,  -1, 1, 1, "Vazao",       false, 0 },
	{ CHILD_ID_WATER_TEMP, M360::M360_SENSOR, S_TEMP,          V_TEMP,  -1, 1, 3, "Temp Agua",   false, 0 }
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocados pelo nó, gerenciados pelo M360Node) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates,
                           M360::M360_ALWAYS_ON);

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
	initSensors();
	// Sendo ALWAYS_ON, deixamos os sensores ligados permanentemente por padrão
	powerUpSensors();
}

void presentation()
{
	node.begin("80nodeAqua", "1.0");
}

void setup()
{
	node.onRead(readNodeItem);
	node.onWrite(writeNodeItem);
}

void loop()
{
	node.process();
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
