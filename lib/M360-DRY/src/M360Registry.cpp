/*
 * M360Registry.cpp — Implementação do rastreamento de nós
 */

#ifdef ESP8266

#include "M360Registry.h"
#include <string.h>

namespace M360 {

	NodeRegistry::NodeRegistry(unsigned long timeoutMs)
		: _count(0)
		, _timeoutMs(timeoutMs)
	{
		for (int i = 0; i < MAX_NODES; i++) {
			_registry[i].nodeId = 0;
			_registry[i].sketchName[0] = '\0';
			_registry[i].lastSeen = 0;
			_registry[i].active = false;
			_registry[i].childCount = 0;
			for (int j = 0; j < MAX_CHILDREN_PER_NODE; j++) {
				_registry[i].children[j].childId = 0;
				_registry[i].children[j].sensorType = 0;
				_registry[i].children[j].label[0] = '\0';
			}
		}
	}

	int NodeRegistry::findNodeIndex(uint8_t nodeId) const {
		if (nodeId == 0) return -1;
		for (int i = 0; i < _count; i++) {
			if (_registry[i].nodeId == nodeId) {
				return i;
			}
		}
		return -1;
	}

	int NodeRegistry::getOrAddNodeIndex(uint8_t nodeId) {
		if (nodeId == 0) return -1;

		int idx = findNodeIndex(nodeId);
		if (idx >= 0) return idx;

		if (_count < MAX_NODES) {
			idx = _count++;
			_registry[idx].nodeId = nodeId;
			_registry[idx].sketchName[0] = '\0';
			_registry[idx].lastSeen = millis();
			_registry[idx].active = true;
			_registry[idx].childCount = 0;
			return idx;
		}

		// Reutilizar slot inativo mais antigo se a tabela estiver cheia (LRU)
		int evictIdx = -1;
		unsigned long oldestSeen = 0xFFFFFFFFUL;
		for (int i = 0; i < _count; i++) {
			if (!_registry[i].active && _registry[i].lastSeen < oldestSeen) {
				oldestSeen = _registry[i].lastSeen;
				evictIdx = i;
			}
		}
		if (evictIdx >= 0) {
			_registry[evictIdx].nodeId = nodeId;
			_registry[evictIdx].sketchName[0] = '\0';
			_registry[evictIdx].lastSeen = millis();
			_registry[evictIdx].active = true;
			_registry[evictIdx].childCount = 0;
			return evictIdx;
		}

		return -1;
	}

	bool NodeRegistry::update(uint8_t nodeId) {
		if (nodeId == 0) return false; // Gateway ID não rastreado

		int idx = findNodeIndex(nodeId);
		if (idx >= 0) {
			_registry[idx].lastSeen = millis();
			bool wasActive = _registry[idx].active;
			_registry[idx].active = true;
			return !wasActive; // Retorna true se reconectou
		}

		int newIdx = getOrAddNodeIndex(nodeId);
		return (newIdx >= 0);
	}

	void NodeRegistry::registerSketchName(uint8_t nodeId, const char* name) {
		if (nodeId == 0 || name == NULL) return;
		int idx = getOrAddNodeIndex(nodeId);
		if (idx >= 0) {
			strncpy(_registry[idx].sketchName, name, MAX_NAME_LEN - 1);
			_registry[idx].sketchName[MAX_NAME_LEN - 1] = '\0';
			_registry[idx].lastSeen = millis();
			_registry[idx].active = true;
		}
	}

	void NodeRegistry::registerChild(uint8_t nodeId, uint8_t childId, uint8_t sensorType, const char* label) {
		if (nodeId == 0) return;
		int idx = getOrAddNodeIndex(nodeId);
		if (idx < 0) return;

		NodeStatus& node = _registry[idx];
		node.lastSeen = millis();
		node.active = true;

		// Verificar se o child já existe
		for (int j = 0; j < node.childCount; j++) {
			if (node.children[j].childId == childId) {
				node.children[j].sensorType = sensorType;
				if (label != NULL && strlen(label) > 0) {
					strncpy(node.children[j].label, label, MAX_NAME_LEN - 1);
					node.children[j].label[MAX_NAME_LEN - 1] = '\0';
				}
				return;
			}
		}

		// Adicionar novo child se houver espaço
		if (node.childCount < MAX_CHILDREN_PER_NODE) {
			ChildInfo& child = node.children[node.childCount++];
			child.childId = childId;
			child.sensorType = sensorType;
			if (label != NULL && strlen(label) > 0) {
				strncpy(child.label, label, MAX_NAME_LEN - 1);
				child.label[MAX_NAME_LEN - 1] = '\0';
			} else {
				child.label[0] = '\0';
			}
		}
	}

	const char* NodeRegistry::getNodeName(uint8_t nodeId) const {
		int idx = findNodeIndex(nodeId);
		if (idx >= 0 && _registry[idx].sketchName[0] != '\0') {
			return _registry[idx].sketchName;
		}
		return nullptr;
	}

	const char* NodeRegistry::getChildLabel(uint8_t nodeId, uint8_t childId) const {
		int idx = findNodeIndex(nodeId);
		if (idx >= 0) {
			const NodeStatus& node = _registry[idx];
			for (int j = 0; j < node.childCount; j++) {
				if (node.children[j].childId == childId && node.children[j].label[0] != '\0') {
					return node.children[j].label;
				}
			}
		}
		return nullptr;
	}

	uint8_t NodeRegistry::getChildType(uint8_t nodeId, uint8_t childId) const {
		int idx = findNodeIndex(nodeId);
		if (idx >= 0) {
			const NodeStatus& node = _registry[idx];
			for (int j = 0; j < node.childCount; j++) {
				if (node.children[j].childId == childId) {
					return node.children[j].sensorType;
				}
			}
		}
		return 0xFF; // Não encontrado
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
		int idx = findNodeIndex(nodeId);
		return (idx >= 0 && _registry[idx].active);
	}

} // namespace M360

#endif // ESP8266

