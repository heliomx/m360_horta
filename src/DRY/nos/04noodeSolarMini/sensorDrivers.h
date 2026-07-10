/*
 * sensorDrivers.h — Definições de hardware e interface do driver para 04noodeSolarMini (Nó 4)
 */

#pragma once
#include <Arduino.h>

// ===== PINOS DE HARDWARE =====
#define PIN_POWER_SENSORS  3  // Pino VCC controlado para economizar bateria
#define PIN_DHT            4  // Pino de dados do DHT11

// ===== CHILD IDs (Canais do Nó) =====
#define CHILD_ID_TEMP      0  // Temperatura
#define CHILD_ID_HUM       1  // Umidade Relativa

// ===== INTERFACE DO DRIVER =====
void initSensors();
void powerUpSensors();
void powerDownSensors();
float readNodeItem(uint8_t nodeIndex);
