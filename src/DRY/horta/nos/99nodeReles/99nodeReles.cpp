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
 *   │  D3  │ Sensor de Vazão YF-S201 (INT1)                   │
 *   └──────┴──────────────────────────────────────────────────┘
 *
 * Macros MY_* definidas no platformio.ini [env:node_99_reles_nano]
 */

// ===== CONFIGURAÇÃO MYSENSORS =====
// Macros MY_* definidas no platformio.ini [env:nano_99reles]
// MySensors.h deve vir antes de M360.h para garantir sei() antes dos
// construtores globais

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"


#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
static unsigned long lastRssiLog = 0;
#endif

// ===== INSTRUMENTACAO DE DIAGNOSTICO (temporaria) =====
// Ativada apenas por -D NODE99_HEALTH_LOG (env nano_99reles_diag).
// Objetivo: separar travamento do loop() de exaustao de RAM.
//   - o tick para de sair  -> o loop() travou; bissectar node.process()
//   - o tick sai e ram cai -> colisao pilha/heap se aproximando
// Todas as strings ficam em flash via F(), para nao deslocar o proprio defeito.
#ifdef NODE99_HEALTH_LOG
static unsigned long lastHealthLog = 0;

// Distancia em bytes entre o topo do heap e o topo da pilha.
static int freeRam() {
  extern int __heap_start;
  extern int *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
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
     MUX_CHANNEL_OFFSET + 2, 0, 1, "Sol.CanteiroC", false, 0},
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
    // --- Vazão de irrigação (YF-S201 em D3/INT1) ---
    // pin = -1: a leitura não passa por readNodeItem(), é despachada por
    // childId em readItem() abaixo.
    {CHILD_ID_FLOW, M360::M360_SENSOR, S_WATER, V_FLOW, -1, 0, 1,
     "Vazao.Irrig", false, 0},
};
static const uint8_t NODE_ITEMS_COUNT =
    sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS =====
// +3 OBRIGATÓRIO: M360Node::begin() escreve _messages[_count], [_count+1] e
// [_count+2] — intervalo (254), bateria (255) e debug remoto (253).
// Dimensionar com +2 causa escrita fora do array (corrupção de RAM adjacente).
static MyMessage messages[NODE_ITEMS_COUNT + 3];
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

// ===== FAILSAFE DE ATUAÇÃO =====
// Um comando OFF perdido deixa a carga ligada indefinidamente. Isso não é
// hipotético: o desligamento da irrigação é agendado por `setTimeout()` dentro
// de um function node do Node-RED, que morre em qualquer redeploy ou restart —
// e o gateway publica o ACK assim que o rádio confirma o salto, então o
// Sincronizador nunca reenvia. Nenhuma camada acima do nó garante o OFF.
//
// Por isso cada atuador tem um tempo máximo ligado. Ao estourar, o nó desliga
// sozinho e envia V_STATUS=0 ao gateway, para o Node-RED reconciliar o estado.
// Esta é a única proteção que sobrevive à queda do WiFi, do MQTT, do Node-RED
// e do próprio gateway.
//
// 0 = sem limite. As bombas NFT (38/39) operam em regime contínuo por projeto:
// um timeout nelas interromperia a circulação da hidroponia.
static uint16_t maxOnSecondsFor(uint8_t childId) {
  switch (childId) {
  case CHILD_ID_SOL_A:
  case CHILD_ID_SOL_B:
  case CHILD_ID_SOL_C:
    return 600; // 10 min — a irrigação real usa no máximo 300 s (cron do A)
  case CHILD_ID_PERIST_A:
  case CHILD_ID_PERIST_B:
  case CHILD_ID_PH_PLUS:
  case CHILD_ID_PH_MINUS:
    return 120; // 2 min — dosagem de suplemento/pH é sempre curta
  default:
    return 0; // bombas NFT (38/39): regime contínuo, sem prazo
  }
}

// Instante (millis) em que cada atuador deve desligar sozinho.
// 0 = desarmado, e por isso um prazo que calhe em 0 é deslocado para 1.
static uint32_t actuatorDeadlineMs[NODE_ITEMS_COUNT];

