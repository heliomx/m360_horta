/*
 * sensorDrivers.cpp — Implementação do driver para nodeReles (Nó 99)
 *
 * Relés com optoacoplador: Active-LOW (LOW = Liga, HIGH = Desliga)
 *
 * Restrição de Concorrência MUX (proteção de fonte e rede elétrica):
 *   Ao receber comando para ligar um canal MUX, o canal anteriormente
 *   ativo é desligado automaticamente antes de selecionar o novo.
 *   Esta restrição NÃO se aplica aos pinos nativos D2 e D8.
 */

#include "sensorDrivers.h"
#include <DHT.h>

// ===== OBJETO DHT11 =====
static DHT dht(PIN_DHT, DHT11);

// ===== ESTADO INTERNO (rastreamento em software) =====
static int8_t s_activeMuxChannel    = -1;       // -1 = nenhum canal MUX ativo
static bool   s_muxChannelState[16] = {false};  // estado lógico de cada canal MUX

// ===== FUNÇÕES INTERNAS =====

/** Configura as 4 linhas de seleção S0-S3 para o canal MUX desejado. */
static void muxSelect(uint8_t ch)
{
	digitalWrite(MUX_S0_PIN, (ch & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_S1_PIN, (ch & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_S2_PIN, (ch & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_S3_PIN, (ch & 0x08) ? HIGH : LOW);
}

// ===== API PÚBLICA =====

void initSensors()
{
	// --- Pinos de controle do MUX ---
	pinMode(MUX_SIG_PIN, OUTPUT);
	pinMode(MUX_S0_PIN,  OUTPUT);
	pinMode(MUX_S1_PIN,  OUTPUT);
	pinMode(MUX_S2_PIN,  OUTPUT);
	pinMode(MUX_S3_PIN,  OUTPUT);

	// Seleciona canal 0 e coloca SIG em HIGH: todos os relés MUX desligados
	muxSelect(0);
	digitalWrite(MUX_SIG_PIN, HIGH);

	// --- Pinos nativos concorrentes ---
	pinMode(PIN_NFT_PUMP, OUTPUT);
	pinMode(PIN_NFT_OXI,  OUTPUT);
	digitalWrite(PIN_NFT_PUMP, HIGH); // relay OFF (Active-LOW)
	digitalWrite(PIN_NFT_OXI,  HIGH); // relay OFF (Active-LOW)

	// --- Inicialização do DHT11 ---
	dht.begin();
}

void writeNodeItem(uint8_t pin, bool state)
{
	if (IS_MUX_CH(pin)) {
		uint8_t ch = MUX_CH(pin);

		if (state) {
			// Restrição de concorrência: desligar canal MUX ativo (se diferente)
			if (s_activeMuxChannel >= 0 && s_activeMuxChannel != (int8_t)ch) {
				muxSelect((uint8_t)s_activeMuxChannel);
				digitalWrite(MUX_SIG_PIN, HIGH); // relay OFF
				s_muxChannelState[s_activeMuxChannel] = false;
			}
			// Selecionar e ligar o novo canal
			muxSelect(ch);
			digitalWrite(MUX_SIG_PIN, LOW); // relay ON
			s_activeMuxChannel = (int8_t)ch;
		} else {
			// Desligar somente se este for o canal atualmente ativo
			if (s_activeMuxChannel == (int8_t)ch) {
				muxSelect(ch);
				digitalWrite(MUX_SIG_PIN, HIGH); // relay OFF
				s_activeMuxChannel = -1;
			}
		}
		s_muxChannelState[ch] = state;

	} else {
		// Pino nativo: acionamento direto (Active-LOW)
		digitalWrite(pin, state ? LOW : HIGH);
	}
}

float readNodeItem(uint8_t pin)
{
	if (IS_MUX_CH(pin)) {
		// Estado de canais MUX é rastreado em software (SIG é compartilhado)
		return s_muxChannelState[MUX_CH(pin)] ? 1.0f : 0.0f;
	}
	// Pinos nativos: leitura direta (nível LOW = ligado)
	return digitalRead(pin) == LOW ? 1.0f : 0.0f;
}

float readDHTTemp()
{
	float t = dht.readTemperature();
	return isnan(t) ? NAN : t;
}

float readDHTHum()
{
	float h = dht.readHumidity();
	return isnan(h) ? NAN : h;
}
