/*
 * 01nodeSolo3D.cpp — Nó 01: Monitoramento 3D de Solo (Canteiros A e B)
 *
 * Hardware: Arduino Nano (5V) + CD74HC4067 (MUX Analógico) + 18 Sensores Resistivos
 *           + nRF24L01+ (CE=D9, CSN=D10)
 *
 * Estratégia de Mitigação de Eletrólise:
 * O pino D3 alimenta a barra de resistores de pull-up. Ele fica HIGH apenas durante
 * o momento da leitura (varredura) e depois volta a LOW.
 *
 * Modo de Operação (M360_ALWAYS_ON):
 * O nó ficará constantemente acordado (alimentação fixa) ouvindo a rede.
 * Para funcionar "sob demanda" (sem intervalo), basta que o gateway configure
 * o intervalo do nó (V_VAR1) para 65535 minutos, ou que a API requisite
 * a atualização imediata via comando V_CUSTOM="FORCE".
 */

#include <Arduino.h>

// Configurações do MySensors devem vir ANTES do include
#define MY_NODE_ID 1

#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// Definição dos 9 sensores resistivos (9 no Canteiro A)
// Canteiro A usa os MUX canais 0 a 8.
M360::M360ItemDef nodeItems[] = {
	// Canteiro A - 9 sensores (Child IDs 0 a 8)
	{0,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_10cm", false, 0},
	{1,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_20cm", false, 0},
	{2,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_30cm", false, 0},
	{3,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_10cm", false, 0},
	{4,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_20cm", false, 0},
	{5,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_30cm", false, 0},
	{6,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_10cm", false, 0},
	{7,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_20cm", false, 0},
	{8,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_30cm", false, 0}
};

const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

// Variáveis exigidas pelo M360-DRY
MyMessage messages[numItems + 2];
float lastValues[numItems];
uint8_t nNoUpdates[numItems];

// Instancia o nó no modo ALWAYS_ON. 
// O rádio não dorme, e pode ouvir pedidos assíncronos do backend a qualquer momento.
M360::M360Node node(nodeItems, numItems, messages, lastValues, nNoUpdates, M360::M360_ALWAYS_ON);

void before() {
	// Inicializa os pinos de hardware (MUX, Pull-up, Pinos nativos)
	initSensors();
}

void setup() {
	// Registra o callback de leitura analógica
	node.onRead(readNodeItem);
	
	// Inicia a apresentação na rede MySensors
	node.begin("01nodeSolo3D", "1.0");
}

void loop() {
	// Processa temporizadores nativos do M360Node
	node.process();
}

void receive(const MyMessage& msg) {
	// Encaminha mensagens recebidas para a engine do M360Node
	node.handleMessage(msg);
}
