/*
 * Sensor Drivers Implementation - nodeSelenoideVazao
 * 
 * Implementação robusta das leituras de:
 * 1. Sensores de Fluxo YF-S201 (D2 e D3) - Com proteção de atomicidade (ATmega328P)
 * 2. Controle de Válvulas Solenoides (D4 e D5)
 */

#include "sensorDrivers.h"

// ===== VARIÁVEIS DE ESTADO (VOLATILE PARA ISR) =====
volatile uint32_t pulseCount1 = 0;
volatile uint32_t pulseCount2 = 0;

// Variáveis de histórico de cálculo (Uma para cada sensor para evitar acoplamento)
uint32_t lastPulses[2] = {0, 0};
unsigned long lastFlowTime[2] = {0, 0};

// Constante de calibração para YF-S201 (Freq = 7.5 * Q)
const float calibrationFactor = 7.5; 

// ===== INTERRUPÇÕES =====
void pulseCounter1() {
  pulseCount1++;
}

void pulseCounter2() {
  pulseCount2++;
}

/**
 * @brief Lê o contador de pulsos de forma atômica (Protege contra corrupção em MCU 8-bit)
 */
uint32_t getPulseCountSafe(uint8_t sensorIndex) {
    uint32_t c;
    noInterrupts();
    c = (sensorIndex == 0) ? pulseCount1 : pulseCount2;
    interrupts();
    return c;
}

// ===== INICIALIZAÇÃO =====
void initSensors() {
  Serial.println(F("[DRV] Inicializando Vazão e Solenoides (8-bit Atomic Mode)..."));
  
  // Configura Vazão
  pinMode(FLOW_SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2_PIN, INPUT_PULLUP);
  
  // Ativa interrupções
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1_PIN), pulseCounter1, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2_PIN), pulseCounter2, FALLING);
  
  // Configura Solenoides
  pinMode(SOLENOID_1_PIN, OUTPUT);
  pinMode(SOLENOID_2_PIN, OUTPUT);
  digitalWrite(SOLENOID_1_PIN, LOW);
  digitalWrite(SOLENOID_2_PIN, LOW);
  
  unsigned long now = millis();
  lastFlowTime[0] = now;
  lastFlowTime[1] = now;
  
  Serial.println(F("[DRV] Sensores e Atuadores prontos."));
}

// ===== CÁLCULOS =====

float getFlowRate(uint8_t sensorIndex) {
  if (sensorIndex > 1) return 0.0;

  unsigned long currentTime = millis();
  unsigned long duration = (currentTime - lastFlowTime[sensorIndex]);
  
  // Exige pelo menos 1 segundo entre cálculos de vazão para manter a precisão do calibrationFactor
  if (duration < 1000) return 0.0; 

  uint32_t currentPulses = getPulseCountSafe(sensorIndex);
  uint32_t pulses = currentPulses - lastPulses[sensorIndex];
  
  // Vazão em L/min = ((pulses / duration_ms) * 1000) / calibrationFactor
  float flowRate = ((1000.0 * pulses) / duration) / calibrationFactor;

  // Atualiza histórico INDIVIDUAL para este sensor
  lastFlowTime[sensorIndex] = currentTime;
  lastPulses[sensorIndex] = currentPulses;

  return flowRate;
}

float getVolume(uint8_t sensorIndex) {
  if (sensorIndex > 1) return 0.0;
  
  uint32_t pulses = getPulseCountSafe(sensorIndex);
  // 1 Litro = calibrationFactor * 60 segundos = 7.5 * 60 = 450 pulsos
  return (float)pulses / 450.0;
}

void resetVolume(uint8_t sensorIndex) {
    if (sensorIndex > 1) return;
    
    noInterrupts();
    if (sensorIndex == 0) pulseCount1 = 0;
    else pulseCount2 = 0;
    interrupts();
    
    lastPulses[sensorIndex] = 0;
    Serial.print(F("[DRV] Volume Reset Sensor ")); Serial.println(sensorIndex + 1);
}

// ===== DISPATCHER =====

float readNodeItem(uint8_t nodeIndex) {
  // Conforme NODE_ITEMS em noSelenoideVazao.cpp
  switch (nodeIndex) {
    case 0: return getFlowRate(0); // CHILD_ID_FLOW_1
    case 1: return getFlowRate(1); // CHILD_ID_FLOW_2
    case 2: return digitalRead(SOLENOID_1_PIN); // CHILD_ID_SOL_1
    case 3: return digitalRead(SOLENOID_2_PIN); // CHILD_ID_SOL_2
    case 4: return getVolume(0);   // Volume 1
    case 5: return getVolume(1);   // Volume 2
    default: return NAN;
  }
}

void writeNodeItem(uint8_t childId, bool state) {
    if (childId == CHILD_ID_SOL_1) {
        digitalWrite(SOLENOID_1_PIN, state ? HIGH : LOW);
        Serial.print(F("[DRV] Sol 1 -> ")); Serial.println(state ? F("ABERTO") : F("FECHADO"));
    } else if (childId == CHILD_ID_SOL_2) {
        digitalWrite(SOLENOID_2_PIN, state ? HIGH : LOW);
        Serial.print(F("[DRV] Sol 2 -> ")); Serial.println(state ? F("ABERTO") : F("FECHADO"));
    }
}
