/*
 * lpNode.cpp — Firmware comum da placa M360-LP (Pro Mini 3,3 V / 8 MHz)
 *
 * Um binário por nó, um fonte para todos: o que muda entre nós entra por
 * config/<nó>/nodeConfig.h, escolhido pelo -I do env em platformio.ini.
 * Nada de hardware aqui — a camada física está em lpDrivers.cpp e drivers/.
 *
 * nodeConfig.h define:
 *   LP_NODE_NAME, LP_NODE_VERSION      nome e versão apresentados ao gateway
 *   LP_POWER_FIXED_5V | LP_POWER_BATTERY  perfil (ALWAYS_ON | LOW_POWER + bateria)
 *   LP_HAS_SOIL / LP_HAS_DHT / LP_HAS_ONEWIRE  drivers compilados
 *   rótulos PROGMEM e NODE_ITEMS[]     com o campo `pin` = código LP_SRC_* (lpPins.h)
 *
 * Reed (SW1, D3/INT1): aproximar o ímã pela parede da caixa força a leitura e o
 * envio de todos os children — conferência do instalador sem abrir a caixa nem
 * esperar o intervalo. Em LOW_POWER o ímã acorda o nó (setWakeInterrupt); em
 * ALWAYS_ON o pino é lido por polling com debounce.
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "lpPins.h"
#define LP_NODE_ITEMS_SCOPE 1   // libera o bloco NODE_ITEMS[] do nodeConfig.h só nesta TU
#include "nodeConfig.h"
#include "lpDrivers.h"

#if !defined(LP_POWER_FIXED_5V) && !defined(LP_POWER_BATTERY)
#  error "nodeConfig.h: defina LP_POWER_FIXED_5V ou LP_POWER_BATTERY"
#endif
#if defined(LP_POWER_FIXED_5V) && defined(LP_POWER_BATTERY)
#  error "nodeConfig.h: LP_POWER_FIXED_5V e LP_POWER_BATTERY sao exclusivos"
#endif

static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// Sem messages[]: a M360-DRY 2.0.0 monta cada envio num único MyMessage interno.
static float   lastValues[NODE_ITEMS_COUNT];
static uint8_t nNoUpdates[NODE_ITEMS_COUNT];

#if LP_POWER_BATTERY
#  define LP_PROFILE M360::M360_LOW_POWER
#else
#  define LP_PROFILE M360::M360_ALWAYS_ON
#endif

static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, lastValues, nNoUpdates, LP_PROFILE);

// ===== LEITURA =====
// Convenção B (R5): o driver recebe o código do campo `pin`, não o índice. O
// VSW sobe no primeiro item e desce no último, o que cobre o perfil ALWAYS_ON
// (que nunca chama powerUp/powerDown) e é inócuo no LOW_POWER (idempotente).
static float readItem(uint8_t nodeIndex)
{
	if (nodeIndex == 0) {
		powerUpSensors();
	}
	float v = lpReadSource(NODE_ITEMS[nodeIndex].pin);
	if (nodeIndex == NODE_ITEMS_COUNT - 1) {
		powerDownSensors();
	}
	return v;
}

// ===== ENERGIA (perfil LOW_POWER chama antes e depois do ciclo) =====
namespace M360
{
	void powerUp()   { powerUpSensors();   }
	void powerDown() { powerDownSensors(); }
}

#if LP_POWER_BATTERY
// ===== WARM-UP PÓS-BOOT (R13) =====
#define WARMUP_INTERVAL_MS 30000UL
#define WARMUP_CYCLES      10
static uint8_t warmupCycles = 0;
static bool    warmupDone   = false;

// Corte por bateria baixa (§4.1 / §9): abaixo do limiar o nó não transmite —
// o pico de TX com a célula em ~2,85 V leva o VCC ao BOD e o nó entra em laço
// de reset. Dorme sem rádio pelo intervalo e tenta de novo.
#ifndef LP_VBAT_CUTOFF
#  define LP_VBAT_CUTOFF 3.0f
#endif
#endif

// ===== MYSENSORS HOOKS =====

void before()
{
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation()
{
	node.begin(F(LP_NODE_NAME), F(LP_NODE_VERSION));
}

void setup()
{
	node.onRead(readItem);
	// node.setupPins() omitido (R7): `pin` carrega códigos LP_SRC_*, não pinos.
	// Reed: borda de descida no D3 (contato fechando) acorda o nó em LOW_POWER.
	node.setWakeInterrupt(digitalPinToInterrupt(LP_PIN_REED), FALLING);
#if LP_POWER_BATTERY
	if (node.setIntervalMs(WARMUP_INTERVAL_MS)) {
		Serial.println(F("WARMUP:30s"));
	}
#endif
}

void loop()
{
#if LP_POWER_BATTERY
	float vbat = M360::readBatteryVoltage();
	if (vbat > 0.0f && vbat < LP_VBAT_CUTOFF) {
		Serial.print(F("BATT:LOW "));
		Serial.println(vbat, 2);
		sleep((uint32_t)node.getInterval() * 60000UL); // sem rádio, sem TX
		return;
	}
#endif

	node.process();

#if !LP_POWER_BATTERY
	// ALWAYS_ON não dorme: polling do reed. process() termina em wait(50), então
	// duas leituras LOW seguidas (~50 ms) filtram o repique do contato. Dispara uma
	// vez por aproximação; precisa afastar o ímã para disparar de novo.
	static uint8_t reedLow = 0;
	if (digitalRead(LP_PIN_REED) == LOW) {
		if (reedLow < 2 && ++reedLow == 2) {
			Serial.println(F("REED"));
			node.forceUpdate();
		}
	} else {
		reedLow = 0;
	}
#endif

#if LP_POWER_BATTERY
	if (!warmupDone && ++warmupCycles >= WARMUP_CYCLES) {
		warmupDone = true;
		if (node.getIntervalMs() > 0) {
			node.setIntervalMs(0);
			Serial.println(F("WARMUP:done->eeprom"));
		} else {
			Serial.println(F("WARMUP:done->backend"));
		}
	}
#endif
}

void receive(const MyMessage& msg)
{
	node.handleMessage(msg);
}
