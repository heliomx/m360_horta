/*
 * M360Registry.h — Gerenciamento de rastreamento de nós para Gateway M360
 */

#pragma once

#ifndef ESP8266
#  error "M360Registry.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>
#include <functional>

namespace M360 {

	static const uint8_t MAX_REGISTRY_NODES     = 30;
	static const uint8_t MAX_CHILDREN_PER_NODE = 16;
	static const uint8_t MAX_NAME_LEN          = 18;

	struct ChildInfo {
		uint8_t childId;
		uint8_t sensorType;               // S_BINARY, S_TEMP, S_WATER...
		char    label[MAX_NAME_LEN];      // "Sol.CanteiroA"
	};

	struct NodeStatus {
		uint8_t   nodeId;
		char      sketchName[MAX_NAME_LEN]; // "Central de Relés"
		unsigned long lastSeen;
		uint16_t  intervalMin;              // Intervalo configurado em minutos
		unsigned long cycleMs;              // Cadência observada (média móvel dos gaps)
		unsigned long timeoutMs;            // Limiar dinâmico: base + deltaTMs, base = max(intervalMin, cycleMs)
		bool      active;
		uint8_t   childCount;
		ChildInfo children[MAX_CHILDREN_PER_NODE];
	};

	class NodeRegistry {
	public:
		static const int MAX_NODES = MAX_REGISTRY_NODES;
		static const unsigned long DEFAULT_TIMEOUT_MS = 900000; // 15 minutos (900 s)

		// Gaps menores que isto são rajada (apresentação, eco de atuador), não ciclo.
		static const unsigned long MIN_CYCLE_SAMPLE_MS = 15000;   // 15 s
		// Teto do limiar. A cadência observada só cresce enquanto o nó chega dentro
		// da janela vigente, o que em tese permite crescimento geométrico; o teto
		// garante que um nó calado seja declarado perdido em algum momento.
		static const unsigned long MAX_TIMEOUT_MS      = 7200000; // 2 h

		NodeRegistry(unsigned long timeoutMs = DEFAULT_TIMEOUT_MS);

		// Atualiza o status e lastSeen de um nó (chamado ao receber qualquer mensagem)
		// Retorna true se o nó for novo no registro
		bool update(uint8_t nodeId);

		// Registra/atualiza o intervalo de reporte do nó e recalcula o timeout dinâmico (Intervalo + Delta T)
		void registerInterval(uint8_t nodeId, uint16_t intervalMin);

		// Registra/atualiza o nome do sketch informado pelo nó (I_SKETCH_NAME)
		void registerSketchName(uint8_t nodeId, const char* name);

		// Registra/atualiza um sensor/atuador apresentado pelo nó (C_PRESENTATION)
		void registerChild(uint8_t nodeId, uint8_t childId, uint8_t sensorType, const char* label);

		// Métodos de consulta semântica
		const char* getNodeName(uint8_t nodeId) const;
		const char* getChildLabel(uint8_t nodeId, uint8_t childId) const;
		uint8_t     getChildType(uint8_t nodeId, uint8_t childId) const;

		// Verifica timeouts e chama o callback para cada nó que ficou offline
		void checkTimeouts(std::function<void(uint8_t nodeId, const char* reason)> onNodeLost);

		// Retorna true se o nó está registrado e ativo
		bool isActive(uint8_t nodeId) const;

		// Retorna o número de nós atualmente registrados
		int count() const { return _count; }

		// Acesso direto aos dados (para debug/iteração)
		const NodeStatus* getNodes() const { return _registry; }

	private:
		NodeStatus    _registry[MAX_NODES];
		uint8_t       _count;
		unsigned long _timeoutMs;

		int findNodeIndex(uint8_t nodeId) const;
		int getOrAddNodeIndex(uint8_t nodeId);

		// Ponto único do cálculo do limiar — chamado por update() e registerInterval()
		void recalcTimeout(int idx);
	};

} // namespace M360
