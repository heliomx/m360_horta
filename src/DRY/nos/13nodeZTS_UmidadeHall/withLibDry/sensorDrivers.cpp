#include "sensorDrivers.h"
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// ===== GLOBALS =====
SoftwareSerial rs485(MAX485_RO_RX, MAX485_DI_TX);
ModbusMaster modbus;

// Modbus Result Cache
float ztsMoisture = 0;
float ztsTemp = 0;
float ztsCond = 0;
float ztsPH = 0;
uint16_t ztsN = 0, ztsP = 0, ztsK = 0;

// ===== HELPER FUNCTIONS =====

void preTransmission() {
  digitalWrite(MAX485_RE_NEG, HIGH);
  digitalWrite(MAX485_DE, HIGH);
  delay(2); // Pequeno atraso para estabilização (at least 3.5 char time)
}

void postTransmission() {
  rs485.flush();
  // 3.5 char silence @ 9600 (~4ms). 5ms é seguro.
  delay(5); 
  digitalWrite(MAX485_RE_NEG, LOW);
  digitalWrite(MAX485_DE, LOW);
}

// ===== DRIVER IMPLEMENTATION =====

void initDrivers() {
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIN_HALL, INPUT);
  pinMode(PIN_LDR, INPUT);

  // Default states
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE_NEG, LOW);
  writeSolenoid(false);

  // RS485 Init
  rs485.begin(RS485_BAUD_RATE);
  rs485.setTimeout(MODBUS_TIMEOUT);
  modbus.begin(1, rs485);
  modbus.preTransmission(preTransmission);
  modbus.postTransmission(postTransmission);
}

void powerUpPeripherals() {
  // Sensores alimentados por barramento fixo ou relé já tratado
}

void powerDownPeripherals() {
  // Logic to save power
}

bool readZTSData() {
  uint8_t result;
  uint8_t retries = 3;
  
  while (retries > 0) {
    result = modbus.readHoldingRegisters(0x0000, 7);
    if (result == 0x00) {
      ztsMoisture = modbus.getResponseBuffer(0) / 10.0;
      ztsTemp = (int16_t)modbus.getResponseBuffer(1) / 10.0;
      ztsCond = modbus.getResponseBuffer(2) / 100.0;
      ztsPH = modbus.getResponseBuffer(3) / 100.0;
      ztsN = modbus.getResponseBuffer(4);
      ztsP = modbus.getResponseBuffer(5);
      ztsK = modbus.getResponseBuffer(6);
      return true;
    }
    retries--;
    if (retries > 0) delay(100);  // driver file não pode incluir MySensors.h (monolítico)
  }
  return false;
}

float readNodeItem(uint8_t index) {
  // Mapeamento baseado na ordem em NODE_ITEMS no arquivo principal
  // 0-6: ZTS, 7: Selenoide (Atuador), 8: Hall
  
  if (index == 0) {
    readZTSData();
  }

  switch (index) {
    case 0: return ztsMoisture;
    case 1: return ztsTemp;
    case 2: return ztsCond;
    case 3: return ztsPH;
    case 4: return (float)ztsN;
    case 5: return (float)ztsP;
    case 6: return (float)ztsK;
    case 8: return readHallHumidity();
    case 9: return readLDR();
    default: return NAN;
  }
}

void writeNodeItem(uint8_t index, bool state) {
  if (index == 7) { // Selenoide
    writeSolenoid(state);
  }
}

float readHallHumidity() {
  long measure = 0;
  for (int i = 0; i < 10; i++) {
    measure += analogRead(PIN_HALL);
    delay(5);
  }
  float avgValue = measure / 10.0;
  
  // Converte para mV (Vref = 3.3V)
  float outputV = avgValue * 3300.0 / 1023.0;
  
  // Diferença em relação ao offset (1.65V)
  float deltaV = (outputV - 1650.0) / 1000.0;
  
  // Sensibilidade 14mV/mT
  float magneticFlux = deltaV / 0.014;
  
  return magneticFlux;
}

float readLDR() {
  long measure = 0;
  for (int i = 0; i < 10; i++) {
    measure += analogRead(PIN_LDR);
    delay(5);
  }
  float avgValue = measure / 10.0;
  
  // Converte para porcentagem (0 a 100%) - pode precisar de ajuste dependente do divisor resistivo
  return (avgValue / 1023.0) * 100.0;
}

void writeSolenoid(bool state) {
  digitalWrite(RELAY_PIN, state ? RELAY_ON : RELAY_OFF);
}
