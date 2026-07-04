/*
 * sensorDrivers.h — Drivers de Sensores do Nó 80 (80nodeAqua)
 *
 * Este arquivo define a pinagem de hardware e declara as funções do driver de
 * sensores.
 */

#pragma once
#include <Arduino.h>

// ===== CONFIGURAÇÃO DE PINOS =====
#define PIN_POWER_SENSORS 7     // D7: Chaveamento VCC dos sensores
#define PIN_FLOW_SENSOR   2     // D2: Sinal do sensor de vazão 1 (INT0)
#define PIN_FLOW_SENSOR_A 3     // D3: Sinal do sensor de vazão A (INT1)
#define PIN_FLOW_SENSOR_B 8     // D8: Sinal do sensor de vazão B (PCINT0)
#define PIN_FLOW_SENSOR_C A2    // A2: Sinal do sensor de vazão C (PCINT10)

// Ultrassônico
#define PIN_TRIG 4 // D4: Trigger
#define PIN_ECHO 5 // D5: Echo

// Analógicos
#define PIN_PH A0 // A0: Sensor de pH
#define PIN_EC A1 // A1: Sensor de EC

// OneWire / DS18B20
#define PIN_ONEWIRE 6 // D6: Dados OneWire (DS18B20)

// ===== DRIVER INTERFACE =====

void initSensors();
void powerUpSensors();
void powerDownSensors();
float readNodeItem(uint8_t itemIndex);
void writeNodeItem(uint8_t childId, bool state);

// Interrupção do sensor de vazão
void pulseCounter();

// Leituras de vazão e flags compartilhadas
extern float currentFlowH;
extern float currentFlowA;
extern float currentFlowB;
extern float currentFlowC;
void updateFlows();

extern bool allowAquaRead;

