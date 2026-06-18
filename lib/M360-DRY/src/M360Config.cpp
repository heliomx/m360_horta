/*
 * M360Config.cpp — Gerenciamento de EEPROM para M360-DRY (Infraestrutura)
 */

#ifdef ESP8266

#include "M360Config.h"
#include <EEPROM.h>

namespace M360 {

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
}

void Config::save(const M360DeviceConfig& cfg) {
	M360DeviceConfig temp = cfg;
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
	
	cfg.version = 1;
	strcpy(cfg.ssid, "ManualConfig");
	strcpy(cfg.password, "12345678");
	strcpy(cfg.mqttServer, "72.62.142.165");
	cfg.mqttPort = 1883;
	strcpy(cfg.mqttUser, "jmm");
	strcpy(cfg.mqttPassword, "jmmsqn");
	strcpy(cfg.uf, "DF");
	strcpy(cfg.carNumber, "0001");
	
	cfg.crc = calculateCRC(cfg);
}

bool Config::isValid(const M360DeviceConfig& cfg) {
	return (cfg.version != 0 && cfg.version != 0xFF && cfg.crc == calculateCRC(cfg));
}

uint16_t Config::calculateCRC(const M360DeviceConfig& cfg) {
	uint16_t crc = 0;
	
	// SSID
	for (unsigned int i = 0; i < strlen(cfg.ssid); i++) crc += cfg.ssid[i];
	// MQTT Server
	for (unsigned int i = 0; i < strlen(cfg.mqttServer); i++) crc += cfg.mqttServer[i];
	// UF
	for (unsigned int i = 0; i < strlen(cfg.uf); i++) crc += cfg.uf[i];
	// CAR Number
	for (unsigned int i = 0; i < strlen(cfg.carNumber); i++) crc += cfg.carNumber[i];
	
	crc += cfg.mqttPort;
	crc += cfg.version;
	
	return crc;
}

} // namespace M360

#endif // ESP8266
