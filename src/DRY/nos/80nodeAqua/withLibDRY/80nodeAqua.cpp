/*
 * 80nodeAqua.cpp — Nó 80: Monitoramento da Caixa de Água, pH, EC e Vazão
 *
 * Hardware: Arduino Nano + nRF24L01+ + HC-SR04 + pH-4502C + EC + YF-S201
 * Alimentação: 5V contínuo (ALWAYS_ON)
 * Gestão de VCC: Pino D7 chaveia a energia dos sensores
 */

#include "sensorDrivers.h"
#include <Arduino.h>
#include <M360.h>
#include <MySensors.h>

// ===== CHILD IDs =====
#define CHILD_ID_LEVEL      0
#define CHILD_ID_PH         1
#define CHILD_ID_EC         2
#define CHILD_ID_FLOW       3
#define CHILD_ID_WATER_TEMP 4
#define CHILD_ID_FLOW_A     5
#define CHILD_ID_FLOW_B     6
#define CHILD_ID_FLOW_C     7

// Índices no array NODE_ITEMS (espelham os CHILD_IDs acima, mas são conceitos distintos).
// Usar IDX_* ao acessar lastValues[], nNoUpdates[] e messages[] por posição.
static const uint8_t IDX_PH         = 1;
static const uint8_t IDX_EC         = 2;
static const uint8_t IDX_WATER_TEMP = 4;

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples |
// label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
    {CHILD_ID_LEVEL, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 1, 3,
     "Nivel da Caixa D'agua", false, 0},
    {CHILD_ID_PH, M360::M360_SENSOR, S_WATER_QUALITY, V_PH, -1, 1, 3, "pH",
     false, 0},
    {CHILD_ID_EC, M360::M360_SENSOR, S_WATER_QUALITY, V_EC, -1, 1, 3, "EC",
     false, 0},
    {CHILD_ID_FLOW, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao da Hidroponia", false, 0},
    {CHILD_ID_WATER_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3,
     "Temp Agua", false, 0},
    {CHILD_ID_FLOW_A, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao Canteiro A", false, 0},
    {CHILD_ID_FLOW_B, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao Canteiro B", false, 0},
    {CHILD_ID_FLOW_C, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 1, 1,
     "Vazao Canteiro C", false, 0}};
static const uint8_t NODE_ITEMS_COUNT =
    sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocados pelo nó, gerenciados pelo M360Node) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float lastValues[NODE_ITEMS_COUNT];
static uint8_t nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

// ===== CALLBACKS DE ENERGIA =====

namespace M360 {
void powerUp() { powerUpSensors(); }

void powerDown() { powerDownSensors(); }
} // namespace M360

// ===== CONTROLE DE TIMING PERSONALIZADO PARA SONDAS (pH, EC, Temp) =====
static bool vazaoH_wasActive = false;
static unsigned long vazaoH_activeStartTime = 0;
static unsigned long lastAquaSampleTime = 0;

bool shouldSampleAqua() {
  unsigned long now = millis();
  // Vazão H está ativa se a vazão atual medida for maior que zero
  bool active = (currentFlowH > 0.001f);

  if (active) {
    if (!vazaoH_wasActive) {
      // Vazão H acabou de ser ativada!
      vazaoH_wasActive = true;
      vazaoH_activeStartTime = now;
      // Agenda a primeira leitura para daqui a 2 minutos
      lastAquaSampleTime = now;
      return false; // Não lê agora, espera a estabilização de 2 minutos
    } else {
      // Vazão H está ligada: verifica se passaram 2 minutos desde a ativação e
      // desde a última amostragem
      if (now - vazaoH_activeStartTime >= 120000UL) {
        if (now - lastAquaSampleTime >= 120000UL) {
          lastAquaSampleTime = now;
          return true;
        }
      }
    }
  } else {
    if (vazaoH_wasActive) {
      // Vazão H desligou!
      vazaoH_wasActive = false;
      // Reinicia a referência de tempo para a próxima amostragem de 10 minutos
      lastAquaSampleTime = now;
    } else {
      // Vazão H desligada: amostra a cada 10 minutos (600.000 ms)
      if (lastAquaSampleTime == 0 || (now - lastAquaSampleTime) >= 600000UL) {
        lastAquaSampleTime = now;
        return true;
      }
    }
  }
  return false;
}

void performAquaSampling() {
  allowAquaRead = true;

  float ph = readNodeItem(IDX_PH);
  if (!isnan(ph)) {
    lastValues[IDX_PH] = ph;
    nNoUpdates[IDX_PH] = 0;
    send(messages[IDX_PH].set(ph, 1));
  }

  float ec = readNodeItem(IDX_EC);
  if (!isnan(ec)) {
    lastValues[IDX_EC] = ec;
    nNoUpdates[IDX_EC] = 0;
    send(messages[IDX_EC].set(ec, 1));
  }

  float temp = readNodeItem(IDX_WATER_TEMP);
  if (!isnan(temp)) {
    lastValues[IDX_WATER_TEMP] = temp;
    nNoUpdates[IDX_WATER_TEMP] = 0;
    send(messages[IDX_WATER_TEMP].set(temp, 1));
  }

  allowAquaRead = false;
}

// ===== MYSENSORS HOOKS =====

void before() {
  Serial.begin(MY_BAUD_RATE);
  initSensors();
  powerUpSensors();
}

void presentation() { node.begin("80nodeAqua", "1.0"); }

void setup() {
  node.onRead(readNodeItem);
  node.onWrite(writeNodeItem);
}

void loop() {
  updateFlows();

  if (shouldSampleAqua()) {
    performAquaSampling();
  }

  node.process();
}

void receive(const MyMessage &msg) {
  if (msg.getType() == V_CUSTOM) {
    char buf[24];
    msg.getString(buf);
    if (strcmp(buf, M360::CMD_FORCE_UPDATE) == 0) {
      allowAquaRead = true;
    }
  }
  node.handleMessage(msg);
  allowAquaRead = false;
}
