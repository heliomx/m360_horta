/*
 * nodeZTS_UmidadeHall.cpp — Versão LibDRY do Nó ZTS + Hall
 * 
 * Este nó monitora: 
 * - Sensor ZTS-3002 (Umidade, Temp, CE, pH, N, P, K) via Modbus/RS485
 * - Sensor de Umidade Hall e Luminosidade (LDR)
 * - Controle de Válvula Solenoide (Relé)
 * 
 * Mantém paridade com sensorDrivers.cpp e utiliza M360.h.
 */

// ===== CONFIGURAÇÃO MYSENSORS =====

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId               | kind             | presentType      | valueType      | pin | intMin | smp | label           | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_ID_MOISTURE,     M360::M360_SENSOR,   S_MOISTURE,    V_LEVEL,       -1, 1, 1, "Umidade ZTS",   false, 0 },
	{ CHILD_ID_TEMP,         M360::M360_SENSOR,   S_TEMP,        V_TEMP,        -1, 1, 1, "Temp ZTS",      false, 0 },
	{ CHILD_ID_CONDUCTIVITY, M360::M360_SENSOR,   S_CUSTOM,      V_VAR1,        -1, 1, 1, "Cond Solo",     false, 0 },
	{ CHILD_ID_PH,           M360::M360_SENSOR,   S_CUSTOM,      V_VAR2,        -1, 1, 1, "pH Solo",       false, 0 },
	{ CHILD_ID_NITROGEN,     M360::M360_SENSOR,   S_CUSTOM,      V_VAR3,        -1, 1, 1, "N Solo",        false, 0 },
	{ CHILD_ID_PHOSPHORUS,   M360::M360_SENSOR,   S_CUSTOM,      V_VAR4,        -1, 1, 1, "P Solo",        false, 0 },
	{ CHILD_ID_POTASSIUM,    M360::M360_SENSOR,   S_CUSTOM,      V_VAR5,        -1, 1, 1, "K Solo",        false, 0 },
	{ CHILD_ID_SELENOIDE,    M360::M360_ACTUATOR, S_BINARY,      V_STATUS,       5, 0, 0, "Válvula",       false, 0 },
	{ CHILD_ID_UMIDADE_HALL, M360::M360_SENSOR,   S_HUM,         V_HUM,         A0, 1, 1, "Umidade Hall",  false, 0 },
	{ CHILD_ID_LDR,          M360::M360_SENSOR,   S_LIGHT_LEVEL, V_LIGHT_LEVEL,  A1, 1, 1, "Luminosidade", false, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocados pelo nó, gerenciados pelo M360Node) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
// Perfil M360_PASSIVE: o nó dorme e só lê quando o gateway enviar V_CUSTOM FORCE_UPDATE.
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates,
                           M360::M360_PASSIVE);

// ===== IMPLEMENTAÇÃO DA LEITURA =====

float readItem(uint8_t nodeIndex)
{
	// sensorDrivers.cpp::readNodeItem() gerencia a lógica de sensor ZTS vs sensores locais.
	return readNodeItem(nodeIndex);
}

// ===== IMPLEMENTAÇÃO DA ESCRITA (atuadores) =====

void writeItem(uint8_t nodeIndex, bool state)
{
	if (nodeIndex < NODE_ITEMS_COUNT) {
		writeNodeItem(nodeIndex, state);
	}
}

// ===== CALLBACKS DE ENERGIA (opcional) =====

namespace M360
{
	void powerUp()
	{
		// Liga o relé que alimenta o RS485 e o sensor ZTS
		powerUpPeripherals();
		// P10: wait() processa mensagens de rádio enquanto aguarda estabilização
		// do sensor Modbus (ao contrário de delay() que bloqueia a janela de escuta)
		wait(500);
	}

	void powerDown()
	{
		// Desliga periféricos para economizar bateria durante o sono
		powerDownPeripherals();
	}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void presentation()
{
	node.begin("M360 ZTS+UmidadeHall LibDRY PASSIVE", "2.3");
}

void before()
{
	Serial.begin(MY_BAUD_RATE);
	initDrivers();
}

void setup()
{
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
