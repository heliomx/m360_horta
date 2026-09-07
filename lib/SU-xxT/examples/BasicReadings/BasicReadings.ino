/*
 * BasicReadings — varredura por índice no monitor serial
 *
 * Documentação executável da SU-xxT, sem MySensors e sem M360-DRY.
 * Não é compilado por nenhum env do projeto: o PlatformIO ignora exemplos de
 * biblioteca. A prova de compilação que vale é o env do nó real.
 *
 * ATENÇÃO — A PINAGEM ABAIXO É ILUSTRATIVA.
 *
 * A PCB SU é agnóstica quanto ao microcontrolador: ela expõe sinais (endereço do
 * MUX, excitação AC, comando do MOSFET, 1-Wire), e qual pino do MCU aciona cada
 * um é escolha de quem monta o nó. Não existe pinagem "da placa" a ser copiada
 * daqui.
 *
 * Num nó de produção, estes valores vivem no sensorDrivers.h do nó — convenção
 * de todos os nós do projeto. Ver ARCHITECTURE.md §13.
 */

#include <Arduino.h>
#include <SU_xxT.h>

#define PIN_MUX_A      5
#define PIN_MUX_B      6
#define PIN_MUX_C      7
#define PIN_AC_EXCITE  3
#define PIN_MOSFET     4
#define PIN_ONEWIRE    8

static SU_Device su(PIN_MUX_A, PIN_MUX_B, PIN_MUX_C,
                    PIN_AC_EXCITE,
                    PIN_MOSFET,
                    PIN_ONEWIRE,
                    SU_MODEL_30T);

void setup()
{
	Serial.begin(115200);
	while (!Serial && millis() < 3000) { }

	Serial.println(F("SU-xxT — BasicReadings"));

	if (!su.begin()) {
		// begin() devolve false se o ADS1115 não respondeu OU se a calibração
		// estava inválida. No segundo caso os defaults já foram aplicados e a
		// leitura segue — apenas não é confiável até calibrar em campo.
		Serial.println(F("AVISO: ADS1115 mudo ou calibracao invalida (defaults aplicados)"));
	}

	Serial.print(F("Modelo com "));
	Serial.print(su.getDeviceCount());
	Serial.println(F(" sensores:"));

	for (uint8_t i = 0; i < su.getDeviceCount(); i++) {
		Serial.print(F("  ["));
		Serial.print(i);
		Serial.print(F("] "));
		Serial.print(su.getLabelByIndex(i));
		Serial.print(F(" ("));
		Serial.print(su.getUnitByIndex(i));
		Serial.println(F(")"));
	}
}

void loop()
{
	su.powerUp();
	su.requestReadings();
	su.powerDown();

	Serial.println(F("---"));

	for (uint8_t i = 0; i < su.getDeviceCount(); i++) {
		const float v = su.getReadingByIndex(i);

		Serial.print(su.getLabelByIndex(i));
		Serial.print(F(": "));

		// SEMPRE testar com SU_isError() — nunca comparar float por igualdade.
		if (SU_isError(v)) {
			Serial.print(F("-- invalido ("));
			if      (v == SU_ERR_LEVEL_LOW)      Serial.print(F("camara seca"));
			else if (v == SU_ERR_STALE_RECHARGE) Serial.print(F("reserva nao trocada"));
			else if (v == SU_ERR_ADC_FAULT)      Serial.print(F("falha de ADC"));
			else if (v == SU_ERR_NOT_SAMPLED)    Serial.print(F("cache frio"));
			else                                 Serial.print(F("sensor ausente"));
			Serial.println(F(")"));
		} else {
			Serial.print(v, 1);
			Serial.print(' ');
			Serial.println(su.getUnitByIndex(i));
		}
	}

	// Diagnóstico da câmara: a idade da amostra é o eixo contra o qual a EC
	// deve ser trendada — EC subindo em fase com o contador é acúmulo de KCl,
	// não salinização do solo.
	Serial.print(F("camara: "));
	switch (su.getLevel()) {
		case SU_LEVEL_FILLED: Serial.print(F("cheia"));  break;
		case SU_LEVEL_DRY:    Serial.print(F("seca"));   break;
		default:              Serial.print(F("FALHA"));  break;
	}
	Serial.print(F(" | acomodada: "));
	Serial.print(su.isRechargeSettled() ? F("sim") : F("nao"));
	Serial.print(F(" | ciclos desde recarga: "));
	Serial.println(su.getCyclesSinceRecharge());

	delay(5000);
}
