/*
 * nodeConfig.h — Nó 2 na M360-LP: Canteiro B, 6 sondas de solo, fonte fixa 5 V
 *
 * Incluído por lpNode.cpp e pelos drivers via -Isrc/DRY/horta/nos/lp/config/node02
 * (env lp_02). Contrato com o Node-RED: inventario.md §5 — child IDs e rótulos
 * são os mesmos do 02nodeSolo3dNano.cpp (Nano), só o hardware mudou.
 *
 * Porta J1 SOLO (ARQUITETURA_FAMILIA.md §7): ch0/ch1 estaca de 1 m (10/30 cm),
 * ch2/ch3 estaca de 3 m, ch4/ch5 estaca de 5 m.
 */

#pragma once

#define LP_NODE_NAME     "LP-Solo3dB"
#define LP_NODE_VERSION  "3.0.0"

// ===== ALIMENTAÇÃO (exatamente uma) =====
#define LP_POWER_FIXED_5V 1      // 5 V em J_VIN → M360_ALWAYS_ON
// #define LP_POWER_BATTERY 1    // TP4056+DW01 em J_VIN → M360_LOW_POWER + bateria pelo A6

// ===== DRIVERS COMPILADOS =====
#define LP_HAS_SOIL     1
#define LP_HAS_DHT      0
#define LP_HAS_ONEWIRE  0

// ===== ITENS (só dentro de lpNode.cpp estes símbolos existem) =====
#ifdef LP_NODE_ITEMS_SCOPE
static const char L_1M_10CM[] PROGMEM = "B_1m_10cm";
static const char L_1M_30CM[] PROGMEM = "B_1m_30cm";
static const char L_3M_10CM[] PROGMEM = "B_3m_10cm";
static const char L_3M_30CM[] PROGMEM = "B_3m_30cm";
static const char L_5M_10CM[] PROGMEM = "B_5m_10cm";
static const char L_5M_30CM[] PROGMEM = "B_5m_30cm";

// childId | kind | S_* | V_* | pin = fonte LP_SRC_* | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ 1, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(0), 0, 1, M360_LABEL(L_1M_10CM), false, 0 },
	{ 2, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(1), 0, 1, M360_LABEL(L_1M_30CM), false, 0 },
	{ 3, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(2), 0, 1, M360_LABEL(L_3M_10CM), false, 0 },
	{ 4, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(3), 0, 1, M360_LABEL(L_3M_30CM), false, 0 },
	{ 5, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(4), 0, 1, M360_LABEL(L_5M_10CM), false, 0 },
	{ 6, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(5), 0, 1, M360_LABEL(L_5M_30CM), false, 0 },
};
#endif // LP_NODE_ITEMS_SCOPE
