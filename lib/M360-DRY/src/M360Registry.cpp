/*
 * M360Registry.cpp — Implementação do rastreamento de nós
 */

#ifdef ESP8266

#include "M360Registry.h"

namespace M360 {

	NodeRegistry::NodeRegistry(unsigned long timeoutMs)
		: _count(0)
		, _timeoutMs(timeoutMs)
	{
		for (int i = 0; i < MAX_NODES; i++) {
			_registry[i] = { 0, 0, false };
		}
	}

	bool NodeRegistry::update(uint8_t nodeId) {
		if (nodeId == 0) return false; // Gateway ID não rastreado

		// Procurar nó existente
		for (int i = 0; i < _count; i++) {
			if (_registry[i].nodeId == nodeId) {
				_registry[i].lastSeen = millis();
				bool wasActive = _registry[i].active;
				_registry[i].active = true;
				return !wasActive; // Retorna true se "reconectou"
			}
		}

		// Adicionar novo nó se houver espaço
		if (_count < MAX_NODES) {
			_registry[_count++] = { nodeId, millis(), true };
			return true;
		}

		// Registro cheio: reutilizar o slot inativo mais antigo (evitar perda silenciosa)
		int evictIdx = -1;
		unsigned long oldestSeen = 0xFFFFFFFFUL;
		for (int i = 0; i < _count; i++) {
			if (!_registry[i].active && _registry[i].lastSeen < oldestSeen) {
				oldestSeen = _registry[i].lastSeen;
				evictIdx = i;
			}
		}
		if (evictIdx >= 0) {
			_registry[evictIdx] = { nodeId, millis(), true };
			return true;
		}

		return false;
	}

	void NodeRegistry::checkTimeouts(std::function<void(uint8_t nodeId, const char* reason)> onNodeLost) {
		unsigned long now = millis();
		for (int i = 0; i < _count; i++) {
			if (_registry[i].active && (now - _registry[i].lastSeen > _timeoutMs)) {
				_registry[i].active = false;
				if (onNodeLost) {
					char buffer[32];
					snprintf(buffer, sizeof(buffer), "timeout %lu s", _timeoutMs / 1000);
					onNodeLost(_registry[i].nodeId, buffer);
				}
			}
		}
	}

	bool NodeRegistry::isActive(uint8_t nodeId) const {
		for (int i = 0; i < _count; i++) {
			if (_registry[i].nodeId == nodeId) {
				return _registry[i].active;
			}
		}
		return false;
	}

} // namespace M360

#endif // ESP8266

