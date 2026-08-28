/*
 * M360Registry.cpp — Implementação do rastreamento de nós
 */

#ifdef ESP8266

#include "M360Registry.h"
#include "M360Config.h"   // M360_MIN_INTERVAL / M360_MAX_INTERVAL
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
			_registry[i].intervalMin = 0;
			_registry[i].cycleMs = 0;
			_registry[i].timeoutMs = _timeoutMs;
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
			_registry[idx].intervalMin = 0;
			_registry[idx].cycleMs = 0;
			_registry[idx].timeoutMs = _timeoutMs;
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
			_registry[evictIdx].intervalMin = 0;
			_registry[evictIdx].cycleMs = 0;
			_registry[evictIdx].timeoutMs = _timeoutMs;
			_registry[evictIdx].active = true;
			_registry[evictIdx].childCount = 0;
			return evictIdx;
		}

		return -1;
	}

	void NodeRegistry::registerInterval(uint8_t nodeId, uint16_t intervalMin) {
		// 0 é o gateway; 255 é BROADCAST — nunca um remetente real. Sem esta guarda,
		// um comando de intervalo em broadcast criava um "nó 255" ativo no registro,
		// que ocupava slot, podia despejar um nó real por LRU e depois disparava um
		// node_lost para um nó que não existe.
		if (nodeId == 0 || nodeId == 255) return;

		// Mesma faixa que M360Node::handleMessage() aplica antes de aceitar o valor.
		// Sem o clamp as duas pontas discordavam por construção, e um valor absurdo
		// (atoi de um typo) estourava o cálculo: unsigned long tem 32 bits no
		// ESP8266, e intervalMs + deltaTMs passa de 2^32 acima de ~47.700 min,
		// fazendo o timeout dar wrap para um valor pequeno e arbitrário.
		if (intervalMin < M360_MIN_INTERVAL || intervalMin > M360_MAX_INTERVAL) return;

		// Só atualiza nó já conhecido: aprender uma cadência não é motivo para
		// inventar um nó que nunca falou. Quem cria a entrada é update(), chamado
		// em receive() antes deste ponto para toda mensagem que chega pelo rádio.
		int idx = findNodeIndex(nodeId);
		if (idx >= 0) {
			_registry[idx].intervalMin = intervalMin;
			recalcTimeout(idx);
		}
	}

	// Limiar de inatividade, em ponto único: base = max(intervalo declarado,
	// cadência observada), mais Delta T = Max(2 min, 50% da base).
	//
	// O intervalo declarado sozinho NÃO descreve o silêncio legítimo de um nó.
	// M360Node::_readAndSendAll() só transmite um sensor quando o valor muda ou a
	// cada 10 ciclos (staleForced = _nNoUpdates[i] >= 10), então um nó ALWAYS_ON
	// com intervalMin = 1 pode ficar ~11 min calado sem nenhum defeito. Derivando
	// só do intervalo, o Nó 99 ganhava timeout de 180 s e o gateway declarava
	// node_lost em toda janela sem mudança de leitura. Antes de 28/08/2026 isso
	// não aparecia porque o limiar era fixo em 900 s — acidentalmente seguro.
	//
	// Nó LOW_POWER não muda de comportamento: o smartSleep emite
	// I_PRE/POST_SLEEP_NOTIFICATION a cada ciclo, então cadência observada e
	// intervalo declarado coincidem e o max() devolve o mesmo valor de antes.
	//
	// Mesma fórmula do `Mapeia nós` no Node-RED — é contrato de dois lados
	// (nodered/funcionalidades_nodered.md §9). Mudar aqui exige mudar lá.
	void NodeRegistry::recalcTimeout(int idx) {
		if (idx < 0 || idx >= MAX_NODES) return;

		unsigned long base = (unsigned long)_registry[idx].intervalMin * 60000UL;
		if (_registry[idx].cycleMs > base) {
			base = _registry[idx].cycleMs;
		}
		if (base == 0UL) {
			_registry[idx].timeoutMs = _timeoutMs; // nada aprendido ainda
			return;
		}

		const unsigned long deltaTMs = (base / 2UL > 120000UL) ? (base / 2UL) : 120000UL;
		unsigned long timeout = base + deltaTMs;
		if (timeout > MAX_TIMEOUT_MS) {
			timeout = MAX_TIMEOUT_MS;
		}
		_registry[idx].timeoutMs = timeout;
	}

	bool NodeRegistry::update(uint8_t nodeId) {
		if (nodeId == 0) return false; // Gateway ID não rastreado

		int idx = findNodeIndex(nodeId);
		if (idx >= 0) {
			const unsigned long now = millis();

			// Aprende a cadência real, para alimentar recalcTimeout().
			// Só amostra com o nó ATIVO: o intervalo desde um node_lost é tempo de
			// queda, não ritmo de reporte — usá-lo faria o nó que volta esticar a
			// própria janela na proporção do tempo em que esteve fora.
			// Gaps curtos também não entram: a rajada de apresentação e o eco de
			// atuador chegam em milissegundos e puxariam a média para baixo.
			if (_registry[idx].active && _registry[idx].lastSeen != 0UL) {
				const unsigned long gap = now - _registry[idx].lastSeen; // wrap-safe
				if (gap > MIN_CYCLE_SAMPLE_MS) {
					_registry[idx].cycleMs = (_registry[idx].cycleMs > 0UL)
					    ? (7UL * _registry[idx].cycleMs + 3UL * gap) / 10UL  // média móvel 0,7/0,3
					    : gap;
					recalcTimeout(idx);
				}
			}

			_registry[idx].lastSeen = now;
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
			unsigned long nodeTimeout = (_registry[i].timeoutMs > 0) ? _registry[i].timeoutMs : _timeoutMs;
			if (_registry[i].active && (now - _registry[i].lastSeen > nodeTimeout)) {
				_registry[i].active = false;
				if (onNodeLost) {
					// O motivo diz de onde veio a janela: sem as duas parcelas não dá
					// para saber se o limiar nasceu do intervalo declarado ou da
					// cadência aprendida. O texto cabe no buffer de 64 do
					// publishTransportEvent() mesmo no pior caso ("Inactivity
					// detected: " + 41 chars).
					char buffer[48];
					if (_registry[i].intervalMin > 0 && _registry[i].cycleMs > 0) {
						snprintf(buffer, sizeof(buffer), "timeout %lu s (Int %u min, obs %lu s)",
						         nodeTimeout / 1000, _registry[i].intervalMin, _registry[i].cycleMs / 1000);
					} else if (_registry[i].intervalMin > 0) {
						snprintf(buffer, sizeof(buffer), "timeout %lu s (Int: %u min)", nodeTimeout / 1000, _registry[i].intervalMin);
					} else if (_registry[i].cycleMs > 0) {
						snprintf(buffer, sizeof(buffer), "timeout %lu s (obs %lu s)", nodeTimeout / 1000, _registry[i].cycleMs / 1000);
					} else {
						snprintf(buffer, sizeof(buffer), "timeout %lu s", nodeTimeout / 1000);
					}
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

