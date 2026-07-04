#ifndef SENSOR_DRIVERS_H
#define SENSOR_DRIVERS_H

#include <Arduino.h>

// Pinos do Multiplexador CD74HC4067
#define MUX_PIN_SIG A0
#define MUX_PIN_S0  4
#define MUX_PIN_S1  5
#define MUX_PIN_S2  6
#define MUX_PIN_S3  7

// Pino de Energia dos Pull-ups (Mitigação de Eletrólise)
#define PIN_POWER_SENSORS 3 // D3

// Pinos Nativos
#define PIN_NATIVE_A1 A1
#define PIN_NATIVE_A2 A2

/**
 * Configura os pinos digitais de controle (MUX e Power) como OUTPUT, e pinos analógicos como INPUT.
 */
void initSensors();

/**
 * Liga a alimentação de pull-up dos sensores (mitigação de eletrólise) e aguarda estabilização.
 */
void powerUpSensors();

/**
 * Desliga a alimentação de pull-up dos sensores.
 */
void powerDownSensors();

/**
 * Lê o sensor resistivo específico.
 * Mapeia 1023..0 para 0..100%.
 * @param itemIndex O índice do sensor (0..15 = MUX, 16 = A1, 17 = A2)
 * @return Umidade do solo estimada (0 a 100.0)
 */
float readNodeItem(uint8_t itemIndex);

#endif // SENSOR_DRIVERS_H
