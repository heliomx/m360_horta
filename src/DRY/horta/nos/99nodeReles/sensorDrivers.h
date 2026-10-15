/*
 * sensorDrivers.h — Driver de hardware para nodeReles (Nó 99)
 *
 * Central de Atuação da Estufa — Concentrador de Potência
 *
 * Duas placas, um único fonte. As diferenças ficam atrás de -D NODE99_BOARD_ACT,
 * definido só no env act_99 do platformio.ini (um env por placa — decisão 8 do
 * hardware/ARQUITETURA_FAMILIA.md). Sem a flag, compila para o Nano atual.
 *
 * --- Arduino Nano 5 V (envs nano_99reles*) ---
 *   Hardware: Nano + CD74HC4067 (MUX 16ch) + 9 relés + DHT11 + YF-S201
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
 *   D3       : Vazão YF-S201 (INT1), INPUT_PULLUP
 *
 * --- Placa M360-ACT, Pro Mini 3,3 V / 8 MHz (envs act_99*) ---
 *   Pinagem da netlist de 2026-10-14 (hardware/ARQUITETURA_FAMILIA.md §5.4):
 *   D4–D7    : S0–S3 do MUX, via 74HCT244 (fronteira 3,3 → 5 V)
 *   D8       : SIG do MUX, via 74HCT244
 *   A0 / A1  : Bomba NFT / Bomba Oxi, via 74HCT244
 *   D3       : Vazão YF-S201 (INT1), INPUT puro — a placa tem R3 10 kΩ em
 *              série, R4 100 kΩ para GND e clamp BAT54WS no 3,3 V; o pull-up
 *              interno contra o R3 deixaria o nível baixo acima do VIL
 *   A5       : /OE do 74HCT244, pull-up R2 para 3,3 V. Liberado (LOW) por
 *              ÚLTIMO em initSensors(), com D4–D8, A0 e A1 já em OUTPUT e os
 *              relés desligados. Enquanto está alto nenhum relé aciona
 *   D2 / A2  : DIO0 / DIO1 do RFM95W (nRF24 legado no J6: CE=D2, CSN=D10)
 *   A3       : RESET do rádio (MY_RFM95_RST_PIN=17, só no env)
 *   A4       : reed switch (sem uso no firmware por enquanto)
 *   A6       : entrada analógica da porta J5 (divisor 10 k / 20 k), sem child
 *   D9       : livre
 *   Sem DHT  : os children 11 e 12 não existem nesta variante
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

#ifdef NODE99_BOARD_ACT
// ===== /OE DO 74HCT244 (só M360-ACT) =====
// Active-LOW. Enquanto o A5 está em alta impedância (boot, reset, gravação) o
// pull-up R2 mantém as saídas do buffer soltas — e entrada solta = relé
// desligado. É a mesma regra de segurança do Nano; aqui ela tem um pino.
#define PIN_OE_N        A5
#endif

// ===== CHILD IDs — FAIXAS NORMATIVAS M360 =====
// Os child IDs seguem as faixas de M360Constants.h e SÃO CONTRATO com o
// Node-RED (flows.json), que endereça os atuadores por esses números.
// NÃO confundir com o canal do MUX: o canal vive no campo `pin` de
// M360ItemDef, como MUX_CHANNEL_OFFSET + canal.
//
//   Clima      11-20  (CHILD_ID_CLIMA_MIN/MAX)
//   Hidrometria 21-30 (CHILD_ID_FLOW_MIN/MAX)
//   Atuação    31-40  (CHILD_ID_ACTUATOR_MIN/MAX)

// ----- Clima (11-20) — só no Nano: a M360-ACT não tem DHT -----
#ifndef NODE99_BOARD_ACT
#define CHILD_ID_DHT_TEMP   11  // Temperatura do Ar (DHT11, D2)
#define CHILD_ID_DHT_HUM    12  // Umidade do Ar (DHT11, D2)
#endif

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

// ===== PINO NATIVO DHT11 (só no Nano) =====
#ifndef NODE99_BOARD_ACT
#define PIN_DHT             2   // D2 para sinal digital do DHT11
#endif

// ===== SENSOR DE VAZÃO YF-S201 =====
// D3 é o único pino de interrupção externa livre no Nano (INT1):
// D2 = DHT11, D4-D8 = MUX, D9-D13 = NRF24, A0/A1 = bombas.
// Na M360-ACT o D2 é o DIO0 do rádio e o D3 é a entrada de vazão da placa
// (rede FLOW_DIV): mesmo pino, mas em INPUT puro — ver initFlowSensor().
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
 * Na M360-ACT, libera o /OE do 74HCT244 (A5) como último passo.
 */
void initSensors();

/**
 * Escreve estado em atuador. O parâmetro `pin` pode ser:
 *   - Pino virtual MUX:  MUX_CHANNEL_OFFSET + canal (ex: 104 para canal 4)
 *   - Pino físico nativo: PIN_NFT_PUMP (A0) ou PIN_NFT_OXI (A1)
 * Impõe restrição de concorrência MUX automaticamente.
 *
 * @return canal MUX que foi desligado automaticamente para respeitar a
 *         restrição de concorrência, ou -1 se nenhum. O chamador precisa
 *         desse retorno para reportar o child preemptado ao gateway: sem
 *         isso, o Node-RED continua mostrando aquele relé como ligado.
 *         Sempre -1 para pinos nativos e para qualquer desligamento.
 */
int8_t writeNodeItem(uint8_t pin, bool state);

/**
 * Lê estado lógico do atuador.
 * Retorna 1.0f = ligado, 0.0f = desligado.
 * Para canais MUX, retorna o estado rastreado em software.
 */
float readNodeItem(uint8_t pin);

#ifndef NODE99_BOARD_ACT
/**
 * Lê a temperatura do sensor DHT11 (só no Nano).
 */
float readDHTTemp();

/**
 * Lê a umidade do sensor DHT11 (só no Nano).
 */
float readDHTHum();
#endif

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
