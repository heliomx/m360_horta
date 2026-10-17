/*
 * M360FactoryReset.h — Factory reset do gateway pelo reed/botão do A0 (ESP8266)
 *
 * O ímã (M360-GW) ou o botão (protótipo, Kit Hélio) puxa o A0 para o GND. No
 * boot isso só abre o modo AP (o sketch lê o A0 uma vez, no before()). Este
 * vigia acrescenta o factory reset:
 *
 *   - janela de `windowMs` (30 s) contada da PRIMEIRA chamada de process(), ou
 *     seja, de quando o loop() começa — a conexão WiFi no before() pode levar
 *     mais que 30 s e a janela venceria antes de o loop() rodar;
 *   - só arma depois de ver o A0 solto (alto) pelo menos uma vez: quem segura o
 *     ímã no boot para entrar no modo AP não apaga a configuração sem querer —
 *     é preciso afastar e aproximar de novo;
 *   - A0 baixo por `holdMs` (3 s) seguidos → Config::reset() (valores de
 *     fábrica do M360Credentials.h) + Config::save() + callback + ESP.restart().
 *
 * Depois do reset o gateway reinicia com a configuração de fábrica. Se o WiFi de
 * fábrica não responder, o M360WiFi cai sozinho no modo AP após 3 tentativas;
 * para forçar o AP de imediato, reiniciar com o ímã no A0.
 *
 * O A0 é amostrado a cada 100 ms: analogRead() em laço apertado derruba o WiFi
 * do ESP8266. Fora da janela, process() não lê nada.
 */

#pragma once

#ifndef ESP8266
#  error "M360FactoryReset.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>
#include <functional>
#include "M360Config.h"

namespace M360
{
	class FactoryReset {
	public:
		explicit FactoryReset(uint8_t pin, unsigned long windowMs = 30000UL,
		                      unsigned long holdMs = 3000UL, int threshold = 400);

		// Chamado logo antes do restart — ex.: publicar o evento no MQTT.
		void onTrigger(std::function<void()> callback);

		// Chamar a cada volta do loop(). Grava e reinicia se o critério fechar.
		void process(M360DeviceConfig& cfg);

	private:
		uint8_t       _pin;
		unsigned long _windowMs;
		unsigned long _holdMs;
		int           _threshold;
		unsigned long _start;      // 0 = process() ainda não rodou
		unsigned long _lastSample;
		unsigned long _lowSince;   // 0 = A0 alto
		bool          _armed;      // A0 já foi visto alto
		bool          _expired;
		std::function<void()> _callback;
	};
} // namespace M360
