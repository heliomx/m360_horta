/*
 * lpDrivers.h — Interface da camada física da M360-LP
 *
 * lpNode.cpp só conhece estas funções. Cada driver em drivers/ é compilado
 * apenas se a flag correspondente (LP_HAS_SOIL, LP_HAS_DHT, LP_HAS_ONEWIRE)
 * estiver definida no nodeConfig.h do nó — o arquivo inteiro fica vazio sem ela.
 */

#pragma once
#include <Arduino.h>
#include "lpPins.h"

// ===== ORQUESTRAÇÃO (lpDrivers.cpp) =====
void  initSensors();        // before(): pinos em estado seguro, VSW desligado
void  powerUpSensors();     // liga o VSW e prepara os sensores presentes; idempotente
void  powerDownSensors();   // S0–S2 em LOW, campo sem nível alto, VSW desligado; idempotente
float lpReadSource(int src); // despacha pelo código LP_SRC_* do campo `pin`; NAN se ausente

// ===== drivers/soilMux.cpp =====
#if LP_HAS_SOIL
void  soilMuxInit();
void  soilMuxIdle();                 // S0–S2 em LOW — obrigatório antes de cortar o VSW
float soilMuxRead(uint8_t channel);  // ADC bruto 0–1023 (alto = seco)
#endif

// ===== drivers/dht.cpp =====
#if LP_HAS_DHT
void  dhtPowerUp();    // reinicia o barramento depois de o VSW subir
void  dhtPowerDown();  // A2 em INPUT sem pull-up: pino alto alimentaria o sensor
float dhtReadTemp();
float dhtReadHum();
#endif

// ===== drivers/onewire.cpp =====
#if LP_HAS_ONEWIRE
void  ds18PowerUp();   // begin() + requestTemperatures()
void  ds18PowerDown(); // A1 em INPUT
float ds18Read();
#endif

// ===== drivers/battery.cpp (só LP_POWER_BATTERY) =====
#if LP_POWER_BATTERY
float lpReadVbat();    // A6 × 3 × VCC(bandgap)/1023, com calibração do nodeConfig.h
#endif
