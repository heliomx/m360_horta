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

// ===== ESTADO DO SENSOR DE VAZÃO =====
// Incrementado dentro da ISR — obrigatoriamente volatile.
static volatile uint32_t s_flowPulses    = 0;
// Instante da última leitura, base da janela de cálculo da vazão.
static uint32_t          s_flowLastReadMs = 0;

// ===== FUNÇÕES INTERNAS =====

/** Configura as 4 linhas de seleção S0-S3 para o canal MUX desejado. */
static void muxSelect(uint8_t ch)
{
	digitalWrite(MUX_S0_PIN, (ch & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_S1_PIN, (ch & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_S2_PIN, (ch & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_S3_PIN, (ch & 0x08) ? HIGH : LOW);
}

/**
 * ISR de contagem de pulsos do YF-S201.
 * Mantém-se mínima: apenas incrementa o contador.
 */
static void flowPulseISR()
{
	s_flowPulses++;
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

	// --- Sensor de vazão (interrupção) ---
	initFlowSensor();
}

int8_t writeNodeItem(uint8_t pin, bool state)
{
	int8_t preempted = -1; // canal desligado pela restrição de concorrência

	if (IS_MUX_CH(pin)) {
		uint8_t ch = MUX_CH(pin);

		if (state) {
			// Restrição de concorrência: desligar canal MUX ativo (se diferente)
			if (s_activeMuxChannel >= 0 && s_activeMuxChannel != (int8_t)ch) {
				preempted = s_activeMuxChannel; // devolvido ao chamador para o eco
				muxSelect((uint8_t)s_activeMuxChannel);
				digitalWrite(MUX_SIG_PIN, HIGH); // relay OFF
				s_muxChannelState[s_activeMuxChannel] = false;
				s_activeMuxChannel = -1; // atualiza antes de ativar novo canal
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

	return preempted;
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

// ===== SENSOR DE VAZÃO YF-S201 =====

void initFlowSensor()
{
	// Saída em coletor aberto: pull-up interno mantém a linha em HIGH em repouso.
	pinMode(PIN_FLOW, INPUT_PULLUP);
	s_flowPulses     = 0;
	s_flowLastReadMs = millis();
	attachInterrupt(digitalPinToInterrupt(PIN_FLOW), flowPulseISR, FALLING);
}

float readFlowLpm()
{
	// Captura e zera o contador com as interrupções desabilitadas: a leitura de
	// um uint32_t não é atômica no AVR de 8 bits, e um pulso chegando no meio
	// da operação corromperia o valor ou seria perdido no reset.
	noInterrupts();
	uint32_t pulses  = s_flowPulses;
	s_flowPulses     = 0;
	interrupts();

	uint32_t now     = millis();
	uint32_t elapsed = now - s_flowLastReadMs;  // seguro no overflow de millis()
	s_flowLastReadMs = now;

	// Janela curta demais não produz média confiável (1 pulso em 50 ms
	// extrapolaria para 2,7 L/min de ruído).
	if (elapsed < 100) {
		return 0.0f;
	}

	// F(Hz) = K x Q(L/min)  →  Q = (pulsos / Δt_s) / K
	float freqHz = (float)pulses * 1000.0f / (float)elapsed;
	return freqHz / FLOW_K_FACTOR;
}
