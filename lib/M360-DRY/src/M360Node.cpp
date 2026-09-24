/*
 * M360Node.cpp — Implementação do ciclo de vida M360-DRY
 */

#include "M360Node.h"
#include "M360Constants.h"
#include <math.h>

// Forward declarations of MySensors functions not defined in core headers
uint8_t getDistanceGW(void);
bool isTransportReady(void);

namespace M360
{
	// ===== CONSTRUCTOR =====

	M360Node::M360Node(const M360ItemDef* items,
	                   uint8_t            count,
	                   float*             lastValues,
	                   uint8_t*           nNoUpdates,
	                   M360PowerProfile   profile)
	    : _items(items),
	      _count(count),
	      _lastValues(lastValues),
	      _nNoUpdates(nNoUpdates),
	      _interval(M360_DEFAULT_INTERVAL),
	      _intervalMs(0),
	      _lastBattVoltage(NAN),
	      _battCycle(0),
	      _profile(profile),
	      _readCb(nullptr),
	      _writeCb(nullptr) {}

	// ===== BUFFER DE ENVIO =====

	// Remonta o cabeçalho no único buffer de envio — o mesmo MyMessage(child, tipo)
	// que begin() construía uma vez por item quando havia messages[count + 3].
	MyMessage& M360Node::_msgFor(uint8_t childId, uint8_t valueType) {
		_txMsg = MyMessage(childId, (mysensors_data_t)valueType);
		return _txMsg;
	}

	void M360Node::sendItemState(uint8_t index, bool state) {
		if (index >= _count) {
			return;
		}
		send(_msgFor(_items[index].childId, _items[index].valueType).set(state));
	}

	// ===== CALLBACKS =====

	void M360Node::onRead(float (*callback)(uint8_t nodeIndex)) {
		_readCb = callback;
	}

	void M360Node::onWrite(void (*callback)(uint8_t nodeIndex, bool state)) {
		_writeCb = callback;
	}

	void M360Node::sendDebug(const char* text) {
		send(_msgFor(M360_CHILD_ID_DEBUG, V_TEXT).set(text));
	}

	// ===== CONTROLE DE INTERVALO EM RUNTIME =====

	// Mesmo eco que handleMessage() faz ao aceitar um C_SET: o Node-RED usa esse
	// valor como cadência esperada do nó, em vez de aprender por média móvel.
	void M360Node::_sendInterval() {
		send(_msgFor(M360_CHILD_ID_INTERVAL, V_VAR1).set(_interval));
	}

	bool M360Node::setInterval(uint16_t minutes) {
		if (minutes < M360_MIN_INTERVAL || minutes > M360_MAX_INTERVAL) {
			Serial.print(F("INT_LOC:REJ:"));
			Serial.println(minutes);
			return false;
		}
		_interval   = minutes;
		_intervalMs = 0; // cancela qualquer override em ms
		_sendInterval();
		// INT_LOC = ajuste local (nó); INT: é o caminho da rede em handleMessage()
		Serial.print(F("INT_LOC:"));
		Serial.println(_interval);
		return true;
	}

