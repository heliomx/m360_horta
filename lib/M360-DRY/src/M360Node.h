/*
 * M360Node.h — Classe principal da biblioteca M360-DRY
 *
 * Encapsula o ciclo de vida completo de um nó MySensors:
 * presentation / begin / process / handleMessage.
 *
 * USO:
 *   1. Defina NODE_ITEMS[] e NODE_ITEMS_COUNT no arquivo do nó.
 *   2. Declare os buffers (messages, lastValues, nNoUpdates).
 *   3. Instancie M360Node e registre os callbacks onRead/onWrite.
 *   4. Chame begin() em presentation(), process() em loop(),
 *      e handleMessage() em receive().
 *
 * IMPORTANTE: Não usa std::vector nem String — compatível com ATmega328P.
 */

#pragma once
#include <Arduino.h>
#include <core/MySensorsCore.h>
#include "M360Config.h"
#include "M360Power.h"

namespace M360
{
	// ===== ESTRUTURAS =====

	typedef enum {
		M360_SENSOR,
		M360_ACTUATOR
	} M360ItemKind;

	// Perfil de energia — passado no constructor do nó
	typedef enum {
		M360_LOW_POWER,   // bateria: usa smartSleep()
		/*
		Comportamento: O nó acorda sozinho em intervalos regulares (ex: a cada 5 minutos).
        Ação ao Acordar: Ele executa automaticamente o ciclo: Ligar Periféricos -> Ler Sensores -> Enviar Dados -> Desligar Periféricos -> Voltar a Dormir.
        Uso Ideal: Sensores de bateria comuns (Temperatura, Umidade de Solo simples) onde você quer um gráfico constante no Home Assistant sem precisar solicitar.
        Comando Externo: Se o gateway enviar um comando enquanto ele está no smartSleep, ele acorda, processa o comando, faz uma leitura completa e volta a dormir.
		*/
		M360_ALWAYS_ON,   // fonte fixa / debug: timer por millis, sem sleep
		M360_PASSIVE      // reativo: acorda no intervalo para "check-in", mas só lê/atua sob comando
		/*
		Comportamento: O nó acorda a cada intervalo definido, mas NÃO realiza a leitura automática dos sensores. 
		Ação ao Acordar: Ele utiliza o smartSleep() para verificar se o gateway possui mensagens pendentes (como comandos V_STATUS ou V_CUSTOM).
		Diferencial: Se não houver comando no gateway, ele volta a dormir imediatamente após o "check-in" do smartSleep. O ciclo de leitura/envio só acontece se o gateway enviar um comando durante a janela de acordado.
		Uso Ideal: Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.
		*/
	} M360PowerProfile;

	typedef struct {
		uint8_t      childId;
		M360ItemKind kind;
		uint8_t      presentationType;  // mysensors_sensor_t
		uint8_t      valueType;         // mysensors_data_t
		int          pin;               // -1 se não aplicável
		uint16_t     reportIntervalMin; // 0 = sempre reporta
		uint8_t      readSamples;       // número de amostras para média
		const char*  label;
		bool         wakeOnRadio;
		uint8_t      flags;             // bit 0: multiplica por 100 antes de enviar
	} M360ItemDef;

	// ===== CLASSE M360Node =====

	class M360Node {
	public:
		/*
		 * Constructor.
		 *
		 * @param items       Array const de M360ItemDef definido pelo nó.
		 * @param count       Número de itens em `items` (NODE_ITEMS_COUNT).
		 * @param messages    Buffer de MyMessage alocado pelo nó: MyMessage[count + 2].
		 * @param lastValues  Buffer float alocado pelo nó: float[count].
		 * @param nNoUpdates  Buffer uint8_t alocado pelo nó: uint8_t[count].
		 */
		M360Node(const M360ItemDef* items,
		         uint8_t            count,
		         MyMessage*         messages,
		         float*             lastValues,
		         uint8_t*           nNoUpdates,
		         M360PowerProfile   profile = M360_LOW_POWER);

		// ----- Lifecycle (chamados pelos hooks MySensors) -----

		// Chame dentro de presentation() — apresenta sensores e inicializa messages[]
		void begin(const char* name, const char* version);

		// Chame dentro de loop() — lê sensores, envia dados, gerencia sleep
		void process();

		// Chame dentro de receive() — processa intervalo e atuadores
		void handleMessage(const MyMessage& msg);

		// ----- Callbacks do nó -----

		// Registra função de leitura: recebe nodeIndex (índice em items[])
		void onRead(float (*callback)(uint8_t nodeIndex));

		// Registra função de escrita: recebe nodeIndex e estado bool
		void onWrite(void (*callback)(uint8_t nodeIndex, bool state));

		// ----- Configuração de pinos (opcional, baseado em items[].pin) -----
		void setupPins();

		// ----- Acesso ao intervalo atual -----
		uint16_t getInterval() const {
			return _interval;
		}

	private:
		const M360ItemDef* _items;
		uint8_t            _count;
		MyMessage*         _messages;
		float*             _lastValues;
		uint8_t*           _nNoUpdates;
		uint16_t           _interval;
		uint8_t            _lastBatt;
		uint8_t            _battCycle;

		M360PowerProfile   _profile;
		float (*_readCb)(uint8_t);
		void  (*_writeCb)(uint8_t, bool);

		void _readAndSendAll();
		void _processBattery();
		void _printNetDiag();
	};
} // namespace M360
