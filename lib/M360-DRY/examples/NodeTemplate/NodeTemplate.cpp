/*
 * NodeTemplate.cpp — Modelo Arquitetural para Nós M360-DRY
 * 
 * Siga estas diretrizes para manter a consistência do projeto:
 * 1. O Sketch (.cpp): Puramente DECLARATIVO (IDs, labels, perfis de energia).
 *    NOTA: As configurações MY_* de rádio e Node ID devem vir do platformio.ini.
 * 2. O Driver (sensorDrivers.h/.cpp): Implementação FÍSICA (leitura do sensor, calibração).
 * 
 * Passos para criar um novo nó:
 *   1. Crie a pasta do nó em src/DRY/nos/
 *   2. Copie este arquivo e os sensorDrivers.h e sensorDrivers.cpp para a pasta de destino
 *   3. Use #include <M360.h> (cabeçalho centralizador)
 *   4. Preencha NODE_ITEMS[] com as configurações MySensors
 *   5. Delegue a leitura física e escrita do sensor para o sensorDrivers.h
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== CHILD IDs =====
#define CHILD_TEMP   0
#define CHILD_HUM    1
// #define CHILD_RELAY  2

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind        | presentType | valueType | pin | intervalMin | samples | label        | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, "Temperatura", false, 0 },
	{ CHILD_HUM,  M360::M360_SENSOR, S_HUM,  V_HUM, -1, 1, 3, "Umidade",     false, 0 },
	// { CHILD_RELAY, M360::M360_ACTUATOR, S_BINARY, V_STATUS, 5, 0, 0, "Rele", true, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocados pelo nó, gerenciados pelo M360Node) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
// Perfil: M360::M360_LOW_POWER (bateria) ou M360::M360_ALWAYS_ON (12V/debug) ou M360::M360_PASSIVE (reativo)
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates,
                           M360::M360_LOW_POWER);

// ===== CALLBACKS DE ENERGIA (opcional) =====

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
}

void presentation()
{
	node.begin("M360 Template", "1.0");
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
