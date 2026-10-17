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
 *   - A0 baixo por `holdMs` (3 s) seguidos → trigger(): callback (ex.: publicar o evento
 *     factory_reset, ainda com o prefixo antigo) → APAGA a M360DeviceConfig
 *     (zerada e gravada: version 0 é inválida) → ESP.restart().
 *
 * No reboot o before() acha a configuração inválida e sobe direto no modo AP
 * (M360-Config, 192.168.4.1) — o mesmo caminho de um gateway novo. Nenhum dado
 * da instalação anterior (WiFi, broker, UF/CAR) sobrevive: é o requisito RF-PRV-09
 * da especificação do backend, para reaproveitar o gateway em outra propriedade.
 * (O before() grava os padrões do M360Credentials.h ao entrar no AP; se ninguém
 * configurar pelo portal, o boot seguinte usa esses padrões, como um gateway novo.)
 *
 * O mesmo trigger() atende o botão "Restaurar padrões de fábrica" do portal web
 * (M360Webserver, POST /reset com confirmação), sem janela nem ímã.
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

		// Chamado logo antes de apagar e reiniciar — ex.: publicar o evento no MQTT.
		// `origem` diz quem disparou: "reed/botao A0" ou "portal web".
		void onTrigger(std::function<void(const char* origem)> callback);

		// Executa o factory reset agora: callback → apaga a configuração → restart
		// (sobe em AP). Usado pelo vigia do A0 e pelo botão do portal (POST /reset).
		// Não retorna.
		void trigger(M360DeviceConfig& cfg, const char* origem);

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
		std::function<void(const char*)> _callback;
	};
} // namespace M360
