/*
 * Sensor Drivers Header - Solenoide e Vazão Node (m360_horta)
 * 
 * Este nó monitora:
 * - 2 Sensores de Fluxo (YF-S201) nos pinos D2 e D3
 * - 2 Válvulas Solenoides nos pinos D4 e D5
 */

#pragma once
#include <Arduino.h>
#include "../shared/config.h"
#include "../shared/node_engine.h"

// ===== CONFIGURAÇÃO DO NÓ =====
#ifndef MY_NODE_ID
#define MY_NODE_ID 10
#endif

// ===== PINAGEM =====
#define FLOW_SENSOR_1_PIN 2
#define FLOW_SENSOR_2_PIN 3
#define SOLENOID_1_PIN 4
#define SOLENOID_2_PIN 5

// Child IDs
#define CHILD_ID_FLOW_1 0
#define CHILD_ID_FLOW_2 1
#define CHILD_ID_SOL_1 2
#define CHILD_ID_SOL_2 3

// ===== FUNÇÕES OBRIGATÓRIAS =====
void initSensors();
float readNodeItem(uint8_t nodeIndex);
void writeNodeItem(uint8_t childId, bool state);

// ===== FUNÇÕES DE FLUXO =====
void IRAM_ATTR pulseCounter1();
void IRAM_ATTR pulseCounter2();
float getFlowRate(uint8_t sensorNum);
float getVolume(uint8_t sensorNum);
void resetVolume(uint8_t sensorNum);
