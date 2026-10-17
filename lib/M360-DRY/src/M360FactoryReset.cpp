/*
 * M360FactoryReset.cpp — Factory reset do gateway pelo A0 (ver o .h)
 */

#ifdef ESP8266

#include "M360FactoryReset.h"

namespace M360
{
	FactoryReset::FactoryReset(uint8_t pin, unsigned long windowMs,
	                           unsigned long holdMs, int threshold)
	    : _pin(pin),
	      _windowMs(windowMs),
	      _holdMs(holdMs),
	      _threshold(threshold),
	      _start(0),
	      _lastSample(0),
	      _lowSince(0),
	      _armed(false),
	      _expired(false),
	      _callback(nullptr) {}

	void FactoryReset::onTrigger(std::function<void()> callback) {
		_callback = callback;
	}

	void FactoryReset::process(M360DeviceConfig& cfg) {
		if (_expired) {
			return;
		}
		const unsigned long now = millis();
		if (_start == 0) {
			_start = now | 1UL;
			Serial.println(F("FRESET: janela de 30 s aberta (afaste e aproxime o ímã, 3 s)"));
		}
		if (now - _start >= _windowMs) {
			_expired = true;
			Serial.println(F("FRESET: janela encerrada"));
			return;
		}
		if (now - _lastSample < 100UL) {
			return;
		}
		_lastSample = now;

		const bool low = analogRead(_pin) < _threshold;
		if (!low) {
			_armed    = true;
			_lowSince = 0;
			return;
		}
		if (!_armed) {
			return; // ímã mantido desde o boot: é o modo AP, não o reset
		}
		if (_lowSince == 0) {
			_lowSince = now | 1UL;
			Serial.println(F("FRESET: A0 baixo — mantenha 3 s para o factory reset"));
			return;
		}
		if (now - _lowSince < _holdMs) {
			return;
		}

		Serial.println(F("FRESET: restaurando a configuração de fábrica e reiniciando"));
		Config::reset(cfg);
		Config::save(cfg);
		if (_callback) {
			_callback();
		}
		delay(500);
		ESP.restart();
	}
} // namespace M360

#endif // ESP8266
