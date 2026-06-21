#include "config_utils.h"
#include <ESP8266WiFi.h>

// Definir a variável global config
DeviceConfig config;

namespace {

template <size_t N>
void terminateField(char (&field)[N]) {
  field[N - 1] = '\0';
}

template <size_t N>
void copyField(char (&destination)[N], const char *source) {
  memset(destination, 0, N);
  strncpy(destination, source, N - 1);
}

void terminateConfigFields(DeviceConfig &cfg) {
  terminateField(cfg.ssid);
  terminateField(cfg.password);
  terminateField(cfg.mqttServer);
  terminateField(cfg.mqttUser);
  terminateField(cfg.mqttPassword);
  terminateField(cfg.uf);
  terminateField(cfg.carNumber);
}

uint16_t calcLegacyCRC(const DeviceConfig &cfg) {
  uint16_t crc = 0;
  for (size_t i = 0; i < sizeof(cfg.ssid) && cfg.ssid[i] != '\0'; i++) crc += cfg.ssid[i];
  for (size_t i = 0; i < sizeof(cfg.mqttServer) && cfg.mqttServer[i] != '\0'; i++) crc += cfg.mqttServer[i];
  for (size_t i = 0; i < sizeof(cfg.uf) && cfg.uf[i] != '\0'; i++) crc += cfg.uf[i];
  for (size_t i = 0; i < sizeof(cfg.carNumber) && cfg.carNumber[i] != '\0'; i++) crc += cfg.carNumber[i];
  return static_cast<uint16_t>(crc + cfg.mqttPort + cfg.version);
}

} // namespace

// ==== FUNÇÕES AUXILIARES MYSENSORS ====
const char* getTypeDescription(int type) {
  switch(type) {
    case 0: return "temperature";
    case 1: return "humidity";
    case 2: return "light";
    case 3: return "dimmer";
    case 4: return "pressure";
    case 5: return "forecast";
    case 6: return "rain";
    case 7: return "rainrate";
    case 8: return "wind";
    case 9: return "gust";
    case 10: return "direction";
    case 11: return "uv";
    case 12: return "weight";
    case 13: return "distance";
    case 14: return "impedance";
    case 15: return "armed";
    case 16: return "tripped";
    case 17: return "watt";
    case 18: return "kwh";
    case 19: return "scene_on";
    case 20: return "scene_off";
    case 21: return "hvac_flow";
    case 22: return "hvac_speed";
    case 23: return "light_level";
    case 24: return "var1";
    case 25: return "var2";
    case 26: return "var3";
    case 27: return "var4";
    case 28: return "var5";
    case 29: return "up";
    case 30: return "down";
    case 31: return "stop";
    case 32: return "ir_send";
    case 33: return "ir_receive";
    case 34: return "flow";
    case 35: return "volume";
    case 36: return "lock_status";
    case 37: return "level";
    case 38: return "voltage";
    case 39: return "current";
    case 40: return "rgb";
    case 41: return "rgbw";
    case 42: return "id";
    case 43: return "unit_prefix";
    case 44: return "hvac_cool";
    case 45: return "hvac_heat";
    case 46: return "hvac_flow_mode";
    case 47: return "text";
    case 48: return "custom";
    case 49: return "position";
    case 50: return "ir_record";
    case 51: return "ph";
    case 52: return "orp";
    case 53: return "ec";
    case 54: return "var";
    case 55: return "va";
    case 56: return "power_factor";
    default: return "unknown";
  }
}

int getRSSI() {
  // Retorna RSSI do último pacote recebido (se disponível)
  // Por enquanto retorna um valor simulado
  return -45;
}

int getBatteryLevel() {
  // Retorna nível da bateria (se disponível)
  // Por enquanto retorna um valor simulado
  return 85;
}

uint16_t calcCRC(const DeviceConfig &cfg) {
  // CRC simples baseado na soma dos caracteres dos campos principais
  uint16_t crc = 0;
  
  // Somar caracteres do SSID
  for (size_t i = 0; i < sizeof(cfg.ssid) && cfg.ssid[i] != '\0'; i++) {
    crc += cfg.ssid[i];
  }
  
  // Somar caracteres do MQTT Server
  for (size_t i = 0; i < sizeof(cfg.password) && cfg.password[i] != '\0'; i++) {
    crc += cfg.password[i];
  }
  for (size_t i = 0; i < sizeof(cfg.mqttServer) && cfg.mqttServer[i] != '\0'; i++) {
    crc += cfg.mqttServer[i];
  }
  
  // Somar caracteres da UF
  for (size_t i = 0; i < sizeof(cfg.mqttUser) && cfg.mqttUser[i] != '\0'; i++) {
    crc += cfg.mqttUser[i];
  }
  for (size_t i = 0; i < sizeof(cfg.mqttPassword) && cfg.mqttPassword[i] != '\0'; i++) {
    crc += cfg.mqttPassword[i];
  }
  for (size_t i = 0; i < sizeof(cfg.uf) && cfg.uf[i] != '\0'; i++) {
    crc += cfg.uf[i];
  }
  
  // Somar caracteres do CAR Number
  for (size_t i = 0; i < sizeof(cfg.carNumber) && cfg.carNumber[i] != '\0'; i++) {
    crc += cfg.carNumber[i];
  }
  
  // Somar porta MQTT
  crc += cfg.mqttPort;
  
  // Somar versão
  crc += cfg.version;
  
  return crc;
}

