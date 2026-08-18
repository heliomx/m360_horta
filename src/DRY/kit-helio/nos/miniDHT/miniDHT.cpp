/*
 * miniDHT.cpp — Nó 11 (Kit Hélio): Sensor de Temperatura e Umidade do Ar (DHT11)
 *
 * Sub-projeto: Kit Hélio
 * Hardware: Arduino Pro Mini 5V/16MHz (pro16MHzatmega328) + DHT11 no pino D4
 * Alimentação: Fonte fixa 5V DC
 * Perfil: M360_ALWAYS_ON
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// Colunas: childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_ID_DHT_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, PIN_DHT, 1, 3, "Temperatura Ar",   false, 0 },
	{ CHILD_ID_DHT_HUM,  M360::M360_SENSOR, S_HUM,  V_HUM,  PIN_DHT, 1, 3, "Umidade Relativa", false, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS ESTÁTICOS =====
// messages: +3 OBRIGATÓRIO — canais reservados de intervalo (254), bateria (255)
// e debug remoto (253). Com +2, M360Node::begin() escreve fora do array.
static MyMessage messages[NODE_ITEMS_COUNT + 3];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

// ===== MYSENSORS HOOKS =====

void before()
{
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation()
{
	node.begin("miniDHT", "1.0");
}

void setup()
{
	node.onRead(readNodeItem);
	node.setupPins();
}

void loop()
{
	node.process();

#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
	static unsigned long lastRssiLog = 0;
	if (millis() - lastRssiLog >= MY_RSSI_LOG_INTERVAL) {
		lastRssiLog = millis();
		Serial.print(F("[RSSI] "));
		Serial.println(transportGetSignalReport(SR_RX_RSSI));
	}
#endif
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
