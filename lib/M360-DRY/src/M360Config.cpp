/*
 * M360Config.cpp — Gerenciamento de EEPROM para M360-DRY (Infraestrutura)
 */

#ifdef ESP8266

#include "M360Config.h"
#include <EEPROM.h>
#include <M360Credentials.h>

namespace M360 {

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

void terminateConfigFields(M360DeviceConfig& cfg) {
	terminateField(cfg.ssid);
	terminateField(cfg.password);
	terminateField(cfg.mqttServer);
	terminateField(cfg.mqttUser);
	terminateField(cfg.mqttPassword);
	terminateField(cfg.uf);
	terminateField(cfg.carNumber);
}

uint16_t calculateLegacyCRC(const M360DeviceConfig& cfg) {
	uint16_t crc = 0;
	for (size_t i = 0; i < sizeof(cfg.ssid) && cfg.ssid[i] != '\0'; i++) crc += cfg.ssid[i];
	for (size_t i = 0; i < sizeof(cfg.mqttServer) && cfg.mqttServer[i] != '\0'; i++) crc += cfg.mqttServer[i];
	for (size_t i = 0; i < sizeof(cfg.uf) && cfg.uf[i] != '\0'; i++) crc += cfg.uf[i];
	for (size_t i = 0; i < sizeof(cfg.carNumber) && cfg.carNumber[i] != '\0'; i++) crc += cfg.carNumber[i];
	return static_cast<uint16_t>(crc + cfg.mqttPort + cfg.version);
}

} // namespace