void loadConfig(DeviceConfig &cfg) {
  // Serial.println("📖 Carregando configuração da EEPROM...");
  
  // Debug do tamanho da estrutura
  // Serial.print("   Tamanho da estrutura DeviceConfig: ");
  // Serial.println(sizeof(DeviceConfig));
  
  // Debug dos primeiros bytes da EEPROM ANTES de qualquer operação (na região de config)
  
  // Serial.println("🔍 Primeiros bytes da EEPROM ANTES de carregar (base=CONFIG_EEPROM_BASE):");
  // for (int i = 0; i < 64; i++) {
  //   uint8_t byte = EEPROM.read(CONFIG_EEPROM_BASE + i);
  //   if (i % 16 == 0) Serial.println();
  //   Serial.printf("%02X ", byte);
  // }
  // Serial.println();
  
  // NÃO limpar estrutura antes de carregar - pode estar causando problema
  // memset(&cfg, 0, sizeof(DeviceConfig));
  
  // Tentar método alternativo: carregar campo por campo
  // Serial.println("🔧 Carregando campo por campo...");
  
  int addr = CONFIG_EEPROM_BASE;
  
  // Carregar versão
  cfg.version = EEPROM.read(addr);
  addr += sizeof(cfg.version);
  
  // Serial.print("   Versão carregada: ");
  // Serial.println(cfg.version);
  // Serial.print("   Próximo endereço (SSID): ");
  // Serial.println(addr);
  
  // Carregar SSID
  
  // Serial.println("   Carregando SSID...");
  for (size_t i = 0; i < sizeof(cfg.ssid); i++) {
    cfg.ssid[i] = EEPROM.read(addr + i);
    // if (i < 10) {
    //   Serial.printf("      SSID[%d] = 0x%02X ('%c') from addr %d\n", i, cfg.ssid[i], 
    //                 (cfg.ssid[i] >= 32 && cfg.ssid[i] <= 126) ? cfg.ssid[i] : '?', addr + i);
    // }
  }
  addr += sizeof(cfg.ssid);
  
 
  // Serial.print("   SSID carregado: '");
  // Serial.print(cfg.ssid);
  // Serial.print("' (length: ");
  // Serial.print(strlen(cfg.ssid));
  // Serial.println(")");
  
  // Carregar password
  for (size_t i = 0; i < sizeof(cfg.password); i++) {
    cfg.password[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.password);
  
  // Carregar MQTT Server
  for (size_t i = 0; i < sizeof(cfg.mqttServer); i++) {
    cfg.mqttServer[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.mqttServer);
  
  // Carregar MQTT Port
  EEPROM.get(addr, cfg.mqttPort);
  addr += sizeof(cfg.mqttPort);
  
  // Carregar MQTT User
  for (size_t i = 0; i < sizeof(cfg.mqttUser); i++) {
    cfg.mqttUser[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.mqttUser);
  
  // Carregar MQTT Password
  for (size_t i = 0; i < sizeof(cfg.mqttPassword); i++) {
    cfg.mqttPassword[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.mqttPassword);
  
  // Carregar UF
  for (size_t i = 0; i < sizeof(cfg.uf); i++) {
    cfg.uf[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.uf);
  
  // Carregar CAR Number
  for (size_t i = 0; i < sizeof(cfg.carNumber); i++) {
    cfg.carNumber[i] = EEPROM.read(addr + i);
  }
  addr += sizeof(cfg.carNumber);
  
  // Carregar CRC
  EEPROM.get(addr, cfg.crc);
  terminateConfigFields(cfg);
  if (cfg.version == 1 && cfg.crc == calcLegacyCRC(cfg)) {
    cfg.version = 2;
    saveConfig(cfg);
  }
  
  // Serial.println("🔍 Verificando configuração:");
  // Serial.print("   Versão: ");
  // Serial.println(cfg.version);
  // Serial.print("   SSID: '");
  // Serial.print(cfg.ssid);
  // Serial.print("' (length: ");
  // Serial.print(strlen(cfg.ssid));
  // Serial.println(")");
  // Serial.print("   MQTT Server: '");
  // Serial.print(cfg.mqttServer);
  // Serial.print("' (length: ");
  // Serial.print(strlen(cfg.mqttServer));
  // Serial.println(")");
  // Serial.print("   MQTT Port: ");
  // Serial.println(cfg.mqttPort);
  // Serial.print("   UF: '");
  // Serial.print(cfg.uf);
  // Serial.println("'");
  // Serial.print("   CAR: '");
  // Serial.print(cfg.carNumber);
  // Serial.println("'");
  // Serial.print("   CRC Calculado: ");
  // Serial.println(calcCRC(cfg));
  // Serial.print("   CRC Salvo: ");
  // Serial.println(cfg.crc);
  
  // loadConfig apenas carrega os dados brutos da EEPROM.
  // A validação e decisão de reset são responsabilidade do chamador (before()).
}

void saveConfig(const DeviceConfig &cfg) {
  // Serial.println("💾 Salvando configuração na EEPROM...");
  
  // Debug antes de calcular CRC
  // Serial.println("🔍 Dados ANTES de calcular CRC:");
  // Serial.print("   SSID: '");
  // Serial.print(cfg.ssid);
  // Serial.print("' (length: ");
  // Serial.print(strlen(cfg.ssid));
  // Serial.println(")");
  
  DeviceConfig temp = cfg;
  terminateConfigFields(temp);
  temp.crc = calcCRC(temp);
  
  // Debug após calcular CRC
  // Serial.println("🔍 Dados APÓS calcular CRC:");
  // Serial.print("   SSID: '");
  // Serial.print(temp.ssid);
  // Serial.print("' (length: ");
  // Serial.print(strlen(temp.ssid));
  // Serial.println(")");
  
  // Serial.println("🔍 Dados a serem salvos:");
  // Serial.print("   Versão: ");
  // Serial.println(temp.version);
  // Serial.print("   SSID: ");
  // Serial.println(temp.ssid);
  // Serial.print("   MQTT Server: ");
  // Serial.println(temp.mqttServer);
  // Serial.print("   CRC: ");
  // Serial.println(temp.crc);
  
  // Tentar método alternativo: salvar campo por campo
  // Serial.println("🔧 Tentando método alternativo de salvamento...");
  
  int addr = CONFIG_EEPROM_BASE;
  
  // Salvar versão
  EEPROM.put(addr, temp.version);
  addr += sizeof(temp.version);
  
  // Salvar SSID
  for (size_t i = 0; i < sizeof(temp.ssid); i++) {
    EEPROM.put(addr + i, temp.ssid[i]);
  }
  addr += sizeof(temp.ssid);
  
  // Salvar password
  for (size_t i = 0; i < sizeof(temp.password); i++) {
    EEPROM.put(addr + i, temp.password[i]);
  }
  addr += sizeof(temp.password);
  
  // Salvar MQTT Server
  for (size_t i = 0; i < sizeof(temp.mqttServer); i++) {
    EEPROM.put(addr + i, temp.mqttServer[i]);
  }
  addr += sizeof(temp.mqttServer);
  
  // Salvar MQTT Port
  EEPROM.put(addr, temp.mqttPort);
  addr += sizeof(temp.mqttPort);
  
  // Salvar MQTT User
  for (size_t i = 0; i < sizeof(temp.mqttUser); i++) {
    EEPROM.put(addr + i, temp.mqttUser[i]);
  }
  addr += sizeof(temp.mqttUser);
  
  // Salvar MQTT Password
  for (size_t i = 0; i < sizeof(temp.mqttPassword); i++) {
    EEPROM.put(addr + i, temp.mqttPassword[i]);
  }
  addr += sizeof(temp.mqttPassword);
  
  // Salvar UF
  for (size_t i = 0; i < sizeof(temp.uf); i++) {
    EEPROM.put(addr + i, temp.uf[i]);
  }
  addr += sizeof(temp.uf);
  
  // Salvar CAR Number
  for (size_t i = 0; i < sizeof(temp.carNumber); i++) {
    EEPROM.put(addr + i, temp.carNumber[i]);
  }
  addr += sizeof(temp.carNumber);
  
  // Salvar CRC
  EEPROM.put(addr, temp.crc);
  
  bool success = EEPROM.commit();
  
  if (success) {
    // Serial.println("✅ Configuração salva com sucesso na EEPROM (método alternativo)");
    
  // Debug dos primeiros bytes após salvar (na região de config)
  
  // Serial.println("🔍 Primeiros bytes da EEPROM após salvar (base=CONFIG_EEPROM_BASE):");
  // for (int i = 0; i < 64; i++) {
  //   uint8_t byte = EEPROM.read(CONFIG_EEPROM_BASE + i);
  //     if (i % 16 == 0) Serial.println();
  //     Serial.printf("%02X ", byte);
  //   }
  //   Serial.println();
  
  } else {
    // Serial.println("❌ Erro ao salvar configuração na EEPROM");
  }
}

void performFactoryReset(DeviceConfig &cfg) {
  // Serial.println("🔄 Executando reset de fábrica...");
  
  // Limpar toda a estrutura
  memset(&cfg, 0, sizeof(DeviceConfig));
  
  // Definir valores padrão
  cfg.version = 2;
  copyField(cfg.ssid, M360_STA_SSID);
  copyField(cfg.password, M360_STA_PASSWORD);
  copyField(cfg.mqttServer, M360_MQTT_SERVER);
  cfg.mqttPort = M360_MQTT_PORT;
  copyField(cfg.mqttUser, M360_MQTT_USER);
  copyField(cfg.mqttPassword, M360_MQTT_PASSWORD);
  copyField(cfg.uf, M360_UF);
  copyField(cfg.carNumber, M360_CAR_NUMBER);
  
  // Calcular CRC
  cfg.crc = calcCRC(cfg);
  
  // Serial.println("✅ Reset de fábrica concluído");
  // Serial.print("   SSID: ");
  // Serial.println(cfg.ssid);
  // Serial.print("   MQTT Server: ");
  // Serial.println(cfg.mqttServer);
  // Serial.print("   UF: ");
  // Serial.println(cfg.uf);
  // Serial.print("   CAR: ");
  // Serial.println(cfg.carNumber);
  // Serial.print("   CRC: ");
  // Serial.println(cfg.crc);
}

bool isValidConfig(const DeviceConfig &cfg) {
  return cfg.version == 2 && cfg.ssid[0] != '\0' && cfg.mqttServer[0] != '\0' &&
         cfg.uf[0] != '\0' && cfg.carNumber[0] != '\0' && cfg.mqttPort > 0 &&
         cfg.crc == calcCRC(cfg);
}

// Funções auxiliares para tópicos MQTT
String buildTopicOut(const DeviceConfig &cfg) {
  String topic = "m360/";
  topic += cfg.uf;
  topic += "/";
  topic += cfg.carNumber;
  topic += "/out";
  return topic;
}

String buildTopicIn(const DeviceConfig &cfg) {
  String topic = "m360/";
  topic += cfg.uf;
  topic += "/";
  topic += cfg.carNumber;
  topic += "/in";
  return topic;
} 

// Utilitário: lê A0 duas vezes e retorna true se a média estiver abaixo do threshold
bool isA0Low(int pin, int threshold) {
  int a1 = analogRead(pin);
  delay(10);
  int a2 = analogRead(pin);
  return ((a1 + a2) / 2) < threshold;
}


void checkResetButton(DeviceConfig &cfg, int pin) {
  // NÃO verificar reset quando estiver em modo AP
  if (WiFi.getMode() == WIFI_AP) {
    return;
  }

  static unsigned long pressed = 0;
  static unsigned long lastCheck = 0;
  static unsigned long bootTime = 0;

  // Inicializar tempo de boot na primeira chamada
  if (bootTime == 0) {
    bootTime = millis();
  }

  // Só verificar reset nos primeiros 30 segundos após boot
  if (millis() - bootTime > 30000) {
    return;
  }

  // Verificar a cada 2 segundos para evitar leituras excessivas
  if (millis() - lastCheck < 2000) {
    return;
  }
  lastCheck = millis();

  // Ler valor analógico e fazer média para reduzir ruído
  int value1 = analogRead(pin);
  delay(10);
  int value2 = analogRead(pin);
  int value = (value1 + value2) / 2;

  // Debug apenas a cada 10 segundos para não poluir o serial
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 10000) {
    Serial.printf("🔍 Pino A0: %d (threshold: 20) - Janela: %lds\n", value, (30000 - (millis() - bootTime)) / 1000);
    lastDebug = millis();
  }

  if (value < 400) {  // Threshold muito mais conservador (20 em vez de 50)
    if (pressed == 0) {
      pressed = millis();
      Serial.println("🔄 Pino A0 detectado como BAIXO - iniciando contagem...");
    }

    if (millis() - pressed > 3000) {
      Serial.println("✅ Reset confirmado! Executando reset de fábrica...");
      performFactoryReset(cfg);
      saveConfig(cfg);
      ESP.restart();
    }
  } else {
    if (pressed != 0) {
      Serial.println("ℹ️ Pino A0 voltou para ALTO - cancelando reset");
      pressed = 0;
    }
  }
}
