/*
 * Teste RS485 MAX485 - Arduino Pro Mini
 * 
 * Código de teste para verificar comunicação RS485
 * entre Arduino Pro Mini e módulo MAX485
 * 
 * Alterações aplicadas:
 * - Controle DE/RE com flush() e delay calculado a partir do baud e bytes transmitidos
 * - Tratamento de temperatura como signed int16_t
 * - Comentários sobre terminação/bias, e recomendações sobre SoftwareSerial
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <Wire.h>
 
 // ===== CONFIGURAÇÃO RS485 =====
 //Manter esta pinagem 
 #define MAX485_DE 7     // Driver Enable (DE)
 #define MAX485_RE_NEG 8   // Receiver Enable (RE_N) -> neste hardware RE e DE estão amarrados ao mesmo pino
 #define MAX485_RO_RX 9      // Pino RX do módulo MAX485 (DI -> Arduino RX)
 #define MAX485_DI_TX 10      // Pino TX do módulo MAX485 (RO -> Arduino TX)
 
//configurar serial para Serial1.begin(4800, SERIAL_8N1, 36, 4)
// Configuração da comunicação serial (linha RS-485)
const unsigned long RS485_BAUD = 4800; 
const unsigned long RS485_BAUD_ALT = 9600; // Baud rate alternativo para teste
bool useBaud9600 = false; // Flag para alternar entre 4800 e 9600
SoftwareSerial rs485Serial(MAX485_RO_RX, MAX485_DI_TX);
ModbusMaster nodeModBus;
 
 // Estimativa do número de bytes transmitidos no último frame (usado para calcular delay após flush)
 volatile uint16_t lastTxBytesEstimate = 0;
 
 // ===== VARIÁVEIS GLOBAIS =====
 unsigned long lastTest = 0;
 const unsigned long testInterval = 5000; // 5 segundos entre testes
 uint8_t testCount = 0;
 
 // ===== FUNÇÕES AUXILIARES =====
 
 // Calcula tempo em milissegundos necessário para transmitir N bytes na taxa RS485_BAUD.
 // Usa 11 bits por caractere como margem (start + 8 data + parity/stop ~ 10, usamos 11 para segurança).
 static inline uint16_t calcTxDelayMs(uint16_t bytes) {
   if (RS485_BAUD == 0) return 2;
   // tempo em ms = bytes * bitsPorByte * 1000 / baud
   uint32_t t = (uint32_t)bytes * 11UL * 1000UL / (uint32_t)RS485_BAUD;
   // garantir mínimo de 2 ms
   if (t < 2) t = 2;
   return (uint16_t)t;
 }
 
 // ===== FUNÇÕES RS485 =====
 // Chamado pelo ModbusMaster antes de transmitir (ativa driver)
 void preTransmission()
 {
   // Se RE for ativo baixo (RE_N), escrever HIGH desativa recepção e permite transmitir
   digitalWrite(MAX485_RE_NEG, HIGH); // Desabilita recepção
   digitalWrite(MAX485_DE, HIGH);     // Habilita transmissão
   // Pequeno tempo para permitir que o driver mude de estado (muito rápido pode cortar o início do frame)
   delay(1);
 }
 
 // Chamado pelo ModbusMaster após finalizar envio (desativa driver -> volta a recepção)
 // Implementa flush() e espera calculada para garantir todos os bits na linha antes de liberar RX.
 void postTransmission()
 {
   // Garante que todo o buffer da UART seja transmitido fisicamente
   rs485Serial.flush();
 
   // Aguardar o tempo calculado baseado na estimativa de bytes transmitidos
   uint16_t waitMs = calcTxDelayMs(lastTxBytesEstimate);
   // adicionar margem extra de 1-2 ms
   waitMs += 2;
   delay(waitMs);
 
   // Habilita recepção e desabilita transmissão
   digitalWrite(MAX485_RE_NEG, LOW); // Habilita recepção
   digitalWrite(MAX485_DE, LOW);     // Desabilita transmissão
   // Pequeno delay para estabilizar antes de iniciar leitura
   delay(1);
 }
 
 void printModbusError(uint8_t errNum)
 {
   Serial.print(F("Erro Modbus: "));
   switch (errNum)
   {
   case 0x00: // ku8MBSuccess
     Serial.println(F("Sucesso"));
     break;
   case 0x01: // ku8MBIllegalFunction
     Serial.println(F("Função Ilegal"));
     break;
   case 0x02: // ku8MBIllegalDataAddress
     Serial.println(F("Endereço de Dados Ilegal"));
     break;
   case 0x03: // ku8MBIllegalDataValue
     Serial.println(F("Valor de Dados Ilegal"));
     break;
   case 0x04: // ku8MBSlaveDeviceFailure
     Serial.println(F("Falha no Dispositivo Escravo"));
     break;
   case 0xE0: // ku8MBInvalidSlaveID
     Serial.println(F("ID de Escravo Inválido"));
     break;
   case 0xE1: // ku8MBInvalidFunction
     Serial.println(F("Função Inválida"));
     break;
   case 0xE2: // ku8MBResponseTimedOut
     Serial.println(F("Timeout na Resposta"));
     break;
   case 0xE3: // ku8MBInvalidCRC
     Serial.println(F("CRC Inválido"));
     break;
   default:
     Serial.print(F("Erro Desconhecido: 0x"));
     Serial.println(errNum, HEX);
     break;
   }
 }
 
 void printSensorData()
 {
   Serial.println(F("\n=== DADOS DO SENSOR ==="));
   
   // Observação: confirme no datasheet o tipo e fator (signed/unsigned, divisor).
   // Aqui aplicamos:
   // - Umidade: raw / 10.0  (float)  -- ajuste caso o datasheet diga /100
   // - Temperatura: signed int16_t -> raw / 10.0 (permite negativos)
   // - Outros campos: raw / 10.0 (ajuste conforme datasheet)
 
   // Umidade do solo (0-100%)
   float moisture = nodeModBus.getResponseBuffer(0) / 10.0;
   Serial.print(F("Umidade: "));
   Serial.print(moisture, 1);
   Serial.println(F(" %"));
 
   // Temperatura do solo (tratada como SIGNED int16)
   int16_t rawTemp = (int16_t) nodeModBus.getResponseBuffer(1);
   float temperature = rawTemp / 10.0;
   Serial.print(F("Temperatura: "));
   Serial.print(temperature, 1);
   Serial.println(F(" °C"));
 
   // Condutividade (mS/cm)
   float conductivity = nodeModBus.getResponseBuffer(2) / 10.0;
   Serial.print(F("Condutividade: "));
   Serial.print(conductivity, 1);
   Serial.println(F(" mS/cm"));
 
   // pH (0-14)
   float ph = nodeModBus.getResponseBuffer(3) / 10.0;
   Serial.print(F("pH: "));
   Serial.print(ph, 1);
   Serial.println();
 
   // Nitrogênio (mg/kg)
   float nitrogen = nodeModBus.getResponseBuffer(4) / 10.0;
   Serial.print(F("Nitrogênio: "));
   Serial.print(nitrogen, 1);
   Serial.println(F(" mg/kg"));
 
   // Potássio (mg/kg)
   float potassium = nodeModBus.getResponseBuffer(5) / 10.0;
   Serial.print(F("Potássio: "));
   Serial.print(potassium, 1);
   Serial.println(F(" mg/kg"));
 
   // Fósforo (mg/kg)
   float phosphorus = nodeModBus.getResponseBuffer(6) / 10.0;
   Serial.print(F("Fósforo: "));
   Serial.print(phosphorus, 1);
   Serial.println(F(" mg/kg"));
   
   Serial.println(F("======================\n"));
 }

// Leitura individual de Nitrogênio (registro 0x001E)
bool readNitrogen(uint16_t &value) {
  lastTxBytesEstimate = 8;
  uint8_t result = nodeModBus.readHoldingRegisters(0x1E, 1);
  if (result == 0x00) {
    value = nodeModBus.getResponseBuffer(0);
    return true;
  }
  printModbusError(result);
  return false;
}

// Leitura individual de Fósforo (registro 0x001F)
bool readPhosphorus(uint16_t &value) {
  lastTxBytesEstimate = 8;
  uint8_t result = nodeModBus.readHoldingRegisters(0x1F, 1);
  if (result == 0x00) {
    value = nodeModBus.getResponseBuffer(0);
    return true;
  }
  printModbusError(result);
  return false;
}

// Leitura individual de Potássio (registro 0x0020)
bool readPotassium(uint16_t &value) {
  lastTxBytesEstimate = 8;
  uint8_t result = nodeModBus.readHoldingRegisters(0x20, 1);
  if (result == 0x00) {
    value = nodeModBus.getResponseBuffer(0);
    return true;
  }
  printModbusError(result);
  return false;
}

// Função para testar leitura individual de NPK
void testNPKIndividual() {
  Serial.println(F("\n=== TESTE NPK INDIVIDUAL ==="));
  
  uint16_t nitrogen, phosphorus, potassium;
  
  if (readNitrogen(nitrogen)) {
    Serial.print(F("Nitrogênio: "));
    Serial.print(nitrogen);
    Serial.println(F(" mg/kg"));
  }
  delay(250);
  
  if (readPhosphorus(phosphorus)) {
    Serial.print(F("Fósforo: "));
    Serial.print(phosphorus);
    Serial.println(F(" mg/kg"));
  }
  delay(250);
  
  if (readPotassium(potassium)) {
    Serial.print(F("Potássio: "));
    Serial.print(potassium);
    Serial.println(F(" mg/kg"));
  }
  
  Serial.println(F("============================\n"));
}

 void testModbusCommunication()
 {
   testCount++;
   Serial.print(F("Teste #"));
   Serial.print(testCount);
   Serial.println(F(" - Iniciando comunicação Modbus..."));
   
   // Estimativa de bytes transmitidos para o pedido de leitura de 7 registradores:
   // Request típico readHoldingRegisters: [ID][FUNC][ADDR_H][ADDR_L][QTY_H][QTY_L][CRC_L][CRC_H] = 8 bytes
   // Ajuste esta estimativa se alterar função/quantidade.
   lastTxBytesEstimate = 8;
 
   // Tentar ler 7 registros a partir do endereço 0x1E (30 decimal)
   uint8_t result = nodeModBus.readHoldingRegisters(0x1E, 7);
   
   if (result == 0x00) // ku8MBSuccess
   {
     Serial.println(F("✓ Comunicação Modbus bem-sucedida!"));
     printSensorData();
   }
   else
   {
     printModbusError(result);
     Serial.println(F("✗ Falha na comunicação Modbus"));
   }
   
   Serial.print(F("Próximo teste em "));
   Serial.print(testInterval / 1000);
   Serial.println(F(" segundos...\n"));
 }
 
 void setup()
 {
   // Inicializar Serial para debug
   Serial.begin(115200);

  

   delay(1000);
   
   Serial.println(F("========================================"));
   Serial.println(F("    TESTE RS485 MAX485 - M360 Network"));
   Serial.println(F("========================================"));
   Serial.println();
   
   // Configurar pinos RS485
   pinMode(MAX485_RE_NEG, OUTPUT);
   pinMode(MAX485_DE, OUTPUT);
   
   // Começar em modo recepção (RE_N low = habilita recepção, DE low = driver desligado)
   digitalWrite(MAX485_RE_NEG, LOW);
   digitalWrite(MAX485_DE, LOW);
   
  // Inicializar comunicação RS485
  rs485Serial.begin(RS485_BAUD);
  nodeModBus.begin(1, rs485Serial); // ID do sensor = 1 (CONFIRME PELO DATASHEET)
   
   // Configurar callbacks de transmissão
   nodeModBus.preTransmission(preTransmission);
   nodeModBus.postTransmission(postTransmission);
   
   Serial.println(F("Configuração RS485 inicializada:"));
   Serial.print(F("  - Baud Rate: "));
   Serial.println(RS485_BAUD);
   Serial.print(F("  - DE/RE Pin: "));
   Serial.println(MAX485_DE);
   
   Serial.println(F("\nConfiguração de Teste:"));
   Serial.println(F("- Modo: Leitura individual NPK + Leitura múltipla registros"));
   Serial.println(F("- Baud rates testados: 4800 e 9600"));
   Serial.println(F("- Pinagem mantida: DE=7, RE=8, RX=9, TX=10"));
   Serial.print(F("  - RO/RX Pin: "));
   Serial.println(MAX485_RO_RX);
   Serial.print(F("  - DI/TX Pin: "));
   Serial.println(MAX485_DI_TX);
   Serial.println();
   Serial.println(F("Aguardando sensor..."));
   Serial.println();
 
   Serial.println(F("Notas importantes (ver PDF original):"));
   Serial.println(F(" - Confirme ID do escravo e baud no datasheet do sensor."));
   Serial.println(F(" - Terminação: use 120 ohm nas extremidades do barramento RS-485 se houver cabo longo."));
   Serial.println(F(" - Polarização (bias): adicione pull-up em A e pull-down em B (ex.: 10k) se necessário para estado definido."));
   Serial.println(F(" - Se precisar de maior robustez em taxas maiores que 9600, prefira AltSoftSerial/NeoSWSerial ou hardware Serial."));
   Serial.println();
   
   lastTest = millis();
 }
 
 void loop()
 {
   unsigned long currentTime = millis();
   
   if (currentTime - lastTest >= testInterval)
   {
     lastTest = currentTime;
     
     // Alternar baud rate a cada 2 ciclos
     if (testCount > 0 && testCount % 2 == 0) {
       useBaud9600 = !useBaud9600;
       unsigned long newBaud = useBaud9600 ? RS485_BAUD_ALT : RS485_BAUD;
       
       Serial.print(F("\n>>> Mudando baud rate para: "));
       Serial.println(newBaud);
       
       rs485Serial.end();
       delay(100);
       rs485Serial.begin(newBaud);
       delay(500);
     }
     
     Serial.print(F("\n>>> Baud rate atual: "));
     Serial.println(useBaud9600 ? RS485_BAUD_ALT : RS485_BAUD);
     
     // Teste 1: Leitura individual de NPK (conforme tutorial)
     testNPKIndividual();
     delay(1000);
     
     // Teste 2: Leitura múltipla de registros (método original)
     testModbusCommunication();
     
     delay(500);
   }
   
   // Pequeno delay para não sobrecarregar o processador
   delay(100);
 }
 