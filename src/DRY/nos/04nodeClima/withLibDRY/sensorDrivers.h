#ifndef SENSOR_DRIVERS_H
#define SENSOR_DRIVERS_H

#include <Arduino.h>
#include <DHT.h>

// Controle de Energia (VCC)
#define PIN_POWER_SENSORS 3  // D3

// DHT22 (Temperatura e Umidade)
#define DHTPIN 4
#define DHTTYPE DHT22

// LDR (Luminosidade)
#define PIN_LDR A0

/**
 * Inicializa pinos em estado de repouso
 */
void initSensors();

/**
 * Energiza DHT e LDR e aguarda o tempo de startup (DHT)
 */
void powerUpSensors();

/**
 * Desenergiza DHT e LDR (LOW em VCC e pinos de sinal)
 */
void powerDownSensors();

/**
 * Lê o sensor mapeado (0=Temp, 1=Umid, 2=Luz)
 */
float readNodeItem(uint8_t itemIndex);

#endif // SENSOR_DRIVERS_H
