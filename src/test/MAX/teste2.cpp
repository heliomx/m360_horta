#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// ===== CONFIGURAÇÃO RS485 =====
#define MAX485_DE 7     // Driver Enable (DE)
#define MAX485_RE_NEG 8 // Receiver Enable (RE_N) -> neste hardware RE e DE estão amarrados ao mesmo pino
#define MAX485_RO_RX 9  // Pino RX do módulo MAX485 (DI -> Arduino RX)
#define MAX485_DI_TX 10 // Pino TX do módulo MAX485 (RO -> Arduino TX)

// Configuração da comunicação serial (linha RS-485)
const unsigned long RS485_BAUD = 4800; // Baud rate padrão do sensor

SoftwareSerial rs485Serial(MAX485_RO_RX, MAX485_DI_TX);
ModbusMaster node;

// ===== VARIÁVEIS GLOBAIS =====
unsigned long lastRead = 0;
const unsigned long readInterval = 5000; // 5 segundos entre leituras
uint8_t readCount = 0;

// ===== FUNÇÕES RS485 =====
// Chamado pelo ModbusMaster antes de transmitir (ativa driver)
void preTransmission()
{
  // Se RE for ativo baixo (RE_N), escrever HIGH desativa recepção e permite transmitir
  digitalWrite(MAX485_RE_NEG, HIGH); // Desabilita recepção
  digitalWrite(MAX485_DE, HIGH);     // Habilita transmissão
  // Pequeno tempo para permitir que o driver mude de estado
  delay(1);
}

// Chamado pelo ModbusMaster após finalizar envio (desativa driver -> volta a recepção)
void postTransmission()
{
  // Garante que todo o buffer da UART seja transmitido fisicamente
  rs485Serial.flush();
  
  // Aguardar tempo para garantir transmissão completa
  delay(2);

  // Habilita recepção e desabilita transmissão
  digitalWrite(MAX485_RE_NEG, LOW); // Habilita recepção
  digitalWrite(MAX485_DE, LOW);     // Desabilita transmissão
  // Pequeno delay para estabilizar antes de iniciar leitura
  delay(1);
}

// ===== FUNÇÕES DE ERRO =====
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

// ===== FUNÇÕES DE LEITURA =====
void printSensorData()
{
  Serial.println(F("\n=== DADOS DO SENSOR ZTS-3002 ==="));
  
  // Umidade do solo (0-100%) - registro 0x0000, escala ÷ 10
  float moisture = node.getResponseBuffer(0) / 10.0;
  Serial.print(F("Umidade: "));
  Serial.print(moisture, 1);
  Serial.println(F(" %"));

  // Temperatura do solo (°C) - registro 0x0001, escala ÷ 10, signed int16
  int16_t rawTemp = (int16_t) node.getResponseBuffer(1);
  float temperature = rawTemp / 10.0;
  Serial.print(F("Temperatura: "));
  Serial.print(temperature, 1);
  Serial.println(F(" °C"));

  // Condutividade elétrica (mS/cm) - registro 0x0002, escala ÷ 100
  float conductivity = node.getResponseBuffer(2) / 100.0;
  Serial.print(F("Condutividade: "));
  Serial.print(conductivity, 2);
  Serial.println(F(" mS/cm"));

  // pH (0-14) - registro 0x0003, escala ÷ 100
  float ph = node.getResponseBuffer(3) / 100.0;
  Serial.print(F("pH: "));
  Serial.print(ph, 2);
  Serial.println();

  // Nitrogênio (mg/kg) - registro 0x0004, valor direto
  uint16_t nitrogen = node.getResponseBuffer(4);
  Serial.print(F("Nitrogênio: "));
  Serial.print(nitrogen);
  Serial.println(F(" mg/kg"));

  // Fósforo (mg/kg) - registro 0x0005, valor direto
  uint16_t phosphorus = node.getResponseBuffer(5);
  Serial.print(F("Fósforo: "));
  Serial.print(phosphorus);
  Serial.println(F(" mg/kg"));

  // Potássio (mg/kg) - registro 0x0006, valor direto
  uint16_t potassium = node.getResponseBuffer(6);
  Serial.print(F("Potássio: "));
  Serial.print(potassium);
  Serial.println(F(" mg/kg"));
  
  Serial.println(F("================================\n"));
}

void readSensorData()
{
  readCount++;
  Serial.print(F("Leitura #"));
  Serial.print(readCount);
  Serial.println(F(" - Iniciando leitura do sensor..."));

  // Ler 7 registros a partir do endereço 0x0000 (0-6)
  // Registros: Umidade, Temperatura, Condutividade, pH, N, P, K
  uint8_t result = node.readHoldingRegisters(0x0000, 7);
  
  if (result == 0x00) // ku8MBSuccess
  {
    Serial.println(F("✓ Leitura bem-sucedida!"));
    printSensorData();
  }
  else
  {
    printModbusError(result);
    Serial.println(F("✗ Falha na leitura do sensor"));
  }
  
  Serial.print(F("Próxima leitura em "));
  Serial.print(readInterval / 1000);
  Serial.println(F(" segundos...\n"));
}

// ===== SETUP =====
void setup()
{
  // Inicializar Serial para debug
  Serial.begin(115200);
  delay(1000);
  
  Serial.println(F("========================================"));
  Serial.println(F("    SENSOR ZTS-3002 - M360 Network"));
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
  node.begin(1, rs485Serial); // ID do sensor = 1 (padrão de fábrica)
  
  // Configurar callbacks de transmissão
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  
  Serial.println(F("Configuração RS485 inicializada:"));
  Serial.print(F("  - Baud Rate: "));
  Serial.println(RS485_BAUD);
  Serial.print(F("  - ID Escravo: 1"));
  Serial.println();
  Serial.print(F("  - DE Pin: "));
  Serial.println(MAX485_DE);
  Serial.print(F("  - RE Pin: "));
  Serial.println(MAX485_RE_NEG);
  Serial.print(F("  - RX Pin: "));
  Serial.println(MAX485_RO_RX);
  Serial.print(F("  - TX Pin: "));
  Serial.println(MAX485_DI_TX);
  Serial.println();
  Serial.println(F("Aguardando sensor..."));
  Serial.println();
  
  lastRead = millis();
}

// ===== LOOP =====
void loop()
{
  unsigned long currentTime = millis();
  
  if (currentTime - lastRead >= readInterval)
  {
    lastRead = currentTime;
    readSensorData();
  }
  
  // Pequeno delay para não sobrecarregar o processador
  delay(100);
}