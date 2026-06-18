/*
 * NodeTemplate.cpp — Modelo Arquitetural para Nós M360-DRY
 * 
 * Siga estas diretrizes para manter a consistência do projeto:
 * 1. O Sketch (.cpp): Puramente DECLARATIVO (IDs, labels, pino VCC).
 * 2. O Driver (sensorDrivers.h/.cpp): Implementação FÍSICA (leitura do sensor, calibração).
 * 
 * Passos para criar um novo nó:
 *   1. Copie este arquivo e os sensorDriversTemplate.h e sensorDriversTemplate.cpp para a pasta src do programa 
 *   2. Use #include <M360.h> (cabeçalho centralizador)
 *   3. Preencha NODE_ITEMS[] com as configurações MySensors
 *   4. Implemente os callbacks de energia (powerUp/Down) se usar o Pino 4
 *   5. Delegue a leitura física do sensor para o seu sensorDrivers.h
 */

// ===== CONFIGURAÇÃO MYSENSORS =====
#define MY_RADIO_RF24
// #define MY_RF24_CE_PIN  6
// #define MY_RF24_CSN_PIN 4
// #define MY_NODE_ID      10

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
// Perfil: M360::M360_LOW_POWER (bateria) ou M360::M360_ALWAYS_ON (12V/debug)
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates,
                           M360::M360_LOW_POWER);

// ===== IMPLEMENTAÇÃO DA LEITURA =====

float readItem(uint8_t nodeIndex)
{
	switch (nodeIndex) {
		case 0:
			return 25.0f; // TODO: ler sensor real
		case 1:
			return 60.0f; // TODO: ler sensor real
		default:
			return M360_SENSOR_INVALID;
	}
}

// ===== IMPLEMENTAÇÃO DA ESCRITA (atuadores) =====

void writeItem(uint8_t nodeIndex, bool state)
{
	// TODO: controlar o pino do atuador
	// if (nodeIndex == 2) digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

// ===== CALLBACKS DE ENERGIA (opcional) =====

namespace M360
{
	void powerUp()
	{
		// TODO: ligar periféricos (sensores com VCC chaveada, etc.)
	}

	void powerDown()
	{
		// TODO: desligar periféricos antes do sleep
	}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void presentation()
{
	node.begin("M360 Template", "1.0");
}

void setup()
{
	Serial.begin(MY_BAUD_RATE);
	node.setupPins();
	node.onRead(readItem);
	node.onWrite(writeItem);
}

void loop()
{
	node.process();
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
