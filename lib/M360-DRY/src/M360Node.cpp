/*
 * M360Node.cpp — Implementação do ciclo de vida M360-DRY
 */

#include "M360Node.h"
#include <math.h>

// Forward declarations of MySensors functions not defined in core headers
uint8_t getDistanceGW(void);
bool isTransportReady(void);

// ===== CONSTRUCTOR =====

namespace M360
{
	// ===== CONSTRUCTOR =====

	M360Node::M360Node(const M360ItemDef* items,
	                   uint8_t            count,
	                   MyMessage*         messages,
	                   float*             lastValues,
	                   uint8_t*           nNoUpdates,
	                   M360PowerProfile   profile)
	    : _items(items),
	      _count(count),
	      _messages(messages),
	      _lastValues(lastValues),
	      _nNoUpdates(nNoUpdates),
	      _interval(M360_DEFAULT_INTERVAL),
	      _lastBatt(0),
	      _battCycle(0),
	      _profile(profile),
	      _readCb(nullptr),
	      _writeCb(nullptr) {}

	// ===== CALLBACKS =====

	void M360Node::onRead(float (*callback)(uint8_t nodeIndex)) {
		_readCb = callback;
	}

	void M360Node::onWrite(void (*callback)(uint8_t nodeIndex, bool state)) {
		_writeCb = callback;
	}

	// ===== SETUP DE PINOS =====

	void M360Node::setupPins() {
		for (uint8_t i = 0; i < _count; i++) {
			if (_items[i].pin < 0) {
				continue;
			}
			if (_items[i].kind == M360_ACTUATOR) {
				pinMode(_items[i].pin, OUTPUT);
				digitalWrite(_items[i].pin, LOW);
			} else {
				pinMode(_items[i].pin, INPUT);
			}
		}
	}

	// ===== LIFECYCLE =====

	void M360Node::begin(const char* name, const char* version) {
		// Adiciona sufixo do perfil ao nome do sketch para fácil identificação no gateway
		char nameBuf[50];
		const char* suffix = "";
		switch (_profile) {
			case M360_LOW_POWER: suffix = " [LP]"; break;
			case M360_ALWAYS_ON: suffix = " [ON]"; break;
			case M360_PASSIVE:   suffix = " [PAS]"; break;
		}
		snprintf(nameBuf, sizeof(nameBuf), "%s%s", name, suffix);

		sendSketchInfo(nameBuf, version);

		for (uint8_t i = 0; i < _count; i++) {
			present(_items[i].childId,
			        (mysensors_sensor_t)_items[i].presentationType,
			        _items[i].label);
			_messages[i] = MyMessage(_items[i].childId,
			                         (mysensors_data_t)_items[i].valueType);
			_lastValues[i] = NAN;
			_nNoUpdates[i] = 0;
		}

		present(M360_CHILD_ID_INTERVAL, S_CUSTOM,     F("Intervalo"));
		present(M360_CHILD_ID_BATTERY,  S_MULTIMETER, F("Bateria"));
		_messages[_count]     = MyMessage(M360_CHILD_ID_INTERVAL, V_VAR1);
		_messages[_count + 1] = MyMessage(M360_CHILD_ID_BATTERY,  V_VOLTAGE);

		_interval = loadInterval();

		_printNetDiag();
	}

	// ===== PROCESS =====

	void M360Node::process() {
		if (_profile == M360_ALWAYS_ON) {
			// Modo 12V / always-on: timer por millis, sem sleep
			static unsigned long lastRun = 0;
			unsigned long now = millis();

			if (lastRun == 0 || (now - lastRun) >= (unsigned long)_interval * 60000UL) {
				lastRun = now;
				_readAndSendAll();
				_processBattery();
			}
			wait(50);
		} else if (_profile == M360_LOW_POWER) {
			// Modo low-power: lê, dorme, acorda
			powerUp();

			_readAndSendAll();
			_processBattery();

			powerDown();

			// Solicita estados dos atuadores antes de dormir
			for (uint8_t i = 0; i < _count; i++) {
				if (_items[i].kind == M360_ACTUATOR && _items[i].wakeOnRadio) {
					request(_items[i].childId, (mysensors_data_t)_items[i].valueType);
				}
			}

			wait(M360_MIN_AWAKE_MS);
			smartSleep((unsigned long)_interval * 60000UL);
		} else if (_profile == M360_PASSIVE) {
			// Modo passivo: APENAS dorme e aguarda mensagens (V_CUSTOM do gateway)
			// Não realiza leituras periódicas automáticas.
			smartSleep((unsigned long)_interval * 60000UL); // Intervalo longo por padrão
		}
	}

