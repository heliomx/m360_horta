/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
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
 * DESCRIPTION
 *
 * Arduino soil moisture based on gypsum sensor/resistive sensor to avoid electric catalyse in soil
 *  Required to interface the sensor: 2 * 4.7kOhm + 2 * 1N4148
 *
 * Gypsum sensor and calibration:
 *	DIY: See http://vanderleevineyard.com/1/category/vinduino/1.html
 *	Built: Davis / Watermark 200SS
 *		http://www.cooking-hacks.com/watermark-soil-moisture-sensor?_bksrc=item2item&_bkloc=product
 *		http://www.irrometer.com/pdf/supportmaterial/sensors/voltage-WM-chart.pdf
 *		cb (centibar) http://www.irrometer.com/basics.html
 *			0-10 Saturated Soil. Occurs for a day or two after irrigation
 *			10-20 Soil is adequately wet (except coarse sands which are drying out at this range)
 *			30-60 Usual range to irrigate or water (except heavy clay soils).
 *			60-100 Usual range to irrigate heavy clay soils
 *			100-200 Soil is becoming dangerously dry for maximum production. Proceed with caution.
 *
 * Connection:
 * D6, D7: alternative powering to avoid sensor degradation
 * A0, A1: alternative resistance measuring
 *
 *  Based on:
 *  "Vinduino" portable soil moisture sensor code V3.00
 *   Date December 31, 2012
 *   Reinier van der Lee and Theodore Kaskalis
 *   www.vanderleevineyard.com
 * Contributor: epierre
 */

// Copyright (C) 2015, Reinier van der Lee
// www.vanderleevineyard.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Enable debug prints to serial monitor
#ifndef MY_DEBUG
#define MY_DEBUG
#endif

// Enable and select radio type attached
#ifndef MY_RADIO_RF24
#define MY_RADIO_RF24
#endif
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95
//#define MY_PJON

#include <math.h>       // Conversion equation from resistance to %
#include <MySensors.h>

// Pinos do Multiplexador CD74HC4067
#define MUX_PIN_SIG A0
#define MUX_PIN_S0  4
#define MUX_PIN_S1  5
#define MUX_PIN_S2  6
#define MUX_PIN_S3  7

// Pino de Energia dos Pull-ups (Mitigação de Eletrólise)
#define PIN_POWER_SENSORS 3 // D3

// Pinos Nativos
#define PIN_NATIVE_A1 A1
#define PIN_NATIVE_A2 A2

#define NUM_SENSORS 18
uint32_t SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

MyMessage msg(0, V_LEVEL);

void initSensors()
{
	// Configura o pino de energia (pull-ups) como saída e desliga por padrão
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW);
	
	// Configura os pinos de controle do MUX como saída
	pinMode(MUX_PIN_S0, OUTPUT);
	pinMode(MUX_PIN_S1, OUTPUT);
	pinMode(MUX_PIN_S2, OUTPUT);
	pinMode(MUX_PIN_S3, OUTPUT);
	
	// Configura os pinos de leitura analógica como entrada
	pinMode(MUX_PIN_SIG, INPUT);
	pinMode(PIN_NATIVE_A1, INPUT);
	pinMode(PIN_NATIVE_A2, INPUT);
}

void powerUpSensors()
{
	// Energiza a barra de resistores de pull-up dos sensores se estiver desligada
	if (digitalRead(PIN_POWER_SENSORS) == LOW) {
		digitalWrite(PIN_POWER_SENSORS, HIGH);
		// Tempo de estabilização do MUX e das capacitâncias parasitas nos cabos longos
		delay(20);
	}
}

void powerDownSensors()
{
	// Desliga a alimentação para cessar corrente e evitar eletrólise nos eletrodos
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

void selectMuxChannel(uint8_t channel)
{
	digitalWrite(MUX_PIN_S0, (channel & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S1, (channel & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S2, (channel & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S3, (channel & 0x08) ? HIGH : LOW);
}

float readSensor(uint8_t itemIndex)
{
	// Se for o início da varredura, garante que os pull-ups estejam energizados
	if (itemIndex == 0) {
		powerUpSensors();
	}

	uint8_t pinToRead = MUX_PIN_SIG;
	float rOn = 0.0f; // Sem multiplexador por padrão (canais nativos)
	
	// Mapeia o índice para o canal correto
	if (itemIndex < 16) {
		selectMuxChannel(itemIndex);
		rOn = 70.0f; // Resistência interna típica (Ron) do CD74HC4067
	} else if (itemIndex == 16) {
		pinToRead = PIN_NATIVE_A1;
	} else if (itemIndex == 17) {
		pinToRead = PIN_NATIVE_A2;
	} else {
		return NAN; // Índice inválido
	}

	// Pequeno delay para estabilização elétrica do canal selecionado
	delay(5);

	// Primeira leitura para purgar a carga acumulada no Sample and Hold do ADC
	analogRead(pinToRead);
	delay(3);
	
	// Leitura real
	int rawAdc = analogRead(pinToRead);

	// Compensação de divisor de tensão (Opção B - pull-up de 10k único no SIG)
	float rPullup = 10000.0f; // Pull-up de 10kΩ
	
	if (rawAdc >= 1023) {
		rawAdc = 1022; // Evita divisão por zero
	}
	
	// Calcula a resistência real do solo descontando Ron do multiplexador
	float rSolo = rPullup * ((float)rawAdc / (1023.0f - (float)rawAdc)) - rOn;
	if (rSolo < 0.0f) {
		rSolo = 0.0f;
	}

	// Converte para porcentagem equivalente: R_pullup / (R_pullup + R_solo) * 100.0%
	float percentage = (rPullup / (rPullup + rSolo)) * 100.0f;
	
	// Limita o valor final entre 0 e 100
	if (percentage < 0.0f) {
		percentage = 0.0f;
	}
	if (percentage > 100.0f) {
		percentage = 100.0f;
	}

	// Ao final da varredura (último sensor, index 17), desliga os sensores
	if (itemIndex == 17) {
		powerDownSensors();
	}

	return percentage;
}

void setup()
{
	initSensors();
}

void presentation()
{
	sendSketchInfo("Soil Moisture Sensor Multiplexed", "1.0");
	for (int sensor = 0; sensor < NUM_SENSORS; sensor++) {
		present(sensor, S_MOISTURE);
	}
}

void loop()
{
	for (int sensor = 0; sensor < NUM_SENSORS; sensor++) {
		float moisture = readSensor(sensor);
		if (!isnan(moisture)) {
			Serial.print("Sensor ");
			Serial.print(sensor);
			Serial.print(": ");
			Serial.print(moisture, 1);
			Serial.println("%");
			
			// Envia o valor como float com 1 casa decimal (conforme convenções)
			send(msg.setSensor(sensor).set(moisture, 1));
		}
		// Pequeno delay entre transmissões para não sobrecarregar o rádio
		wait(50);
	}
	Serial.println();
	
	// Dorme até a próxima medição (msec)
	sleep(SLEEP_TIME);
}