static void armFailsafe(uint8_t index, bool state) {
  if (NODE_ITEMS[index].kind != M360::M360_ACTUATOR) {
    return;
  }

  if (!state) {
    actuatorDeadlineMs[index] = 0;
    return;
  }

  // Ligar um canal MUX desliga o anterior no hardware (ver writeNodeItem()):
  // desarmar os demais evita disparar o failsafe sobre um relé já desligado.
  // O eco de V_STATUS=0 do child preemptado é feito por writeItem().
  if (IS_MUX_CH(NODE_ITEMS[index].pin)) {
    for (uint8_t i = 0; i < NODE_ITEMS_COUNT; i++) {
      if (i != index && IS_MUX_CH(NODE_ITEMS[i].pin)) {
        actuatorDeadlineMs[i] = 0;
      }
    }
  }

  const uint16_t limite = maxOnSecondsFor(NODE_ITEMS[index].childId);
  if (limite == 0) {
    actuatorDeadlineMs[index] = 0;
    return;
  }

  uint32_t prazo = millis() + (uint32_t)limite * 1000UL;
  if (prazo == 0) {
    prazo = 1;
  }
  actuatorDeadlineMs[index] = prazo;
}

static void checkActuatorFailsafe() {
  const uint32_t agora = millis();

  for (uint8_t i = 0; i < NODE_ITEMS_COUNT; i++) {
    if (actuatorDeadlineMs[i] == 0) {
      continue;
    }
    // Comparação por subtração com sinal: correta no overflow de millis()
    // (~49 dias), ao contrário de `agora >= prazo`.
    if ((int32_t)(agora - actuatorDeadlineMs[i]) < 0) {
      continue;
    }

    actuatorDeadlineMs[i] = 0;
    writeNodeItem(NODE_ITEMS[i].pin, false);
    send(messages[i].set(false)); // reconcilia o estado no gateway/Node-RED

    Serial.print(F("FAILSAFE OFF child "));
    Serial.println(NODE_ITEMS[i].childId);
  }
}

// ===== CALLBACKS =====

static float readItem(uint8_t index) {
  if (NODE_ITEMS[index].childId == CHILD_ID_DHT_TEMP) {
    return readDHTTemp();
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_DHT_HUM) {
    return readDHTHum();
  }
  if (NODE_ITEMS[index].childId == CHILD_ID_FLOW) {
    return readFlowLpm();
  }
  return readNodeItem(NODE_ITEMS[index].pin);
}

// Ligar um canal MUX desliga o canal anterior no hardware. Sem este eco, o
// gateway e o Node-RED continuariam mostrando o child preemptado como ligado —
// e a irrigação daquele canteiro seria cortada sem nenhum sinal em lugar algum.
// Enviado ANTES do eco do child comandado (M360Node::handleMessage() envia o
// dele logo após esta callback), então o Node-RED vê "31→0" e depois "32→1".
static void reportPreemptedMuxChannel(int8_t channel) {
  for (uint8_t i = 0; i < NODE_ITEMS_COUNT; i++) {
    if (NODE_ITEMS[i].pin == MUX_CHANNEL_OFFSET + channel) {
      send(messages[i].set(false));
      Serial.print(F("MUX preempcao OFF child "));
      Serial.println(NODE_ITEMS[i].childId);
      return;
    }
  }
}

static void writeItem(uint8_t index, bool state) {
  const int8_t preempted = writeNodeItem(NODE_ITEMS[index].pin, state);
  armFailsafe(index, state);

  if (preempted >= 0) {
    reportPreemptedMuxChannel(preempted);
  }
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

void presentation() { node.begin("NodeReles", "2.0.0"); }


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
  node.process();

  // node.process() em M360_ALWAYS_ON termina em wait(50), então o failsafe é
  // avaliado ~20x por segundo — resolução de sobra para prazos de minutos.
  checkActuatorFailsafe();

#ifdef NODE99_HEALTH_LOG
  // Batimento a cada 5 s: uptime em segundos e RAM livre.
  if (millis() - lastHealthLog >= 5000UL) {
    lastHealthLog = millis();
    Serial.print(F("[TICK] up="));
    Serial.print(millis() / 1000UL);
    Serial.print(F("s ram="));
    Serial.println(freeRam());

    // A cada 6 ticks (30 s), testa se o radio ainda TRANSMITE.
    // st=OK prova que o gateway respondeu o auto-ACK de hardware, ou seja,
    // que o radio do no ainda fala E ouve a resposta imediata. Combinado com
    // MAX_RT no sentido gateway->no, isola o defeito no modo de escuta (RX).
    static uint8_t txProbe = 0;
    if (++txProbe >= 6) {
      txProbe = 0;
      MyMessage probe(M360_CHILD_ID_DEBUG, V_TEXT);
      const bool ok = send(probe.set("probe"));
      Serial.print(F("[TXPROBE] "));
      Serial.println(ok ? F("OK") : F("NACK"));
    }
  }
#endif


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
