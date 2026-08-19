/*
 * sensorDrivers.h — Driver de hardware para nodeReles (Nó 99)
 *
 * Central de Atuação da Estufa — Concentrador de Potência
 * Hardware: Arduino Nano (5V) + CD74HC4067 (MUX 16ch) + 16 Relés 10A
 *
 * Arquitetura de Pinos:
 *   MUX SIG  : D8  (sinal comum do CD74HC4067)
 *   MUX S0   : D4  |
 *   MUX S1   : D5  | Seleção de canal (4 bits — 0..15)
 *   MUX S2   : D6  |
 *   MUX S3   : D7  |
 *   MUX EN   : GND (sempre habilitado — sem pino de controle)
 *
 *   A0 (D14) : Bomba Circulação Principal NFT  (pino nativo, operação concorrente)
 *   A1 (D15) : Bomba Oxigenação NFT            (pino nativo, operação concorrente)
 *   D2       : Sensor DHT11 DATA                (operação concorrente)
 *
 * Lógica dos Relés (optoacoplador): Active-LOW (LOW = Liga)
 *
 * Restrição de Concorrência do MUX:
 *   Apenas UM canal MUX pode estar ativo por vez.
 *   Ao ligar um canal MUX, o canal anteriormente ativo é desligado
 *   automaticamente antes de selecionar o novo (proteção contra picos).
 *   Pinos nativos A0 e A1 operam independentemente do MUX e entre si.
 */

#pragma once
#include <Arduino.h>

// ===== PINOS MUX CD74HC4067 =====
#define MUX_SIG_PIN     8
#define MUX_S0_PIN      4
#define MUX_S1_PIN      5
#define MUX_S2_PIN      6
#define MUX_S3_PIN      7

// ===== PINOS NATIVOS CONCORRENTES =====
#define PIN_NFT_PUMP    A0  // Bomba Circulação Principal — Hidroponia NFT
#define PIN_NFT_OXI     A1  // Bomba Oxigenação — Hidroponia NFT

// ===== CHILD IDs — FAIXAS NORMATIVAS M360 =====
// Os child IDs seguem as faixas de M360Constants.h e SÃO CONTRATO com o
// Node-RED (flows.json), que endereça os atuadores por esses números.
// NÃO confundir com o canal do MUX: o canal vive no campo `pin` de
// M360ItemDef, como MUX_CHANNEL_OFFSET + canal.
//
//   Clima      11-20  (CHILD_ID_CLIMA_MIN/MAX)
//   Hidrometria 21-30 (CHILD_ID_FLOW_MIN/MAX)
//   Atuação    31-40  (CHILD_ID_ACTUATOR_MIN/MAX)

// ----- Clima (11-20) -----
#define CHILD_ID_DHT_TEMP   11  // Temperatura do Ar (DHT11, D2)
#define CHILD_ID_DHT_HUM    12  // Umidade do Ar (DHT11, D2)

// ----- Hidrometria (21-30) -----
#define CHILD_ID_FLOW       21  // Vazão de irrigação (YF-S201, D3/INT1)

// ----- Atuação via MUX CD74HC4067 (31-40) -----
#define CHILD_ID_SOL_A      31  // Canal MUX 0 — Solenóide Gotejamento Canteiro A
#define CHILD_ID_SOL_B      32  // Canal MUX 1 — Solenóide Gotejamento Canteiro B
#define CHILD_ID_SOL_C      33  // Canal MUX 2 — Solenóide Gotejamento Canteiro C
#define CHILD_ID_PERIST_A   34  // Canal MUX 3 — Peristáltica Suplemento A
#define CHILD_ID_PERIST_B   35  // Canal MUX 4 — Peristáltica Suplemento B
#define CHILD_ID_PH_PLUS    36  // Canal MUX 5 — Peristáltica pH+
#define CHILD_ID_PH_MINUS   37  // Canal MUX 6 — Peristáltica pH-
// Canais MUX 7-15: Reservados para expansão (climatização/iluminação)

// ----- Atuação via pinos nativos (31-40) -----
#define CHILD_ID_NFT_PUMP   38  // A0 — Bomba Circulação Principal NFT
#define CHILD_ID_NFT_OXI    39  // A1 — Bomba Oxigenação NFT

// ===== PINO NATIVO DHT11 =====
#define PIN_DHT             2   // D2 para sinal digital do DHT11

// ===== SENSOR DE VAZÃO YF-S201 =====
// D3 é o único pino de interrupção externa livre no Nano (INT1):
// D2 = DHT11, D4-D8 = MUX, D9-D13 = NRF24, A0/A1 = bombas.
#define PIN_FLOW            3      // D3 / INT1 — sinal de pulso do YF-S201
// Fator K do YF-S201: F(Hz) = 7.5 x Q(L/min)  →  450 pulsos por litro.
#define FLOW_K_FACTOR       7.5f

// ===== ENCODING DE PINOS VIRTUAIS (MUX) =====
// Canais MUX são representados como pinos virtuais no campo `pin` de M360ItemDef:
//   pin = MUX_CHANNEL_OFFSET + channelNumber
// Exemplos: Canal 0 → 100 | Canal 4 → 104 | Canal 7 → 107
// Pinos nativos (A0, A1) são usados diretamente sem offset.
#define MUX_CHANNEL_OFFSET  100
#define IS_MUX_CH(pin)      ((pin) >= MUX_CHANNEL_OFFSET && (pin) < (MUX_CHANNEL_OFFSET + 16))
#define MUX_CH(pin)         ((uint8_t)((pin) - MUX_CHANNEL_OFFSET))

// ===== FUNÇÕES EXPOSTAS =====

/**
 * Inicializa todos os pinos (MUX + nativos) e garante todos os relés
 * desligados no boot. Deve ser chamada em before() antes de MySensors init.
 */
void initSensors();

/**
 * Escreve estado em atuador. O parâmetro `pin` pode ser:
 *   - Pino virtual MUX:  MUX_CHANNEL_OFFSET + canal (ex: 104 para canal 4)
 *   - Pino físico nativo: PIN_NFT_PUMP (2) ou PIN_NFT_OXI (8)
 * Impõe restrição de concorrência MUX automaticamente.
 */
void writeNodeItem(uint8_t pin, bool state);

/**
 * Lê estado lógico do atuador.
 * Retorna 1.0f = ligado, 0.0f = desligado.
 * Para canais MUX, retorna o estado rastreado em software.
 */
float readNodeItem(uint8_t pin);

/**
 * Lê a temperatura do sensor DHT11.
 */
float readDHTTemp();

/**
 * Lê a umidade do sensor DHT11.
 */
float readDHTHum();

/**
 * Configura o pino de vazão e registra a interrupção de contagem de pulsos.
 * Chamada por initSensors(); não precisa ser invocada diretamente.
 */
void initFlowSensor();

/**
 * Lê a vazão instantânea em litros/minuto e reinicia a janela de contagem.
 *
 * Calcula a média sobre o intervalo decorrido desde a chamada anterior:
 *   Q(L/min) = (pulsos / Δt_segundos) / FLOW_K_FACTOR
 *
 * CONSUMPTIVO: zera o contador a cada chamada. Chamar apenas no ciclo de
 * leitura do M360Node (via readItem), nunca em dois pontos concorrentes,
 * sob pena de dividir os pulsos entre os chamadores.
 */
float readFlowLpm();
