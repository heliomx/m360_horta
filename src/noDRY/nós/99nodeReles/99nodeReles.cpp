/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * Example sketch showing how to control physical relays.
 * This example will remember relay state after power failure.
 * http://www.mysensors.org/build/relay
 */

// Enable debug prints to serial monitor
#ifndef MY_DEBUG
#define MY_DEBUG
#endif

// Enable and select radio type attached
#ifndef MY_RADIO_RF24
#define MY_RADIO_RF24
#endif
// #define MY_RADIO_NRF5_ESB
// #define MY_RADIO_RFM69
// #define MY_RADIO_RFM95
// #define MY_PJON

// Enable repeater functionality for this node
#ifndef MY_REPEATER_FEATURE
#define MY_REPEATER_FEATURE
#endif

#include <MySensors.h>
#include <DHT.h>

// ===== PINOS MUX CD74HC4067 =====
#define MUX_SIG_PIN     3
#define MUX_S0_PIN      4
#define MUX_S1_PIN      5
#define MUX_S2_PIN      6
#define MUX_S3_PIN      7

// ===== PINOS NATIVOS CONCORRENTES =====
#define PIN_NFT_PUMP    2   // Bomba Circulação Principal — Hidroponia NFT
#define PIN_NFT_OXI     8   // Bomba Oxigenação — Hidroponia NFT

// ===== CHILD IDs =====
#define CHILD_ID_SOL_A      0   // Canal MUX 0
#define CHILD_ID_SOL_B      1   // Canal MUX 1
#define CHILD_ID_SOL_C      2   // Canal MUX 2
#define CHILD_ID_PERIST_A   3   // Canal MUX 3
#define CHILD_ID_PERIST_B   4   // Canal MUX 4
#define CHILD_ID_PH_PLUS    5   // Canal MUX 5
#define CHILD_ID_PH_MINUS   6   // Canal MUX 6

#define CHILD_ID_NFT_PUMP   16  // Pino nativo D2
#define CHILD_ID_NFT_OXI    17  // Pino nativo D8
#define CHILD_ID_DHT_TEMP   18  // Temperatura DHT11
#define CHILD_ID_DHT_HUM    19  // Umidade DHT11

// ===== PINO NATIVO DHT11 =====
#define PIN_DHT             A0

static DHT dht(PIN_DHT, DHT11);

// ===== ESTADO INTERNO =====
static int8_t s_activeMuxChannel    = -1;       // -1 = nenhum canal MUX ativo
static bool   s_muxChannelState[16] = {false};  // estado lógico de cada canal MUX

// ===== CONFIGURAÇÃO PERIÓDICA =====
#define DHT_SEND_INTERVAL   30000UL             // Intervalo de leitura do DHT (30s)
static unsigned long lastDhtSend = 0;

MyMessage msgTemp(CHILD_ID_DHT_TEMP, V_TEMP);
MyMessage msgHum(CHILD_ID_DHT_HUM, V_HUM);

