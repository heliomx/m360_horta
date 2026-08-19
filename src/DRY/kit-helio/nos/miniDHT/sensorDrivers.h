/*
 * sensorDrivers.h — Interface de Drivers de Sensores do miniDHT (Kit Hélio)
 *
 * M360 Horta — Kit Hélio
 * Nó: miniDHT (ID 11)
 * Hardware: Arduino Pro Mini 5V/16MHz + DHT11
 */

#pragma once
#include <Arduino.h>

// ===== PINOS DE HARDWARE =====
#define PIN_DHT 4 // D4 para sinal de dados do DHT11

// ===== CHILD IDs (Normativos R13: 11-20 Clima) =====
#define CHILD_ID_DHT_TEMP 11 // Temperatura Ar (S_TEMP / V_TEMP)
#define CHILD_ID_DHT_HUM  12 // Umidade Relativa (S_HUM / V_HUM)

// ===== DRIVER INTERFACE =====

/**
 * Inicializa a biblioteca do DHT11
 */
void initSensors();

/**
 * Lê a temperatura do DHT11
 */
float readDHTTemp();

/**
 * Lê a umidade do DHT11
 */
float readDHTHum();

/**
 * Lê o sensor mapeado pelo índice do item no NODE_ITEMS[]
 * @param nodeIndex Índice no array NODE_ITEMS[] (0=Temp, 1=Hum)
 */
float readNodeItem(uint8_t nodeIndex);
