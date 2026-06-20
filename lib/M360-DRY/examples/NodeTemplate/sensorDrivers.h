/*
 * sensorDrivers.h — Interface de Drivers de Sensores do NodeTemplate M360
 * 
 * Este arquivo define a pinagem e as interfaces de hardware do nó.
 */

#pragma once
#include <Arduino.h>

// ===== PINAGEM RECOMENDADA =====
#define PIN_POWER_SENSORS 3 // D3 para controle de VCC (opcional)
#define PIN_TEMP_HUM      4 // D4 para DHT (exemplo)
#define PIN_RELAY         5 // D5 para Relé (exemplo)

// ===== DRIVER INTERFACE =====

/**
 * Inicializa pinos em estado de repouso
 */
void initSensors();

/**
 * Energiza sensores e aguarda tempo de estabilização pós-boot
 */
void powerUpSensors();

/**
 * Desenergiza sensores para economia profunda de energia
 */
void powerDownSensors();

/**
 * Lê o sensor mapeado pelo índice do item no NODE_ITEMS[]
 */
float readNodeItem(uint8_t itemIndex);

/**
 * Controla os atuadores com base no childId do MySensors
 */
void writeNodeItem(uint8_t childId, bool state);
