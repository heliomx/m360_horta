/*
 * Node Engine - Motor Genérico para Nós MySensors
 * 
 * Implementa a lógica comum de todos os nós MySensors de forma DRY,
 * permitindo que cada nó seja implementado com apenas algumas linhas de código.
 */

#pragma once
#include <Arduino.h>
#include "config.h"
#include "powerProfile.h"

// Forward declarations para evitar dependências circulares
struct MyMessage;

// ===== ESTRUTURAS DE DADOS =====

typedef enum { NODE_ITEM_SENSOR, NODE_ITEM_ACTUATOR } NodeItemKind;

typedef struct {
  uint8_t childId;
  NodeItemKind kind;
  uint8_t presentationType;
  uint8_t valueType;
  int pin;
  uint32_t reportIntervalMin;
  uint8_t readSamples;
  const char* label;
  bool wakeOnRadio;
  uint8_t flags;  // Bit 0: multiplica por 100 antes de enviar
} NodeItemDef;

// ===== DECLARAÇÕES EXTERNAS =====

// Arrays que cada nó deve definir
extern const NodeItemDef NODE_ITEMS[];
extern const uint8_t NODE_ITEMS_COUNT;

// Node ID deve ser definido por cada nó antes de incluir este header

// Função que cada nó deve implementar para ler sensores
extern float readNodeItem(uint8_t nodeIndex);

// ===== FUNÇÕES PÚBLICAS DO ENGINE =====

// Funções principais do MySensors que o engine implementa
void nodeEngine_presentation();
void nodeEngine_setup();
void nodeEngine_loop();
void nodeEngine_handleReceive(const MyMessage &m);

// ===== FUNÇÕES AUXILIARES =====

// Gerenciamento de energia
void nodeEngine_powerDown();
void nodeEngine_powerUp();

// Leitura de bateria
float nodeEngine_readBattery();
uint8_t nodeEngine_voltageToPercent(float voltage);
uint8_t nodeEngine_readBatteryPercent();

// Gerenciamento de intervalo
void nodeEngine_saveInterval(uint16_t interval);
uint16_t nodeEngine_loadInterval();

// Inicialização de arrays de histórico (lastValues → NAN, nNoUpdates → 0)
void nodeEngine_initArrays(float* lastValues, uint8_t* nNoUpdates, uint8_t count);

// Configura pinos de OUTPUT/INPUT com base em NODE_ITEMS[]
void nodeEngine_setupPins();

// ===== MACROS DRY (expandidas no contexto do nó — requerem MySensors) =====

// Define as variáveis globais padrão de um nó (sensores + bateria)
#define NODE_ENGINE_DEFINE_GLOBALS()          \
  volatile bool wokeUpByRadio = false;        \
  uint16_t updateInterval = DEFAULT_INTERVAL; \
  float    lastValues[NODE_ITEMS_COUNT];      \
  uint8_t  nNoUpdates[NODE_ITEMS_COUNT];      \
  uint8_t  lastBatt = 0;                      \
  MyMessage messages[NODE_ITEMS_COUNT + 2]

// Corpo completo de presentation(): apresenta sensores e inicializa messages[]
#define NODE_ENGINE_PRESENTATION(name, version)                                    \
  sendSketchInfo(F(name), F(version));                                             \
  for (uint8_t _i = 0; _i < NODE_ITEMS_COUNT; _i++) {                            \
    present(NODE_ITEMS[_i].childId,                                                \
            (mysensors_sensor_t)NODE_ITEMS[_i].presentationType,                   \
            NODE_ITEMS[_i].label);                                                 \
    messages[_i] = MyMessage(NODE_ITEMS[_i].childId,                              \
                             (mysensors_data_t)NODE_ITEMS[_i].valueType);          \
  }                                                                                \
  present(CHILD_ID_INTERVAL, S_CUSTOM,     F("Intervalo"));                       \
  present(CHILD_ID_BATTERY,  S_MULTIMETER, F("Bateria"));                         \
  messages[NODE_ITEMS_COUNT]   = MyMessage(CHILD_ID_INTERVAL, V_VAR1);            \
  messages[NODE_ITEMS_COUNT+1] = MyMessage(CHILD_ID_BATTERY,  V_VOLTAGE)

