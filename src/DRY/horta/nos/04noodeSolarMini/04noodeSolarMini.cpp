/*
 * 04noodeSolarMini.cpp — Nó 4: Monitoramento Solar de Clima e Solo (DHT11 + DS18B20)
 *
 * Hardware: Arduino Pro Mini 3.3V/8MHz + DHT11 + DS18B20 + nRF24L01+
 * Alimentação: Baixo Consumo (Bateria / Solar)
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== RÓTULOS (na Flash) =====
// M360_LABEL() só aceita arrays PROGMEM. O texto é o que o Node-RED exibe —
// ver inventario.md §6.
static const char L_TEMP[]      PROGMEM = "Temperatura Ar";
static const char L_HUM[]       PROGMEM = "Umidade Ar";
static const char L_SOIL_TEMP[] PROGMEM = "Temperatura Solo";

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_ID_TEMP,      M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, M360_LABEL(L_TEMP),      false, 0 },
	{ CHILD_ID_HUM,       M360::M360_SENSOR, S_HUM,  V_HUM,  -1, 1, 3, M360_LABEL(L_HUM),       false, 0 },
	{ CHILD_ID_SOIL_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, M360_LABEL(L_SOIL_TEMP), false, 0 }
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS ESTÁTICOS (M360Node) =====
// Sem messages[]: a M360-DRY 2.0.0 monta cada envio num único MyMessage interno.
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, lastValues, nNoUpdates,
                           M360::M360_LOW_POWER);

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

	float readBatteryVoltage()
	{
		// Lê a tensão no divisor de 100k/100k conectado ao pino A0.
		// O divisor por 2 divide a tensão da bateria pela metade (max 4.2V -> 2.1V).
		// Usando a referência AVcc = 3.3V e 1023 passos: Vbat = (raw * 3.3 / 1023) * 2
		int raw = analogRead(PIN_BATTERY_ADC);
		return (float)raw * (6.6f / 1023.0f);
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
	node.begin(F("SolarMini"), F("2.0.0"));
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
