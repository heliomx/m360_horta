/*
 * lpPins.h — Pinagem fixa da placa M360-LP (Pro Mini 3,3 V / 8 MHz)
 *
 * Fonte: hardware/m360-lp/M360-LP.kicad_sch (netlist conferida em 2026-10-14)
 * e hardware/ARQUITETURA_FAMILIA.md §4.3. Nada aqui varia entre nós: o que
 * varia (sensores presentes, rótulos, child IDs, alimentação) fica em
 * config/<nó>/nodeConfig.h.
 *
 * Pinos do rádio (DIO0=D2, DIO1=D9, NSS=D10, RESET=D8; nRF24 legado CE=D2,
 * CSN=D10) são do MySensors e entram pelo platformio.ini ([lp_base]) — R1.
 */

#pragma once
#include <Arduino.h>

// ===== TRILHO CHAVEADO VSW (3,3 V dos sensores, pull-ups e par marrom) =====
// P-MOSFET high-side (Q2) com gate em 100 kΩ para o 3V3: LOW liga, HIGH desliga.
// Desligado no boot por hardware; o firmware só confirma.
#define LP_PIN_VSW_EN      4
#define LP_VSW_ON          LOW
#define LP_VSW_OFF         HIGH

// ===== 74HC4051 (8 canais de solo em A0) =====
#define LP_PIN_MUX_S0      5
#define LP_PIN_MUX_S1      6
#define LP_PIN_MUX_S2      7
#define LP_PIN_MUX_OUT     A0

// ===== DIGITAIS DE CAMPO (porta J2 CLIMA/AUX) =====
#define LP_PIN_ONEWIRE     A1   // DS18B20, pull-up 4,7 kΩ para o VSW na placa
#define LP_PIN_DHT         A2   // DHT11/22, pull-up 10 kΩ para o VSW na placa
// A4/A5 = SDA/SCL (pull-ups 3,3 kΩ para o VSW). Sem driver ainda — ver README.

// ===== OUTROS =====
#define LP_PIN_REED        3    // reed switch SW1 para GND (INT1) — nunca saída
#define LP_PIN_VBAT        A6   // divisor 2 MΩ / 1 MΩ permanente + 100 nF
#define LP_VBAT_DIVIDER    3.0f // VBAT = 3 × V(A6)

// ===== CÓDIGOS DE FONTE NO CAMPO `pin` DE M360ItemDef =====
// Mesma convenção de pino virtual do Nó 99 (R7): o motor não faz pinMode em
// nada — setupPins() é omitido — e lpReadSource() despacha pelo código.
#define LP_SRC_SOIL_BASE   100               // canal 0..7 do 74HC4051
#define LP_SRC_SOIL(ch)    (LP_SRC_SOIL_BASE + (ch))
#define LP_IS_SOIL(p)      ((p) >= LP_SRC_SOIL_BASE && (p) < LP_SRC_SOIL_BASE + 8)
#define LP_SOIL_CH(p)      ((uint8_t)((p) - LP_SRC_SOIL_BASE))
#define LP_SRC_DHT_TEMP    120
#define LP_SRC_DHT_HUM     121
#define LP_SRC_DS18B20     130