void Config::load(M360DeviceConfig& cfg) {
	int addr = M360_EEPROM_DEVICE_BASE;
	
	// Carregar versão
	cfg.version = ::EEPROM.read(addr);
	addr += sizeof(cfg.version);
	
	// Carregar SSID
	for (unsigned int i = 0; i < sizeof(cfg.ssid); i++) {
		cfg.ssid[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.ssid);
	
	// Carregar password
	for (unsigned int i = 0; i < sizeof(cfg.password); i++) {
		cfg.password[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.password);
	
	// Carregar MQTT Server
	for (unsigned int i = 0; i < sizeof(cfg.mqttServer); i++) {
		cfg.mqttServer[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.mqttServer);
	
	// Carregar MQTT Port
	::EEPROM.get(addr, cfg.mqttPort);
	addr += sizeof(cfg.mqttPort);
	
	// Carregar MQTT User
	for (unsigned int i = 0; i < sizeof(cfg.mqttUser); i++) {
		cfg.mqttUser[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.mqttUser);
	
	// Carregar MQTT Password
	for (unsigned int i = 0; i < sizeof(cfg.mqttPassword); i++) {
		cfg.mqttPassword[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.mqttPassword);
	
	// Carregar UF
	for (unsigned int i = 0; i < sizeof(cfg.uf); i++) {
		cfg.uf[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.uf);
	
	// Carregar CAR Number
	for (unsigned int i = 0; i < sizeof(cfg.carNumber); i++) {
		cfg.carNumber[i] = ::EEPROM.read(addr + i);
	}
	addr += sizeof(cfg.carNumber);
	
	// Carregar CRC
	::EEPROM.get(addr, cfg.crc);
	terminateConfigFields(cfg);
	if (cfg.version == 1 && cfg.crc == calculateLegacyCRC(cfg)) {
		cfg.version = 2;
		Config::save(cfg);
	}
}

void Config::save(const M360DeviceConfig& cfg) {
	M360DeviceConfig temp = cfg;
	terminateConfigFields(temp);
	temp.crc = calculateCRC(temp);
	
	int addr = M360_EEPROM_DEVICE_BASE;
	
	// Salvar campo a campo (robusto a alinhamento de struct)
	::EEPROM.put(addr, temp.version);
	addr += sizeof(temp.version);
	
	for (unsigned int i = 0; i < sizeof(temp.ssid); i++) {
		::EEPROM.put(addr + i, temp.ssid[i]);
	}
	addr += sizeof(temp.ssid);
	
	for (unsigned int i = 0; i < sizeof(temp.password); i++) {
		::EEPROM.put(addr + i, temp.password[i]);
	}
	addr += sizeof(temp.password);
	
	for (unsigned int i = 0; i < sizeof(temp.mqttServer); i++) {
		::EEPROM.put(addr + i, temp.mqttServer[i]);
	}
	addr += sizeof(temp.mqttServer);
	
	::EEPROM.put(addr, temp.mqttPort);
	addr += sizeof(temp.mqttPort);
	
	for (unsigned int i = 0; i < sizeof(temp.mqttUser); i++) {
		::EEPROM.put(addr + i, temp.mqttUser[i]);
	}
	addr += sizeof(temp.mqttUser);
	
	for (unsigned int i = 0; i < sizeof(temp.mqttPassword); i++) {
		::EEPROM.put(addr + i, temp.mqttPassword[i]);
	}
	addr += sizeof(temp.mqttPassword);
	
	for (unsigned int i = 0; i < sizeof(temp.uf); i++) {
		::EEPROM.put(addr + i, temp.uf[i]);
	}
	addr += sizeof(temp.uf);
	
	for (unsigned int i = 0; i < sizeof(temp.carNumber); i++) {
		::EEPROM.put(addr + i, temp.carNumber[i]);
	}
	addr += sizeof(temp.carNumber);
	
	::EEPROM.put(addr, temp.crc);
	
	::EEPROM.commit();
}

void Config::reset(M360DeviceConfig& cfg) {
	memset(&cfg, 0, sizeof(M360DeviceConfig));
	
	cfg.version = 2;
	copyField(cfg.ssid, M360_STA_SSID);
	copyField(cfg.password, M360_STA_PASSWORD);
	copyField(cfg.mqttServer, M360_MQTT_SERVER);
	cfg.mqttPort = M360_MQTT_PORT;
	copyField(cfg.mqttUser, M360_MQTT_USER);
	copyField(cfg.mqttPassword, M360_MQTT_PASSWORD);
	copyField(cfg.uf, M360_UF);
	copyField(cfg.carNumber, M360_CAR_NUMBER);
	
	cfg.crc = calculateCRC(cfg);
}

bool Config::isValid(const M360DeviceConfig& cfg) {
	return cfg.version == 2 && cfg.ssid[0] != '\0' && cfg.mqttServer[0] != '\0' &&
	       cfg.uf[0] != '\0' && cfg.carNumber[0] != '\0' && cfg.mqttPort > 0 &&
	       cfg.crc == calculateCRC(cfg);
}

uint16_t Config::calculateCRC(const M360DeviceConfig& cfg) {
	uint16_t crc = 0;
	
	// SSID
	for (size_t i = 0; i < sizeof(cfg.ssid) && cfg.ssid[i] != '\0'; i++) crc += cfg.ssid[i];
	for (size_t i = 0; i < sizeof(cfg.password) && cfg.password[i] != '\0'; i++) crc += cfg.password[i];
	// MQTT Server
	for (size_t i = 0; i < sizeof(cfg.mqttServer) && cfg.mqttServer[i] != '\0'; i++) crc += cfg.mqttServer[i];
	for (size_t i = 0; i < sizeof(cfg.mqttUser) && cfg.mqttUser[i] != '\0'; i++) crc += cfg.mqttUser[i];
	for (size_t i = 0; i < sizeof(cfg.mqttPassword) && cfg.mqttPassword[i] != '\0'; i++) crc += cfg.mqttPassword[i];
	// UF
	for (size_t i = 0; i < sizeof(cfg.uf) && cfg.uf[i] != '\0'; i++) crc += cfg.uf[i];
	// CAR Number
	for (size_t i = 0; i < sizeof(cfg.carNumber) && cfg.carNumber[i] != '\0'; i++) crc += cfg.carNumber[i];
	
	crc += cfg.mqttPort;
	crc += cfg.version;
	
	return crc;
}

} // namespace M360

#endif // ESP8266
