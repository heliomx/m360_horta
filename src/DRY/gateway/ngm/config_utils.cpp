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
  return WiFi.RSSI();
}

int getBatteryLevel() {
  return 100;  // Gateway alimentado por fonte fixa — sem medição de bateria
}

uint16_t calcCRC(const DeviceConfig &cfg) {
  uint16_t crc = 0;
  for (size_t i = 0; i < sizeof(cfg.ssid) && cfg.ssid[i] != '\0'; i++) crc += cfg.ssid[i];
  for (size_t i = 0; i < sizeof(cfg.password) && cfg.password[i] != '\0'; i++) crc += cfg.password[i];
  for (size_t i = 0; i < sizeof(cfg.mqttServer) && cfg.mqttServer[i] != '\0'; i++) crc += cfg.mqttServer[i];
  for (size_t i = 0; i < sizeof(cfg.mqttUser) && cfg.mqttUser[i] != '\0'; i++) crc += cfg.mqttUser[i];
  for (size_t i = 0; i < sizeof(cfg.mqttPassword) && cfg.mqttPassword[i] != '\0'; i++) crc += cfg.mqttPassword[i];
  for (size_t i = 0; i < sizeof(cfg.uf) && cfg.uf[i] != '\0'; i++) crc += cfg.uf[i];
  for (size_t i = 0; i < sizeof(cfg.carNumber) && cfg.carNumber[i] != '\0'; i++) crc += cfg.carNumber[i];
  crc += cfg.mqttPort;
  crc += cfg.version;
  return crc;
}

void loadConfig(DeviceConfig &cfg) {
  int addr = CONFIG_EEPROM_BASE;

  cfg.version = EEPROM.read(addr);
  addr += sizeof(cfg.version);

  for (size_t i = 0; i < sizeof(cfg.ssid); i++) cfg.ssid[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.ssid);

  for (size_t i = 0; i < sizeof(cfg.password); i++) cfg.password[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.password);

  for (size_t i = 0; i < sizeof(cfg.mqttServer); i++) cfg.mqttServer[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.mqttServer);

  EEPROM.get(addr, cfg.mqttPort);
  addr += sizeof(cfg.mqttPort);

  for (size_t i = 0; i < sizeof(cfg.mqttUser); i++) cfg.mqttUser[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.mqttUser);

  for (size_t i = 0; i < sizeof(cfg.mqttPassword); i++) cfg.mqttPassword[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.mqttPassword);

  for (size_t i = 0; i < sizeof(cfg.uf); i++) cfg.uf[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.uf);

  for (size_t i = 0; i < sizeof(cfg.carNumber); i++) cfg.carNumber[i] = EEPROM.read(addr + i);
  addr += sizeof(cfg.carNumber);

  EEPROM.get(addr, cfg.crc);
  terminateConfigFields(cfg);

  // Migração v1 → v2 (P2-nota: calcLegacyCRC não inclui password/mqttPassword)
  if (cfg.version == 1 && cfg.crc == calcLegacyCRC(cfg)) {
    cfg.version = 2;
    saveConfig(cfg);
  }

  // loadConfig apenas carrega os dados brutos da EEPROM.
  // A validação e decisão de reset são responsabilidade do chamador (before()).
}

void saveConfig(const DeviceConfig &cfg) {
  DeviceConfig temp = cfg;
  terminateConfigFields(temp);
  temp.crc = calcCRC(temp);

  int addr = CONFIG_EEPROM_BASE;

  EEPROM.put(addr, temp.version);
  addr += sizeof(temp.version);

  for (size_t i = 0; i < sizeof(temp.ssid); i++) EEPROM.put(addr + i, temp.ssid[i]);
  addr += sizeof(temp.ssid);

  for (size_t i = 0; i < sizeof(temp.password); i++) EEPROM.put(addr + i, temp.password[i]);
  addr += sizeof(temp.password);

  for (size_t i = 0; i < sizeof(temp.mqttServer); i++) EEPROM.put(addr + i, temp.mqttServer[i]);
  addr += sizeof(temp.mqttServer);

  EEPROM.put(addr, temp.mqttPort);
  addr += sizeof(temp.mqttPort);

  for (size_t i = 0; i < sizeof(temp.mqttUser); i++) EEPROM.put(addr + i, temp.mqttUser[i]);
  addr += sizeof(temp.mqttUser);

  for (size_t i = 0; i < sizeof(temp.mqttPassword); i++) EEPROM.put(addr + i, temp.mqttPassword[i]);
  addr += sizeof(temp.mqttPassword);

  for (size_t i = 0; i < sizeof(temp.uf); i++) EEPROM.put(addr + i, temp.uf[i]);
  addr += sizeof(temp.uf);

  for (size_t i = 0; i < sizeof(temp.carNumber); i++) EEPROM.put(addr + i, temp.carNumber[i]);
  addr += sizeof(temp.carNumber);

  EEPROM.put(addr, temp.crc);

  // P1: falha de commit sempre visível em produção
  if (!EEPROM.commit()) {
    Serial.println(F("EEPROM:COMMIT_FAIL"));
  }
}

void performFactoryReset(DeviceConfig &cfg) {
  memset(&cfg, 0, sizeof(DeviceConfig));
  cfg.version = 2;
  copyField(cfg.ssid, M360_STA_SSID);
  copyField(cfg.password, M360_STA_PASSWORD);
  copyField(cfg.mqttServer, M360_MQTT_SERVER);
  cfg.mqttPort = M360_MQTT_PORT;
  copyField(cfg.mqttUser, M360_MQTT_USER);
  copyField(cfg.mqttPassword, M360_MQTT_PASSWORD);
  copyField(cfg.uf, M360_UF);
  copyField(cfg.carNumber, M360_CAR_NUMBER);
  cfg.crc = calcCRC(cfg);
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

String buildTopicGatewayStatus(const DeviceConfig &cfg) {
  String topic = "m360/";
  topic += cfg.uf;
  topic += "/";
  topic += cfg.carNumber;
  topic += "/gateway/status";
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

  int value1 = analogRead(pin);
  delay(10);
  int value2 = analogRead(pin);
  int value = (value1 + value2) / 2;

  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 10000) {
    Serial.printf("A0:%d janela:%lds\n", value, (30000 - (millis() - bootTime)) / 1000);
    lastDebug = millis();
  }

  if (value < 400) {
    if (pressed == 0) {
      pressed = millis();
      Serial.println(F("A0:LOW reset contando..."));
    }
    if (millis() - pressed > 3000) {
      Serial.println(F("A0:RESET confirmado"));
      performFactoryReset(cfg);
      saveConfig(cfg);
      ESP.restart();
    }
  } else {
    if (pressed != 0) {
      Serial.println(F("A0:HIGH reset cancelado"));
      pressed = 0;
    }
  }
}
