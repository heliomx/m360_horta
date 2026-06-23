/*
 * 01nodeSolo3dMini.cpp — Versão de Baixo Consumo do Nó 01: Monitoramento 3D de
 * Solo
 *
 * Hardware: Arduino Pro Mini (3.3V / 8MHz) + CD74HC4067 (MUX Analógico) + 18
 * Sensores Resistivos
 *           + nRF24L01+ (CE=D9, CSN=D10)
 * Alimentação: Bateria Li-ion / LiFePO4 + Placa Solar (Carregador TP4056 ou
 * similar)
 *
 * Estratégia de Economia de Energia (LOW_POWER):
 * Para operação prolongada em bateria e placa solar, este nó opera no perfil
 * M360_LOW_POWER, entrando em smartSleep no restante do tempo. O pino D3
 * (PIN_POWER_SENSORS) é ativado antes de realizar a varredura e desativado
 * imediatamente antes de dormir, minimizando o consumo estático de corrente e
 * mitigando a eletrólise dos eletrodos no solo.
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// MY_NODE_ID definido via platformio.ini [env:node_01_...] — não redefinir aqui
// (redefinir aqui prevaleceria sobre o .ini e geraria confusão de manutenção)

// Definição dos 9 sensores resistivos (9 no Canteiro A)
// Canteiro A usa os MUX canais 0 a 8.
// TODO: adicionar canais 9-17 (canteiros B e C) quando eletrodos forem
// instalados.
static const M360::M360ItemDef nodeItems[] = {
    // Canteiro A - 9 sensores (Child IDs 0 a 8)
    {0, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_10cm", false,
     0},
    {1, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_20cm", false,
     0},
    {2, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_30cm", false,
     0},
    {3, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_10cm", false,
     0},
    {4, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_20cm", false,
     0},
    {5, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_30cm", false,
     0},
    {6, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_10cm", false,
     0},
    {7, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_20cm", false,
     0},
    {8, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_30cm", false,
     0},
    {9, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_10cm", false,
     0},
    {10, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_20cm", false,
     0},
    {11, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_30cm", false,
     0},
    {12, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_10cm", false,
     0},
    {13, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_20cm", false,
     0},
    {14, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_30cm", false,
     0},
    {15, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_10cm", false,
     0},
    {16, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_20cm", false,
     0},
    {17, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_30cm", false,
     0}};

static const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

static MyMessage messages[numItems + 2];
static float lastValues[numItems];
static uint8_t nNoUpdates[numItems];

static M360::M360Node node(nodeItems, numItems, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

void before() {
  Serial.begin(MY_BAUD_RATE);
  initSensors();
}

// Hooks do M360-DRY para acordar e dormir
namespace M360 {
void powerUp() { powerUpSensors(); }
void powerDown() { powerDownSensors(); }
} // namespace M360

void presentation() { node.begin("01nodeSolo3dMini", "1.0"); }

void setup() {
  // Registra o callback de leitura analógica
  node.onRead(readNodeItem);

  // Inicia a apresentação na rede MySensors e carrega o intervalo da EEPROM
}

void loop() {
  // Processa temporizadores e ciclo de dormir/acordar/ler/enviar
  node.process();
}

void receive(const MyMessage &msg) {
  // Trata comandos de alteração de intervalo e force update
  node.handleMessage(msg);
}
