/*
 * 04nodeClima.cpp — Nó 04: Clima Geral da Estufa
 *
 * Hardware: Arduino Pro Mini (3.3V / 8MHz) + DHT22 + LDR + nRF24L01+
 * Alimentação: Painel Solar + Bateria 18650
 *
 * Estratégia de Economia (LOW_POWER):
 * Para que o painel solar seja autossuficiente, este nó fica 99% do tempo
 * em 'smartSleep'. O pino D3 funciona como interruptor de VCC para o DHT22 e LDR.
 * Eles só são energizados durante a varredura e desligados imediatamente antes do sleep.
 * O DHT22 exige no mínimo 2 segundos de estabilização elétrica antes da leitura.
 */

#include <Arduino.h>

// Configurações do MySensors devem vir ANTES do include
#define MY_NODE_ID 4

#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// Definição dos sensores: Temperatura (0), Umidade (1) e Luminosidade LDR (2)
M360::M360ItemDef nodeItems[] = {
	{0, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 0, 1, "Temp_Geral", false, 0},
	{1, M360::M360_SENSOR, S_HUM,  V_HUM,  -1, 0, 1, "Umid_Geral", false, 0},
	{2, M360::M360_SENSOR, S_LIGHT_LEVEL, V_LEVEL, -1, 0, 1, "Luz_Geral", false, 0}
};

const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

// Variáveis exigidas pelo M360-DRY
MyMessage messages[numItems + 2];
float lastValues[numItems];
uint8_t nNoUpdates[numItems];

// Instancia o nó no modo LOW_POWER (usará o smartSleep da rede)
M360::M360Node node(nodeItems, numItems, messages, lastValues, nNoUpdates, M360::M360_LOW_POWER);

void before() {
	// Inicializa os pinos de hardware em repouso
	initSensors();
}

// Hooks do M360-DRY para acordar e dormir
namespace M360 {
	void powerUp() {
		powerUpSensors();
	}
	void powerDown() {
		powerDownSensors();
	}
}

void presentation() {
	node.begin("04nodeClima", "1.0");
}

void setup() {
	// Registra o callback de leitura
	node.onRead(readNodeItem);
	
	// Inicia a apresentação na rede MySensors
	// Nota: Para forçar 15 minutos obrigatórios, o Backend deve enviar 
	// uma mensagem V_VAR1=15 para o child 255 deste nó.
}

void loop() {
	// O M360Node.process() chamará automaticamente: 
	// powerUp() -> read() -> send() -> powerDown() -> smartSleep()
	node.process();
}

void receive(const MyMessage& msg) {
	node.handleMessage(msg);
}
