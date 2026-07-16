/*
 * nodeReles.cpp — Nó 99: Central de Atuação da Estufa (M360-DRY)
 *
 * Hardware: Arduino Nano (5V) + CD74HC4067 (MUX 16ch) + 16 Relés 10A
 * (optoacoplador)
 *           + nRF24L01+ (adaptador socket regulado 3.3V) — CE=D9, CSN=D10
 *
 * Arquitetura de Atuação:
 *
 *   Canais via MUX CD74HC4067 (SIG=D8 | S0-S3=D4-D7 | EN=GND):
 *   ┌─────────┬───────────────────────────────────────────────┐
 *   │ Canal 0 │ Solenóide Irrigação Gotejamento — Canteiro A  │
 *   │ Canal 1 │ Solenóide Irrigação Gotejamento — Canteiro B  │
 *   │ Canal 2 │ Solenóide Irrigação Gotejamento — Canteiro C  │
 *   │ Canal 3 │ Peristáltica Suplemento A                     │
 *   │ Canal 4 │ Peristáltica Suplemento B                     │
 *   │ Canal 5 │ Peristáltica pH+                              │
 *   │ Canal 6 │ Peristáltica pH-                              │
 *   │ Ch 7-15 │ Reservados (climatização / iluminação)        │
 *   └─────────┴───────────────────────────────────────────────┘
 *   RESTRIÇÃO: Apenas 1 canal MUX ativo por vez (proteção de fonte).
 *
 *   Pinos Nativos — Operação Concorrente Independente:
 *   ┌──────┬──────────────────────────────────────────────────┐
 *   │  A0  │ Bomba Circulação Principal — Hidroponia NFT      │
 *   │  A1  │ Bomba Oxigenação — Hidroponia NFT                │
 *   │  D2  │ Sensor DHT11 DATA                                │
 *   └──────┴──────────────────────────────────────────────────┘
 *
 * Macros MY_* definidas no platformio.ini [env:node_99_reles_nano]
 */

// ===== CONFIGURAÇÃO MYSENSORS =====
// Macros MY_* definidas no platformio.ini [env:nano_99reles]
// MySensors.h deve vir antes de M360.h para garantir sei() antes dos
// construtores globais

#include "sensorDrivers.h"
#include <Arduino.h>
#include <M360.h>
#include <MySensors.h>


