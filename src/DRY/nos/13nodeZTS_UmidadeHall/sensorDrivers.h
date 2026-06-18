#pragma once
#include <Arduino.h>

// ===== CONFIGURAÇÃO DE PINOS (DERIVADO DO ESQUEMA ELÉTRICO) =====
#define MAX485_DE       7     // Driver Enable
#define MAX485_RE_NEG   8     // Receiver Enable (N)
#define MAX485_RO_RX    9     // RO -> Arduino RX
#define MAX485_DI_TX    10    // DI -> Arduino TX
#define RELAY_PIN       5     // Controle Válvula / Alimentação ZTS (Active LOW)
#define PIN_HALL        A0    // Sensor de Umidade Local (Hall)
#define PIN_LDR         A1    // Sensor de Luminosidade (LDR)

// ===== CHILD IDs (PADRÃO M360) =====
#define CHILD_ID_MOISTURE      0
#define CHILD_ID_TEMP          1
#define CHILD_ID_CONDUCTIVITY  2
#define CHILD_ID_PH            3
#define CHILD_ID_NITROGEN      4
#define CHILD_ID_PHOSPHORUS    5
#define CHILD_ID_POTASSIUM     6
#define CHILD_ID_SELENOIDE     7
#define CHILD_ID_UMIDADE_HALL  8
#define CHILD_ID_LDR           9

// Node ID único para o nó de monitoramento de horta

// ===== CONSTANTES =====
#define RELAY_ON  0
#define RELAY_OFF 1

// ===== RS485 / MODBUS CONFIG =====
#define RS485_BAUD_RATE 9600
#define MODBUS_TIMEOUT  2000 // ms

// ===== DRIVER INTERFACE =====

/**
 * Inicializa os drivers de hardware
 */
void initDrivers();

/**
 * Liga os periféricos controlados (Relé chaveia 12V para o ZTS)
 */
void powerUpPeripherals();

/**
 * Desliga os periféricos
 */
void powerDownPeripherals();

/**
 * Leitura dos parâmetros do sensor ZTS-3002 via RS485/Modbus
 */
bool readZTSData(); // Lê todos os registros de uma vez

// Função para o Node Engine ler sensores
float readNodeItem(uint8_t nodeIndex);

// Função para o Node Engine escrever em atuadores
void writeNodeItem(uint8_t nodeIndex, bool state);

/**
 * Leitura do sensor de umidade local (Hall)
 */
float readHallHumidity();

/**
 * Leitura do sensor de luminosidade (LDR)
 */
float readLDR();

/**
 * Controle direto do relé/selenoide
 */
void writeSolenoid(bool state);