	bool M360Node::setIntervalMs(uint32_t ms) {
		if (_profile == M360_ALWAYS_ON || _profile == M360_REPEATER) {
			// process() desses perfis usa só _interval no timer por millis();
			// aceitar aqui seria um no-op silencioso com log enganoso.
			Serial.println(F("INT_MS:IGN"));
			return false;
		}
		if (ms > 0 && (ms < M360_MIN_INTERVAL_MS ||
		               ms > (uint32_t)M360_MAX_INTERVAL * 60000UL)) {
			Serial.print(F("INT_MS:REJ:"));
			Serial.println(ms);
			return false;
		}
		const bool hadOverride = (_intervalMs > 0);
		_intervalMs = ms;
		if (ms > 0) {
			Serial.print(F("INT_MS:"));
			Serial.println(ms);
		} else if (hadOverride) {
			// Fim do override: volta ao intervalo em minutos e avisa o controlador.
			_sendInterval();
			Serial.print(F("INT_MS:OFF->"));
			Serial.println(_interval);
		}
		return true;
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

	void M360Node::begin(const __FlashStringHelper* name, const __FlashStringHelper* version) {
		// Adiciona sufixo do perfil ao nome do sketch para fácil identificação no gateway.
		// Nome, versão e sufixo vêm da Flash e só existem em RAM aqui, na pilha.
		char nameBuf[50];
		char verBuf[MAX_PAYLOAD_SIZE + 1];
		PGM_P suffix = PSTR("");
		switch (_profile) {
			case M360_LOW_POWER: suffix = PSTR(" [LP]");  break;
			case M360_ALWAYS_ON: suffix = PSTR(" [ON]");  break;
			case M360_PASSIVE:   suffix = PSTR(" [PAS]"); break;
			case M360_REPEATER:  suffix = PSTR(" [REP]"); break;
		}
		// Reserva lugar para o maior sufixo (6 caracteres) e o terminador.
		strncpy_P(nameBuf, reinterpret_cast<PGM_P>(name), sizeof(nameBuf) - 7);
		nameBuf[sizeof(nameBuf) - 7] = '\0';
		strcat_P(nameBuf, suffix);
		strncpy_P(verBuf, reinterpret_cast<PGM_P>(version), sizeof(verBuf) - 1);
		verBuf[sizeof(verBuf) - 1] = '\0';

		sendSketchInfo(nameBuf, verBuf);

		for (uint8_t i = 0; i < _count; i++) {
			present(_items[i].childId,
			        (mysensors_sensor_t)_items[i].presentationType,
			        _items[i].label);
			_lastValues[i] = NAN;
			_nNoUpdates[i] = 0;
		}

		present(M360_CHILD_ID_INTERVAL, S_CUSTOM,     F("Intervalo"));
		present(M360_CHILD_ID_BATTERY,  S_MULTIMETER, F("Bateria"));
		present(M360_CHILD_ID_DEBUG,    S_INFO,       F("Debug"));

		_interval = loadInterval();

		_printNetDiag();
	}

	// ===== PROCESS =====

	void M360Node::process() {
		if (_profile == M360_ALWAYS_ON || _profile == M360_REPEATER) {
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
			uint32_t sleepMs = (_intervalMs > 0)
			                 ? _intervalMs
			                 : (uint32_t)_interval * 60000UL;
			smartSleep(sleepMs);
		} else if (_profile == M360_PASSIVE) {
			// Modo passivo: APENAS dorme e aguarda mensagens (V_CUSTOM do gateway)
			// Não realiza leituras periódicas automáticas.
			uint32_t sleepMs = (_intervalMs > 0)
			                 ? _intervalMs
			                 : (uint32_t)_interval * 60000UL;
			smartSleep(sleepMs); // Intervalo longo por padrão
		}
	}

	// ===== HANDLE MESSAGE =====

	void M360Node::handleMessage(const MyMessage& msg) {
		// Ecos são devolvidos pelo destino quando requestEcho=true — não processar
		if (msg.isEcho()) {
			return;
		}

		// C_REQ: controlador pediu o valor atual de um sensor (ex: "atualizar agora"
		// no Domoticz/Home Assistant) — responde com o último valor em cache.
		if (msg.getCommand() == C_REQ) {
			for (uint8_t i = 0; i < _count; i++) {
				if (_items[i].kind == M360_SENSOR && msg.getSensor() == _items[i].childId) {
					_sendCachedValue(i);
					break;
				}
			}
			return;
		}

		if (msg.getCommand() != C_SET) {
			return;
		}

		// Intervalo via V_VAR1 ou V_VAR5
		if (msg.getSensor() == M360_CHILD_ID_INTERVAL &&
		    (msg.getType() == V_VAR1 || msg.getType() == V_VAR5)) {
			uint16_t iv = msg.getUInt();
			if (iv >= M360_MIN_INTERVAL && iv <= M360_MAX_INTERVAL) {
				_interval   = iv;
				_intervalMs = 0; // cancela warm-up: comando externo tem precedência
				saveInterval(_interval);
			}
			// Sempre confirma o valor vigente (gateway sabe se a mudança foi aceita ou rejeitada)
			_sendInterval();
			Serial.print(F("INT:"));
			Serial.println(_interval);
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
					sendItemState(i, state);
					break;
				}
			}
		}

