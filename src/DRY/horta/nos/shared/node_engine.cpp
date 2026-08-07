/*
 * Node Engine - Funções Auxiliares para Nós MySensors
 * 
 * Implementa apenas as funções auxiliares que não dependem diretamente
 * do MySensors, evitando conflitos de múltiplas definições.
 */

#include "node_engine.h"
#include <EEPROM.h>

// ===== IMPLEMENTAÇÃO DAS FUNÇÕES AUXILIARES =====

// ===== FUNÇÕES AUXILIARES =====

void nodeEngine_powerDown() {
  if (IS_POWER_MANAGEMENT_ENABLED()) {
    powerDownPeripherals();
  }
}

void nodeEngine_powerUp() {
  if (IS_POWER_MANAGEMENT_ENABLED()) {
    powerUpPeripherals();
  }
}

// Leitura interna do ADC (referência 1.1V) — apenas AVR
static uint16_t readADCInternal() {
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2);
    ADCSRA |= _BV(ADSC);
    while (bit_is_set(ADCSRA, ADSC));
    return (ADCH << 8) | ADCL;
  #else
    return 0;  // não suportado em plataformas não-AVR
  #endif
}

float nodeEngine_readBattery() {
  uint16_t result = readADCInternal();
  if (result == 0) return 0.0;
  // 1.1 * 1023 * 1000 = 1125300
  return (1125300L / result) / 1000.0;
}

uint8_t nodeEngine_voltageToPercent(float voltage) {
  if (voltage >= MAX_VOLTAGE)
    return 100;
  if (voltage <= MIN_VOLTAGE)
    return 0;

  // Conversão linear
  float percent = ((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0;

  return (uint8_t)percent;
}

uint8_t nodeEngine_readBatteryPercent() {
  return nodeEngine_voltageToPercent(nodeEngine_readBattery());
}

void nodeEngine_saveInterval(uint16_t interval) {
  uint16_t stored;
  EEPROM.get(EEPROM_INTERVAL_ADDRESS, stored);
  
  // Só escreve se o valor for diferente, para economizar ciclos de escrita da EEPROM
  if (stored != interval) {
    EEPROM.put(EEPROM_INTERVAL_ADDRESS, interval);
    
    #ifdef MY_DEBUG
    Serial.print("Salvo na EEPROM: ");
    Serial.print(interval);
    Serial.println(" min");
    #endif
  }
}

uint16_t nodeEngine_loadInterval() {
  uint16_t interval;
  EEPROM.get(EEPROM_INTERVAL_ADDRESS, interval);
  
  // Verifica se o valor está dentro dos limites permitidos
  if (interval >= MIN_INTERVAL && interval <= MAX_INTERVAL) {
    #ifdef MY_DEBUG
    Serial.print("Carregado da EEPROM: ");
    Serial.print(interval);
    Serial.println(" min");
    #endif
    return interval;
  }
  
  // Se o valor for inválido, persiste e usa o padrão
  #ifdef MY_DEBUG
  Serial.print("Valor inválido na EEPROM, usando padrão: ");
  Serial.print(DEFAULT_INTERVAL);
  Serial.println(" min");
  #endif
  nodeEngine_saveInterval(DEFAULT_INTERVAL);
  return DEFAULT_INTERVAL;
}

void nodeEngine_initArrays(float* lastValues, uint8_t* nNoUpdates, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    lastValues[i]  = NAN;
    nNoUpdates[i]  = 0;
  }
}

void nodeEngine_setupPins() {
  for (uint8_t i = 0; i < NODE_ITEMS_COUNT; i++) {
    if (NODE_ITEMS[i].pin < 0) continue;
    if (NODE_ITEMS[i].kind == NODE_ITEM_ACTUATOR) {
      pinMode(NODE_ITEMS[i].pin, OUTPUT);
      digitalWrite(NODE_ITEMS[i].pin, LOW);
    } else {
      pinMode(NODE_ITEMS[i].pin, INPUT);
    }
  }
}

// Funções que dependem do MySensors foram movidas para o arquivo principal