static void selectMuxChannel(uint8_t ch)
{
	digitalWrite(MUX_S0_PIN, (ch & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_S1_PIN, (ch & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_S2_PIN, (ch & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_S3_PIN, (ch & 0x08) ? HIGH : LOW);
}

void writeRelay(uint8_t childId, bool state)
{
	if (childId >= 0 && childId <= 6) {
		uint8_t ch = childId;

		if (state) {
			// Restrição de concorrência: desliga canal MUX ativo (se diferente)
			if (s_activeMuxChannel >= 0 && s_activeMuxChannel != (int8_t)ch) {
				selectMuxChannel((uint8_t)s_activeMuxChannel);
				digitalWrite(MUX_SIG_PIN, HIGH); // relay OFF (Active-LOW)
				s_muxChannelState[s_activeMuxChannel] = false;
				saveState(s_activeMuxChannel, false);
				s_activeMuxChannel = -1;
			}
			// Selecionar e ligar o novo canal
			selectMuxChannel(ch);
			digitalWrite(MUX_SIG_PIN, LOW); // relay ON (Active-LOW)
			s_activeMuxChannel = (int8_t)ch;
		} else {
			// Desligar somente se for o canal ativo
			if (s_activeMuxChannel == (int8_t)ch) {
				selectMuxChannel(ch);
				digitalWrite(MUX_SIG_PIN, HIGH); // relay OFF (Active-LOW)
				s_activeMuxChannel = -1;
			}
		}
		s_muxChannelState[ch] = state;
		saveState(childId, state);

	} else if (childId == CHILD_ID_NFT_PUMP) {
		digitalWrite(PIN_NFT_PUMP, state ? LOW : HIGH); // Active-LOW
		saveState(childId, state);
	} else if (childId == CHILD_ID_NFT_OXI) {
		digitalWrite(PIN_NFT_OXI, state ? LOW : HIGH); // Active-LOW
		saveState(childId, state);
	}
}

void before()
{
	// --- Pinos de controle do MUX ---
	pinMode(MUX_SIG_PIN, OUTPUT);
	pinMode(MUX_S0_PIN,  OUTPUT);
	pinMode(MUX_S1_PIN,  OUTPUT);
	pinMode(MUX_S2_PIN,  OUTPUT);
	pinMode(MUX_S3_PIN,  OUTPUT);

	// Desliga todos os relés do MUX por padrão
	selectMuxChannel(0);
	digitalWrite(MUX_SIG_PIN, HIGH);

	// --- Pinos nativos concorrentes ---
	pinMode(PIN_NFT_PUMP, OUTPUT);
	pinMode(PIN_NFT_OXI,  OUTPUT);
	digitalWrite(PIN_NFT_PUMP, HIGH); // relay OFF (Active-LOW)
	digitalWrite(PIN_NFT_OXI,  HIGH); // relay OFF (Active-LOW)

	// --- Inicialização do DHT11 ---
	dht.begin();

	// --- Restauração de estados anteriores ---
	bool statePump = loadState(CHILD_ID_NFT_PUMP);
	digitalWrite(PIN_NFT_PUMP, statePump ? LOW : HIGH);

	bool stateOxi = loadState(CHILD_ID_NFT_OXI);
	digitalWrite(PIN_NFT_OXI, stateOxi ? LOW : HIGH);

	int8_t activeCh = -1;
	for (uint8_t ch = 0; ch <= 6; ch++) {
		bool state = loadState(ch);
		s_muxChannelState[ch] = state;
		if (state) {
			if (activeCh == -1) {
				activeCh = ch;
			} else {
				// Corrige caso a EEPROM salve estados inconsistentes
				s_muxChannelState[ch] = false;
				saveState(ch, false);
			}
		}
	}

	if (activeCh >= 0) {
		selectMuxChannel((uint8_t)activeCh);
		digitalWrite(MUX_SIG_PIN, LOW); // relay ON (Active-LOW)
		s_activeMuxChannel = activeCh;
	}
}

void setup() {}

void presentation()
{
	sendSketchInfo("Greenhouse Actuators & Climate", "1.0");

	// Apresenta atuadores MUX (0 a 6)
	present(CHILD_ID_SOL_A, S_BINARY, "Sol.CanteiroA");
	present(CHILD_ID_SOL_B, S_BINARY, "Sol.CanteiroB");
	present(CHILD_ID_SOL_C, S_BINARY, "Sol.CanteiroC");
	present(CHILD_ID_PERIST_A, S_BINARY, "Perist.SuplA");
	present(CHILD_ID_PERIST_B, S_BINARY, "Perist.SuplB");
	present(CHILD_ID_PH_PLUS, S_BINARY, "Perist.pH+");
	present(CHILD_ID_PH_MINUS, S_BINARY, "Perist.pH-");

	// Apresenta atuadores nativos (16 e 17)
	present(CHILD_ID_NFT_PUMP, S_BINARY, "BombaNFT");
	present(CHILD_ID_NFT_OXI, S_BINARY, "BombaOxi");

	// Apresenta sensores DHT (18 e 19)
	present(CHILD_ID_DHT_TEMP, S_TEMP, "TempReles");
	present(CHILD_ID_DHT_HUM, S_HUM, "UmidReles");
}

void loop()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastDhtSend >= DHT_SEND_INTERVAL || lastDhtSend == 0) {
		lastDhtSend = currentMillis;

		float temp = dht.readTemperature();
		if (!isnan(temp)) {
			Serial.print("Temperatura: ");
			Serial.print(temp, 1);
			Serial.println("C");
			send(msgTemp.set(temp, 1));
		}

		float hum = dht.readHumidity();
		if (!isnan(hum)) {
			Serial.print("Umidade: ");
			Serial.print(hum, 1);
			Serial.println("%");
			send(msgHum.set(hum, 1));
		}
	}
}

void receive(const MyMessage &message)
{
	if (message.getType() == V_STATUS) {
		uint8_t childId = message.getSensor();
		bool state = message.getBool();
		writeRelay(childId, state);

		Serial.print("Comando recebido para o sensor: ");
		Serial.print(childId);
		Serial.print(", Novo estado: ");
		Serial.println(state);
	}
}