#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
static unsigned long lastRssiLog = 0;
#endif

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// Colunas: childId | kind | presentType | valueType | pin | intMin | smp |
// label | wakeOnRadio | flags
//
// Pinos MUX usam encoding virtual: pin = MUX_CHANNEL_OFFSET + canal
// (ex.: canal 4 → pin 104). Ver sensorDrivers.h para detalhes.
static const M360::M360ItemDef NODE_ITEMS[] = {
    // --- Canais MUX (concorrência restrita: 1 ativo por vez) ---
    {CHILD_ID_SOL_A, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 0, 0, 1, "Sol.CanteiroA", false, 0},
    {CHILD_ID_SOL_B, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 1, 0, 1, "Sol.CanteiroB", false, 0},
    {CHILD_ID_SOL_C, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 2, 0, 1, "Sol.CanteiroN", false, 0},
    {CHILD_ID_PERIST_A, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 3, 0, 1, "Perist.SuplA", false, 0},
    {CHILD_ID_PERIST_B, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 4, 0, 1, "Perist.SuplB", false, 0},
    {CHILD_ID_PH_PLUS, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 5, 0, 1, "Perist.pH+", false, 0},
    {CHILD_ID_PH_MINUS, M360::M360_ACTUATOR, S_BINARY, V_STATUS,
     MUX_CHANNEL_OFFSET + 6, 0, 1, "Perist.pH-", false, 0},
    // --- Pinos Nativos (concorrência livre com MUX e entre si) ---
    {CHILD_ID_NFT_PUMP, M360::M360_ACTUATOR, S_BINARY, V_STATUS, PIN_NFT_PUMP,
     0, 1, "BombaNFT", false, 0},
    {CHILD_ID_NFT_OXI, M360::M360_ACTUATOR, S_BINARY, V_STATUS, PIN_NFT_OXI, 0,
     1, "BombaOxi", false, 0},
    // --- Sensores Nativos (DHT11 - Concorrência livre) ---
    {CHILD_ID_DHT_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, PIN_DHT, 0, 1,
     "Temperatura do Ar", false, 0},
    {CHILD_ID_DHT_HUM, M360::M360_SENSOR, S_HUM, V_HUM, PIN_DHT, 0, 1,
     "Umidade do Ar", false, 0},
    // --- Sensores de Vazão Virtuais ---
    {CHILD_ID_FLOW_A, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao.CanteiroA", false, 0},
    {CHILD_ID_FLOW_B, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao.CanteiroB", false, 0},
    {CHILD_ID_FLOW_N, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao.CanteiroN", false, 0},
};
static const uint8_t NODE_ITEMS_COUNT =
    sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// Índices para reporte rápido das vazões de cada canteiro
static const uint8_t IDX_FLOW_A = 11;
static const uint8_t IDX_FLOW_B = 12;
static const uint8_t IDX_FLOW_N = 13;

// ===== BUFFERS =====
static MyMessage
    messages[NODE_ITEMS_COUNT +
             3]; // +1 Intervalo (ID 254) +1 Bateria (ID 255) +1 Debug (ID 253)
static float lastValues[NODE_ITEMS_COUNT];
static uint8_t nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
// MY_REPEATER_FEATURE (via build_flags) ativa encaminhamento de mensagens de
// outros nós. Use: pio run -e nano_99reles_rep -t upload
#ifdef MY_REPEATER_FEATURE
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_REPEATER);
#else
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);
#endif

// ===== CALLBACKS =====

static float readItem(uint8_t index) {
  if (NODE_ITEMS[index].childId == CHILD_ID_DHT_TEMP) {
    return readDHTTemp();
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_DHT_HUM) {
    return readDHTHum();
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_FLOW_A) {
    return getFlowForCanteiro(CHILD_ID_SOL_A);
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_FLOW_B) {
    return getFlowForCanteiro(CHILD_ID_SOL_B);
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_FLOW_N) {
    return getFlowForCanteiro(CHILD_ID_SOL_C); // Canteiro N no MUX é CHILD_ID_SOL_C
  }
  return readNodeItem(NODE_ITEMS[index].pin);
}

static void writeItem(uint8_t index, bool state) {
  writeNodeItem(NODE_ITEMS[index].pin, state);
}

namespace M360 {
void powerUp() {
  // Sem gerenciamento de VCC externo — relés controlam diretamente a carga
}

void powerDown() {
  // Sem gerenciamento de VCC externo
}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void presentation() { node.begin("NodeReles", "1.0"); }

void before() {
  Serial.begin(MY_BAUD_RATE);
  // initSensors() deve ser chamado ANTES do MySensors init (before() é o local
  // correto) para garantir que todos os relés estejam desligados antes de
  // qualquer operação.
  initSensors();
}

void setup() {
  node.onRead(readItem);
  node.onWrite(writeItem);
  // NOTA ARQUITETURAL: node.setupPins() é intencionalmente omitido.
  // Os pinos MUX usam valores virtuais (MUX_CHANNEL_OFFSET + N = 100..115)
  // que não são pinos físicos válidos no AVR ATmega328.
  // Todo o setup de hardware (pinMode + estado inicial) já foi realizado
  // em initSensors(), chamada em before() acima.
}

void loop() {
  updateFlows();

  // Reporte rápido de vazão em tempo real sempre que houver variação significativa
  static float lastSentFlowA = -1.0f;
  static float lastSentFlowB = -1.0f;
  static float lastSentFlowN = -1.0f;

  float flowA = getFlowForCanteiro(CHILD_ID_SOL_A);
  float flowB = getFlowForCanteiro(CHILD_ID_SOL_B);
  float flowN = getFlowForCanteiro(CHILD_ID_SOL_C); // Canteiro N no MUX é CHILD_ID_SOL_C

  // Envia atualização imediata se mudar significativamente
  if (fabsf(flowA - lastSentFlowA) > 0.02f || (flowA == 0.0f && lastSentFlowA > 0.0f)) {
    lastSentFlowA = flowA;
    send(messages[IDX_FLOW_A].set(flowA, 1));
  }
  if (fabsf(flowB - lastSentFlowB) > 0.02f || (flowB == 0.0f && lastSentFlowB > 0.0f)) {
    lastSentFlowB = flowB;
    send(messages[IDX_FLOW_B].set(flowB, 1));
  }
  if (fabsf(flowN - lastSentFlowN) > 0.02f || (flowN == 0.0f && lastSentFlowN > 0.0f)) {
    lastSentFlowN = flowN;
    send(messages[IDX_FLOW_N].set(flowN, 1));
  }

  node.process();

  // Log periódico de RSSI (saída Serial apenas — não enviado como sensor
  // MySensors)
#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
  if (millis() - lastRssiLog >= MY_RSSI_LOG_INTERVAL) {
    int16_t rssi = transportGetSignalReport(SR_RX_RSSI);
    Serial.print(F("[RSSI] "));
    Serial.print(rssi);
    Serial.println(F(" dBm"));
    lastRssiLog = millis();
  }
#endif
}

void receive(const MyMessage &msg) { node.handleMessage(msg); }
