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

	struct NodeStatus {
		uint8_t	   nodeId;
		unsigned long lastSeen;
		bool		  active;
	};

	class NodeRegistry {
	public:
		static const int MAX_NODES = 25; // Aumentado em relação ao original (10)
		static const unsigned long DEFAULT_TIMEOUT_MS = 300000; // 5 minutos

		NodeRegistry(unsigned long timeoutMs = DEFAULT_TIMEOUT_MS);

		// Atualiza o status de um nó (chamado ao receber qualquer mensagem)
		// Retorna true se o nó for novo no registro
		bool update(uint8_t nodeId);

		// Verifica timeouts e chama o callback para cada nó que ficou offline
		void checkTimeouts(std::function<void(uint8_t nodeId, const char* reason)> onNodeLost);

		// Retorna true se o nó está registrado e ativo
		bool isActive(uint8_t nodeId) const;

		// Retorna o número de nós atualmente registrados
		int count() const { return _count; }

		// Acesso direto aos dados (para debug/iteração)
		const NodeStatus* getNodes() const { return _registry; }

	private:
		NodeStatus	_registry[MAX_NODES];
		uint8_t	   _count;
		unsigned long _timeoutMs;
	};

} // namespace M360
