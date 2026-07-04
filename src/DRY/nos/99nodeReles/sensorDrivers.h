/*
 * sensorDrivers.h — Driver de hardware para nodeReles (Nó 99)
 *
 * Central de Atuação da Estufa — Concentrador de Potência
 * Hardware: Arduino Nano (5V) + CD74HC4067 (MUX 16ch) + 16 Relés 10A
 *
 * Arquitetura de Pinos:
 *   MUX SIG  : D3  (sinal comum do CD74HC4067)
 *   MUX S0   : D4  |
 *   MUX S1   : D5  | Seleção de canal (4 bits — 0..15)
 *   MUX S2   : D6  |
 *   MUX S3   : D7  |
 *   MUX EN   : GND (sempre habilitado — sem pino de controle)
 *
 *   D2 : Bomba Circulação Principal NFT  (pino nativo, operação concorrente)
 *   D8 : Bomba Oxigenação NFT            (pino nativo, operação concorrente)
 *
 * Lógica dos Relés (optoacoplador): Active-LOW (LOW = Liga)
 *
 * Restrição de Concorrência do MUX:
 *   Apenas UM canal MUX pode estar ativo por vez.
 *   Ao ligar um canal MUX, o canal anteriormente ativo é desligado
 *   automaticamente antes de selecionar o novo (proteção contra picos).
 *   Pinos nativos D2 e D8 operam independentemente do MUX e entre si.
 */

#pragma once
#include <Arduino.h>

// ===== PINOS MUX CD74HC4067 =====
#define MUX_SIG_PIN     3
#define MUX_S0_PIN      4
#define MUX_S1_PIN      5
#define MUX_S2_PIN      6
#define MUX_S3_PIN      7

// ===== PINOS NATIVOS CONCORRENTES =====
#define PIN_NFT_PUMP    2   // Bomba Circulação Principal — Hidroponia NFT
#define PIN_NFT_OXI     8   // Bomba Oxigenação — Hidroponia NFT

// ===== CHILD IDs — Canais via MUX CD74HC4067 =====
#define CHILD_ID_SOL_A      0   // Canal 0 — Solenóide Gotejamento Canteiro A
#define CHILD_ID_SOL_B      1   // Canal 1 — Solenóide Gotejamento Canteiro B
#define CHILD_ID_SOL_C      2   // Canal 2 — Solenóide Gotejamento Canteiro C
#define CHILD_ID_PERIST_A   3   // Canal 3 — Peristáltica Suplemento A
#define CHILD_ID_PERIST_B   4   // Canal 4 — Peristáltica Suplemento B
#define CHILD_ID_PH_PLUS    5   // Canal 5 — Peristáltica pH+
#define CHILD_ID_PH_MINUS   6   // Canal 6 — Peristáltica pH-
// Canais 7-15: Reservados para expansão (climatização/iluminação)

// ===== CHILD IDs — Pinos Nativos Concorrentes =====
#define CHILD_ID_NFT_PUMP   16  // D2 — Bomba Circulação Principal NFT
#define CHILD_ID_NFT_OXI    17  // D8 — Bomba Oxigenação NFT
#define CHILD_ID_DHT_TEMP   18  // Sensor de Temperatura DHT11
#define CHILD_ID_DHT_HUM    19  // Sensor de Umidade DHT11

// ===== PINO NATIVO DHT11 =====
#define PIN_DHT             A0  // A0 (D14) para sinal digital do DHT11

// ===== ENCODING DE PINOS VIRTUAIS (MUX) =====
// Canais MUX são representados como pinos virtuais no campo `pin` de M360ItemDef:
//   pin = MUX_CHANNEL_OFFSET + channelNumber
// Exemplos: Canal 0 → 100 | Canal 4 → 104 | Canal 7 → 107
// Pinos nativos (2, 8) são usados diretamente sem offset.
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