		// Comandos V_CUSTOM: REPRESENT (reapresenta sensores) e FORCE_UPDATE (leitura imediata)
		if (msg.getType() == V_CUSTOM) {
			// MAX_PAYLOAD_SIZE do MySensors é 25 bytes; buf[25] comporta string de 24 chars + \0
			if (msg.getLength() >= 25) {
				return;
			}
			char buf[25];
			msg.getString(buf);
			if (strcmp_P(buf, PSTR(M360_TXT_REPRESENT)) == 0) {
				_rePresent();
				return;
			}
			if (strcmp_P(buf, PSTR(M360_TXT_FORCE_UPDATE)) == 0) {
				// No modo passivo, precisamos ligar e desligar periféricos explicitamente
				if (_profile == M360_PASSIVE) {
					powerUp();
				}
				_readAndSendAll(true); // ignora o filtro de "mudou o suficiente"
				_processBattery();
				if (_profile == M360_PASSIVE) {
					powerDown();
				}
				return;
			}
			if (strcmp_P(buf, PSTR(M360_TXT_DEBUG_NET)) == 0) {
				// Envia o mesmo diagnóstico de _printNetDiag(), mas pela rede em
				// vez do Serial — permite consultar parent/dist/ready remotamente.
				char diag[25];
				snprintf_P(diag, sizeof(diag), PSTR("P:%u D:%u R:%c"),
				           getParentNodeId(), getDistanceGW(),
				           isTransportReady() ? 'S' : 'N');
				sendDebug(diag);
				return;
			}
		}
	}

	// ===== PRIVADOS =====

	void M360Node::_readAndSendAll(bool forceAll) {
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

			bool staleForced = (_nNoUpdates[i] >= 10);
			bool changed     = isnan(_lastValues[i]) || (fabsf(val - _lastValues[i]) > 0.05f);

			if (forceAll || changed || staleForced) {
				_lastValues[i]  = val;
				_nNoUpdates[i]  = 0;
				_sendCachedValue(i);
			} else {
				_nNoUpdates[i]++;
			}
		}
	}

	void M360Node::_sendCachedValue(uint8_t i) {
		if (isnan(_lastValues[i])) {
			return;
		}
		MyMessage& msg = _msgFor(_items[i].childId, _items[i].valueType);
		if (_items[i].flags & 0x01) {
			// Usa int32_t para evitar overflow em valores > 327 (ex: tensões)
			send(msg.set((int32_t)(_lastValues[i] * 100)));
		} else {
			send(msg.set(_lastValues[i], 1));
		}
	}

	void M360Node::_processBattery() {
		_battCycle++;
		if (isnan(_lastBattVoltage) || _battCycle >= 10) {
			_battCycle = 0;
			float voltage = readBatteryVoltage();
			if (voltage > 0.0f &&
			    (isnan(_lastBattVoltage) || fabsf(voltage - _lastBattVoltage) >= 0.1f)) {
				_lastBattVoltage = voltage;
				send(_msgFor(M360_CHILD_ID_BATTERY, V_VOLTAGE).set(voltage, 1));
				// Percentual padrão MySensors (I_BATTERY_LEVEL) — usado pela UI nativa
				// de bateria dos controladores, além do V_VOLTAGE bruto acima.
				sendBatteryLevel(voltageToPercent(voltage));
				Serial.print(F("Bat:"));
				Serial.println(voltage, 1);
			}
		}
	}

	void M360Node::_rePresent() {
		for (uint8_t i = 0; i < _count; i++) {
			present(_items[i].childId,
			        (mysensors_sensor_t)_items[i].presentationType,
			        _items[i].label);
		}
		present(M360_CHILD_ID_INTERVAL, S_CUSTOM,     F("Intervalo"));
		present(M360_CHILD_ID_BATTERY,  S_MULTIMETER, F("Bateria"));
		present(M360_CHILD_ID_DEBUG,    S_INFO,       F("Debug"));
		Serial.println(F("REPRES:OK"));
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
