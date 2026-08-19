/*
 * sensorDrivers.h — Definições de hardware e interface do driver para 04noodeSolarMini (Nó 4)
 */

#pragma once
#include <Arduino.h>

// ===== PINOS DE HARDWARE =====
#define PIN_POWER_SENSORS  3  // Pino VCC controlado para economizar bateria
#define PIN_DHT            4  // Pino de dados do DHT11
#define PIN_ONEWIRE        5  // Pino de dados OneWire do DS18B20 (Temperatura Solo)
#define PIN_BATTERY_ADC    A0 // Divisor de tensão da bateria (100k/100k)

// ===== CHILD IDs (Canais do Nó conforme R13) =====
#define CHILD_ID_TEMP      11 // Temperatura do Ar (DHT11 - Clima 11–20)
#define CHILD_ID_HUM       12 // Umidade Relativa do Ar (DHT11 - Clima 11–20)
#define CHILD_ID_SOIL_TEMP 1  // Temperatura do Solo (DS18B20 - Solo 1–10)


// ===== INTERFACE DO DRIVER =====
void initSensors();
void powerUpSensors();
void powerDownSensors();
float readNodeItem(uint8_t nodeIndex);
