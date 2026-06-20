/*
 * sensorDrivers.h — Drivers de Sensores do Nó 80 (80nodeAqua)
 * 
 * Este arquivo define a pinagem de hardware e declara as funções do driver de sensores.
 */

#pragma once
#include <Arduino.h>

// ===== CONFIGURAÇÃO DE PINOS =====
#define PIN_POWER_SENSORS  3  // D3: Chaveamento VCC dos sensores
#define PIN_FLOW_SENSOR    2  // D2: Sinal do sensor de vazão (INT0)

// Ultrassônico
#define PIN_TRIG           4  // D4: Trigger
#define PIN_ECHO           5  // D5: Echo

// Analógicos
#define PIN_PH             A0 // A0: Sensor de pH
#define PIN_EC             A1 // A1: Sensor de EC

// OneWire / DS18B20
#define PIN_ONEWIRE        6  // D6: Dados OneWire (DS18B20)

// ===== DRIVER INTERFACE =====

void initSensors();
void powerUpSensors();
void powerDownSensors();
float readNodeItem(uint8_t itemIndex);
void writeNodeItem(uint8_t childId, bool state);

// Interrupção do sensor de vazão
void pulseCounter();