// Valida e aplica novo intervalo recebido via V_VAR1 ou V_VAR5
#define NODE_ENGINE_HANDLE_INTERVAL(msg)                                           \
  do {                                                                             \
    if ((msg).getSensor() == CHILD_ID_INTERVAL &&                                  \
        ((msg).getType() == V_VAR1 || (msg).getType() == V_VAR5)) {               \
      uint16_t _iv = (msg).getUInt();                                              \
      if (_iv >= MIN_INTERVAL && _iv <= MAX_INTERVAL) {                           \
        updateInterval = _iv;                                                      \
        nodeEngine_saveInterval(updateInterval);                                   \
        send(messages[NODE_ITEMS_COUNT].set(updateInterval));                      \
        Serial.print(F("INT:")); Serial.println(updateInterval);                  \
      }                                                                            \
    }                                                                              \
  } while(0)

// Controla atuadores via V_STATUS (busca em NODE_ITEMS por childId)
#define NODE_ENGINE_HANDLE_ACTUATORS(msg)                                          \
  do {                                                                             \
    if ((msg).getType() == V_STATUS) {                                             \
      for (uint8_t _i = 0; _i < NODE_ITEMS_COUNT; _i++) {                        \
        if ((msg).getSensor() == NODE_ITEMS[_i].childId &&                        \
            NODE_ITEMS[_i].kind == NODE_ITEM_ACTUATOR) {                          \
          bool _s = (msg).getBool();                                               \
          writeNodeItem(_i, _s);                                                   \
          send(messages[_i].set(_s));                                              \
          break;                                                                   \
        }                                                                          \
      }                                                                            \
    }                                                                              \
  } while(0)

// Imprime diagnóstico de rede uma vez por boot
#define NODE_ENGINE_CHECK_TRANSPORT()                                              \
  do {                                                                             \
    static bool _netChecked = false;                                               \
    if (!_netChecked) {                                                            \
      _netChecked = true;                                                          \
      Serial.print(F("ID:"));     Serial.println(getNodeId());                    \
      Serial.print(F("Parent:")); Serial.println(getParentNodeId());              \
      Serial.print(F("Dist:"));   Serial.println(getDistanceGW());                \
      Serial.print(F("Ready:"));  Serial.println(isTransportReady() ? F("S") : F("N")); \
    }                                                                              \
  } while(0)

// Relança presentation() quando recebe V_CUSTOM com payload "REPRESENT"
#define NODE_ENGINE_HANDLE_REREPRESENTATION(msg, name, version)              \
  do {                                                                        \
    if ((msg).getType() == V_CUSTOM) {                                       \
      char _rbuf[12];                                                         \
      (msg).getString(_rbuf);                                                 \
      if (strcmp(_rbuf, "REPRESENT") == 0) {                                 \
        NODE_ENGINE_PRESENTATION(name, version);                              \
        Serial.println(F("REPRES:OK"));                                       \
      }                                                                       \
    }                                                                         \
  } while(0)

// Envia mensagem de teste de conectividade ao gateway
#define NODE_ENGINE_TEST_CONNECTIVITY()                                            \
  do {                                                                             \
    MyMessage _ctm(CHILD_ID_INTERVAL, V_CUSTOM);                                  \
    bool _cok = send(_ctm.set("TESTE_CONEXAO"));                                  \
    Serial.println(_cok ? F("Conn:OK") : F("Conn:FAIL"));                        \
  } while(0)

// Lê e envia porcentagem de bateria a cada N ciclos de loop
#define NODE_ENGINE_PROCESS_BATTERY(cycles)                                        \
  do {                                                                             \
    static uint8_t _bc = 0;                                                       \
    if (_bc++ >= (cycles)) {                                                       \
      _bc = 0;                                                                     \
      uint8_t _b = nodeEngine_readBatteryPercent();                               \
      if (abs(_b - lastBatt) >= 1) {                                              \
        lastBatt = _b;                                                             \
        send(messages[NODE_ITEMS_COUNT + 1].set(_b));                             \
        Serial.print(F("Bat:")); Serial.println(_b);                              \
      }                                                                            \
    }                                                                              \
  } while(0)

// ===== VARIÁVEIS GLOBAIS DO ENGINE =====
// As variáveis globais são definidas no arquivo principal de cada nó via NODE_ENGINE_DEFINE_GLOBALS()
