/*
 * M360Power.cpp — Implementação das funções de energia M360-DRY
 */

#include "M360Power.h"
#include <EEPROM.h>

namespace M360
{
	// ===== CALLBACKS DE ENERGIA (implementações padrão fracas) =====
	// O nó pode declarar suas próprias versões para sobrescrever.

	__attribute__((weak)) void powerDown() {
		// padrão: sem ação — o nó sobrescreve se necessário
	}

	__attribute__((weak)) void powerUp() {
		// padrão: sem ação — o nó sobrescreve se necessário
	}

	// ===== LEITURA DE BATERIA =====

	static uint16_t readADCInternal() {
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
		delay(2);
		ADCSRA |= _BV(ADSC);
		while (bit_is_set(ADCSRA, ADSC)) {
			;
		}
		return (uint16_t)((ADCH << 8) | ADCL);
#else
		return 0; // não suportado em plataformas não-AVR
#endif
	}

	float readBatteryVoltage() {
		uint16_t result = readADCInternal();
		if (result == 0) {
			return 0.0f;
		}
		// 1.1 * 1023 * 1000 = 1125300
		return (float)(1125300L / result) / 1000.0f;
	}

	uint8_t voltageToPercent(float voltage) {
		if (voltage >= M360_MAX_VOLTAGE) {
			return 100;
		}
		if (voltage <= M360_MIN_VOLTAGE) {
			return 0;
		}
		return (uint8_t)(((voltage - M360_MIN_VOLTAGE) / (M360_MAX_VOLTAGE - M360_MIN_VOLTAGE)) * 100.0f);
	}

	uint8_t readBatteryPercent() {
		return voltageToPercent(readBatteryVoltage());
	}

	// ===== EEPROM =====

	typedef struct {
		uint16_t magic;
		uint16_t interval;
	} M360EEPROM;

	void saveInterval(uint16_t interval) {
		M360EEPROM data;
		EEPROM.get(M360_EEPROM_INTERVAL_ADDRESS, data);
		if (data.magic != M360_EEPROM_MAGIC || data.interval != interval) {
			data.magic = M360_EEPROM_MAGIC;
			data.interval = interval;
			EEPROM.put(M360_EEPROM_INTERVAL_ADDRESS, data);
		}
	}

	uint16_t loadInterval() {
		M360EEPROM data;
		EEPROM.get(M360_EEPROM_INTERVAL_ADDRESS, data);

		// Novo formato: magic + interval
		if (data.magic == M360_EEPROM_MAGIC &&
		    data.interval >= M360_MIN_INTERVAL &&
		    data.interval <= M360_MAX_INTERVAL) {
			return data.interval;
		}

		// Migração: formato legado gravava apenas uint16_t no mesmo endereço
		uint16_t legacy;
		EEPROM.get(M360_EEPROM_INTERVAL_ADDRESS, legacy);
		if (legacy >= M360_MIN_INTERVAL && legacy <= M360_MAX_INTERVAL) {
			saveInterval(legacy);  // promove para novo formato com magic
			return legacy;
		}

		saveInterval(M360_DEFAULT_INTERVAL);
		return M360_DEFAULT_INTERVAL;
	}
} // namespace M360
