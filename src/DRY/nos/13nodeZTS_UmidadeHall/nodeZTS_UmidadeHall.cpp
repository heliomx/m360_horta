/*
 * M360 Network - Nó ZTS + Umidade Hall + Selenoide (M360-DRY v2)
 *
 * Arquitetura de Pinos (Conforme Esquema Elétrico):
 * - RS485 (MAX485): RO:D9, DI:D10, DE:D7, RE:D8
 * - Rádio (NRF24): CE:D6, CSN:D4 (Evita conflito com D9/D10)
 * - Atuadores: Selenoide em D5
 * - Sensores: Hall em A0, LDR em A1
 */

// ===== PERFIL DE ENERGIA (12V - FONTE FIXA) =====
#include <Arduino.h>
#include <MySensors.h>
#include <M360Node.h>
#include "sensorDrivers.h"

using namespace M360;

// ===== DEFINIÇÃO DO NÓ =====
// childId               | kind          | presentType   | valueType | pin | intMin | smp | label           | wakeOnRadio | flags
static const M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID_MOISTURE,     M360_SENSOR,   S_MOISTURE,    V_LEVEL,    -1, 1, 1, "Umidade ZTS",   false, 0 },
    { CHILD_ID_TEMP,         M360_SENSOR,   S_TEMP,        V_TEMP,     -1, 1, 1, "Temp ZTS",      false, 0 },
    { CHILD_ID_CONDUCTIVITY, M360_SENSOR,   S_CUSTOM,      V_VAR1,     -1, 1, 1, "Cond Solo",     false, 0 },
    { CHILD_ID_PH,           M360_SENSOR,   S_CUSTOM,      V_VAR2,     -1, 1, 1, "pH Solo",       false, 0 },
    { CHILD_ID_NITROGEN,     M360_SENSOR,   S_CUSTOM,      V_VAR3,     -1, 1, 1, "N Solo",        false, 0 },
    { CHILD_ID_PHOSPHORUS,   M360_SENSOR,   S_CUSTOM,      V_VAR4,     -1, 1, 1, "P Solo",        false, 0 },
    { CHILD_ID_POTASSIUM,    M360_SENSOR,   S_CUSTOM,      V_VAR5,     -1, 1, 1, "K Solo",        false, 0 },
    { CHILD_ID_SELENOIDE,    M360_ACTUATOR, S_BINARY,      V_STATUS,    5, 0, 0, "Válvula",       false, 0 },
    { CHILD_ID_UMIDADE_HALL, M360_SENSOR,   S_HUM,         V_HUM,      A0, 1, 1, "Umidade Hall",  false, 0 },
    { CHILD_ID_LDR,          M360_SENSOR,   S_LIGHT_LEVEL, V_LIGHT_LEVEL, A1, 1, 1, "Luminosidade", false, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates,
                     M360_ALWAYS_ON);

// ===== MYSENSORS HOOKS =====

void presentation() {
    node.begin("M360 ZTS+Hall DRY 12V", "2.1");
}

void setup() {
    Serial.begin(MY_BAUD_RATE);
    initDrivers();
    node.onRead(readNodeItem);
    node.onWrite(writeNodeItem);
    // setupPins() é omitido: initDrivers() já configura os pinos do nó
}

void loop() {
    node.process();
}

void receive(const MyMessage& msg) {
    node.handleMessage(msg);
}
