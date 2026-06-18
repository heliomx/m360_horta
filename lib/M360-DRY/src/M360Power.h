/*
 * M360Power.h — Gerenciamento de energia para nós M360-DRY
 *
 * Interface de callbacks que cada nó pode sobrescrever para
 * controlar periféricos antes/após o sleep.
 */

#pragma once
#include <Arduino.h>
#include "M360Config.h"

// ===== INTERFACE DE CALLBACKS DE ENERGIA =====
// Implemente estas funções no arquivo do nó se precisar
// controlar periféricos (sensores, relés, etc.) durante o sleep.
// Implementações padrão (vazias) são fornecidas em M360Power.cpp.

namespace M360
{
	// Chamada antes do sleep — desliga periféricos.
	//
	// IMPORTANTE: Para sobrescrever, implemente DENTRO do namespace M360:
	//   namespace M360 { void powerDown() { /* seu código */ } }
	// Implementar como função global (fora do namespace) NÃO sobrescreve
	// o símbolo fraco — o linker os trata como símbolos distintos.
	void powerDown();

	// Chamada ao acordar — liga periféricos.
	// Mesma regra de namespace acima se aplica.
	void powerUp();

	// ===== FUNÇÕES INTERNAS DA BIBLIOTECA =====

	// Leitura de tensão via ADC interno (apenas AVR ATmega328P/168)
	float readBatteryVoltage();

	// Converte tensão em porcentagem (MIN_VOLTAGE=0%, MAX_VOLTAGE=100%)
	uint8_t voltageToPercent(float voltage);

	// Lê tensão e retorna porcentagem
	uint8_t readBatteryPercent();

	// Persiste intervalo na EEPROM (só escreve se mudou)
	void saveInterval(uint16_t interval);

	// Carrega intervalo da EEPROM; retorna DEFAULT se inválido
	uint16_t loadInterval();
} // namespace M360
