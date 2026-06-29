/*
 * nodeReles.cpp — Nó 99: Central de Atuação da Estufa (M360-DRY)
 *
 * Hardware: Arduino Nano (5V) + CD74HC4067 (MUX 16ch) + 16 Relés 10A (optoacoplador)
 *           + nRF24L01+ (adaptador socket regulado 3.3V) — CE=D9, CSN=D10
 *
 * Arquitetura de Atuação:
 *
 *   Canais via MUX CD74HC4067 (SIG=D3 | S0-S3=D4-D7 | EN=GND):
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
 *   │  D2  │ Bomba Circulação Principal — Hidroponia NFT      │
 *   │  D8  │ Bomba Oxigenação — Hidroponia NFT                │
 *   └──────┴──────────────────────────────────────────────────┘
 *
 * Macros MY_* definidas no platformio.ini [env:node_99_reles_nano]
 */

// ===== CONFIGURAÇÃO MYSENSORS =====
// Macros MY_* definidas no platformio.ini [env:nano_99reles]
// MySensors.h deve vir antes de M360.h para garantir sei() antes dos construtores globais

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
static unsigned long lastRssiLog = 0;
#endif

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// Colunas: childId | kind | presentType | valueType | pin | intMin | smp | label | wakeOnRadio | flags
//
// Pinos MUX usam encoding virtual: pin = MUX_CHANNEL_OFFSET + canal
// (ex.: canal 4 → pin 104). Ver sensorDrivers.h para detalhes.
static const M360::M360ItemDef NODE_ITEMS[] = {
    // --- Canais MUX (concorrência restrita: 1 ativo por vez) ---
    {CHILD_ID_SOL_A,    M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 0, 0, 1, "Sol.CanteiroA", false, 0},
    {CHILD_ID_SOL_B,    M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 1, 0, 1, "Sol.CanteiroB", false, 0},
    {CHILD_ID_SOL_C,    M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 2, 0, 1, "Sol.CanteiroC", false, 0},
    {CHILD_ID_PERIST_A, M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 3, 0, 1, "Perist.SuplA",  false, 0},
    {CHILD_ID_PERIST_B, M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 4, 0, 1, "Perist.SuplB",  false, 0},
    {CHILD_ID_PH_PLUS,  M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 5, 0, 1, "Perist.pH+",    false, 0},
    {CHILD_ID_PH_MINUS, M360::M360_ACTUATOR, S_BINARY, V_STATUS, MUX_CHANNEL_OFFSET + 6, 0, 1, "Perist.pH-",    false, 0},
    // --- Pinos Nativos (concorrência livre com MUX e entre si) ---
    {CHILD_ID_NFT_PUMP, M360::M360_ACTUATOR, S_BINARY, V_STATUS, PIN_NFT_PUMP,            0, 1, "BombaNFT",     false, 0},
    {CHILD_ID_NFT_OXI,  M360::M360_ACTUATOR, S_BINARY, V_STATUS, PIN_NFT_OXI,             0, 1, "BombaOxi",     false, 0},
    // --- Sensores Nativos (DHT11 - Concorrência livre) ---
    {CHILD_ID_DHT_TEMP, M360::M360_SENSOR,   S_TEMP,   V_TEMP,   PIN_DHT,                 0, 1, "TempReles",    false, 0},
    {CHILD_ID_DHT_HUM,  M360::M360_SENSOR,   S_HUM,    V_HUM,    PIN_DHT,                 0, 1, "UmidReles",    false, 0},
};
static const uint8_t NODE_ITEMS_COUNT =
    sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS =====
static MyMessage messages[NODE_ITEMS_COUNT + 2]; // +1 Intervalo (ID 254) +1 Bateria (ID 255)
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
// MY_REPEATER_FEATURE (via build_flags) ativa encaminhamento de mensagens de outros nós.
// Use: pio run -e nano_99reles_rep -t upload
#ifdef MY_REPEATER_FEATURE
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_REPEATER);
#else
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);
#endif

// ===== CALLBACKS =====

static float readItem(uint8_t index)
{
    if (NODE_ITEMS[index].childId == CHILD_ID_DHT_TEMP) {
        return readDHTTemp();
    }
    if (NODE_ITEMS[index].childId == CHILD_ID_DHT_HUM) {
        return readDHTHum();
    }
    return readNodeItem(NODE_ITEMS[index].pin);
}

static void writeItem(uint8_t index, bool state)
{
    writeNodeItem(NODE_ITEMS[index].pin, state);
}

namespace M360 {
void powerUp()
{
    // Sem gerenciamento de VCC externo — relés controlam diretamente a carga
}

void powerDown()
{
    // Sem gerenciamento de VCC externo
}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void presentation()
{
    node.begin("NodeReles", "1.0");
}

void before()
{
    Serial.begin(MY_BAUD_RATE);
    // initSensors() deve ser chamado ANTES do MySensors init (before() é o local correto)
    // para garantir que todos os relés estejam desligados antes de qualquer operação.
    initSensors();
}

void setup()
{
    node.onRead(readItem);
    node.onWrite(writeItem);
    // NOTA ARQUITETURAL: node.setupPins() é intencionalmente omitido.
    // Os pinos MUX usam valores virtuais (MUX_CHANNEL_OFFSET + N = 100..115)
    // que não são pinos físicos válidos no AVR ATmega328.
    // Todo o setup de hardware (pinMode + estado inicial) já foi realizado
    // em initSensors(), chamada em before() acima.
}

void loop()
{
    node.process();

    // Log periódico de RSSI (saída Serial apenas — não enviado como sensor MySensors)
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

void receive(const MyMessage &msg)
{
    node.handleMessage(msg);
}