	// ===== HANDLE MESSAGE =====

	void M360Node::handleMessage(const MyMessage& msg) {
		if (msg.getCommand() != C_SET) {
			return;
		}

		// Intervalo via V_VAR1 ou V_VAR5
		if (msg.getSensor() == M360_CHILD_ID_INTERVAL &&
		    (msg.getType() == V_VAR1 || msg.getType() == V_VAR5)) {
			uint16_t iv = msg.getUInt();
			if (iv >= M360_MIN_INTERVAL && iv <= M360_MAX_INTERVAL) {
				_interval = iv;
				saveInterval(_interval);
				send(_messages[_count].set(_interval));
				Serial.print(F("INT:"));
				Serial.println(_interval);
			}
			return;
		}

		// Atuadores via V_STATUS
		if (msg.getType() == V_STATUS) {
			for (uint8_t i = 0; i < _count; i++) {
				if (msg.getSensor() == _items[i].childId &&
				    _items[i].kind == M360_ACTUATOR) {
					bool state = msg.getBool();
					if (_writeCb) {
						_writeCb(i, state);
					}
					send(_messages[i].set(state));
					break;
				}
			}
		}

		// Força atualização imediata
		if (msg.getType() == V_CUSTOM) {
			char buf[24];
			msg.getString(buf);
			if (strcmp(buf, M360_CMD_FORCE_UPDATE) == 0) {
				// No modo passivo, precisamos ligar e desligar periféricos explicitamente
				if (_profile == M360_PASSIVE) {
					powerUp();
				}
				_readAndSendAll();
				_processBattery();
				if (_profile == M360_PASSIVE) {
					powerDown();
				}
			}
		}
	}

	// ===== PRIVADOS =====

	void M360Node::_readAndSendAll() {
		if (!_readCb) {
			return;
		}

		for (uint8_t i = 0; i < _count; i++) {
			if (_items[i].kind != M360_SENSOR) {
				continue;
			}

			float val = _readCb(i);
			if (isnan(val) || val <= -32767.0f) {
				continue;
			}

			bool forceUpdate = (_nNoUpdates[i] >= 10);
			bool changed     = isnan(_lastValues[i]) || (fabsf(val - _lastValues[i]) > 0.05f);

			if (changed || forceUpdate) {
				_lastValues[i]  = val;
				_nNoUpdates[i]  = 0;
				if (_items[i].flags & 0x01) {
					// Usa int32_t para evitar overflow em valores > 327 (ex: tensões)
					send(_messages[i].set((int32_t)(val * 100)));
				} else {
					send(_messages[i].set(val, 1));
				}
			} else {
				_nNoUpdates[i]++;
			}
		}
	}

	void M360Node::_processBattery() {
		if (_profile == M360_ALWAYS_ON) {
			// Fonte fixa: reporta 100%
			send(_messages[_count + 1].set((uint8_t)100));
		} else {
			_battCycle++;
			if (_battCycle >= 10) {
				_battCycle = 0;
				uint8_t b = readBatteryPercent();
				int diff = (int)b - (int)_lastBatt;
				if (diff < 0) {
					diff = -diff;
				}
				if ((uint8_t)diff >= 1) {
					_lastBatt = b;
					send(_messages[_count + 1].set(b));
					Serial.print(F("Bat:"));
					Serial.println(b);
				}
			}
		}
	}

	void M360Node::_printNetDiag() {
		Serial.print(F("ID:"));
		Serial.println(getNodeId());
		Serial.print(F("Parent:"));
		Serial.println(getParentNodeId());
		Serial.print(F("Dist:"));
		Serial.println(getDistanceGW());
		Serial.print(F("Ready:"));
		Serial.println(isTransportReady() ? F("S") : F("N"));
	}
} // namespace M360
