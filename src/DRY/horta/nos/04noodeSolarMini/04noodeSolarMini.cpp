/*
 * 04noodeSolarMini.cpp — Nó 4: Monitoramento Solar de Clima e Solo (DHT11 + DS18B20)
 *
 * Hardware: Arduino Pro Mini 5V/16MHz (env pro16MHzatmega328) + DHT11 + DS18B20 + nRF24L01+
 * Alimentação: Baixo Consumo (Bateria / Solar)
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== WARM-UP PÓS-BOOT =====
// Após o reboot, o nó transmite a cada 30 s por WARMUP_CYCLES ciclos (~5 min de
// relógio) para confirmar a reconexão com o gateway. Depois volta ao intervalo
// em minutos que begin() carregou da EEPROM (child 254) — o warm-up nunca
// sobrescreve o valor do operador. Ver API_REFERENCE.md §4.
//
// Conta-se ciclos, não millis(): o smartSleep() para o millis() durante o sono,
// então "5 min de millis()" seriam ~60 ciclos (~35 min de relógio a 30 s). E o
// millis() já corre enquanto _begin() espera o gateway, o que faria um warm-up
// por tempo nascer expirado em setup() numa queda geral de energia.
#define WARMUP_INTERVAL_MS 30000UL   // 30 s entre ciclos
#define WARMUP_CYCLES      10        // 10 ciclos × 30 s ≈ 5 min

static uint8_t warmupCycles = 0;     // ciclos concluídos; para em WARMUP_CYCLES
static bool    warmupDone   = false; // transiciona apenas uma vez por boot

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

	// Sem override de readBatteryVoltage(): a bateria entra direto no VCC, então o
	// padrão da lib (VCC medido contra o bandgap de 1,1 V) já é a tensão da bateria.
	// O antigo divisor em A0 era lido contra o próprio AVcc e travava em ~3,3 V.
} // namespace M360

// ===== MYSENSORS HOOKS =====

void before()
{
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation()
{
	node.begin(F("SolarMini"), F("2.1.0"));
}


void setup()
{
	node.onRead(readNodeItem);

	// Warm-up: 30 s por ciclo até WARMUP_CYCLES; a transição ocorre em loop().
	if (node.setIntervalMs(WARMUP_INTERVAL_MS)) {
		Serial.println(F("WARMUP:30s"));
	}
}

void loop()
{
	node.process(); // LOW_POWER: uma chamada = um ciclo (lê → envia → dorme)

	// Após WARMUP_CYCLES ciclos, encerra o override e volta ao intervalo da
	// EEPROM. setIntervalMs(0) não toca em _interval: se o backend já tiver
	// enviado um C_SET V_VAR1 durante o warm-up, handleMessage() zerou o
	// override e persistiu o valor do operador — o comando externo prevalece.
	if (!warmupDone && ++warmupCycles >= WARMUP_CYCLES) {
		warmupDone = true;
		if (node.getIntervalMs() > 0) {
			node.setIntervalMs(0); // a lib ecoa o intervalo vigente no child 254
			Serial.println(F("WARMUP:done->eeprom"));
		} else {
			Serial.println(F("WARMUP:done->backend"));
		}
	}
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
