/*
 * M360Node.h — Classe principal da biblioteca M360-DRY
 *
 * Encapsula o ciclo de vida completo de um nó MySensors:
 * presentation / begin / process / handleMessage.
 *
 * USO:
 *   1. Declare cada rótulo como array PROGMEM e defina NODE_ITEMS[] e
 *      NODE_ITEMS_COUNT, passando os rótulos com M360_LABEL().
 *   2. Declare os buffers lastValues[] e nNoUpdates[] (tamanho NODE_ITEMS_COUNT).
 *   3. Instancie M360Node e registre os callbacks onRead/onWrite.
 *   4. Chame begin(F(nome), F(versão)) em presentation(), process() em loop(),
 *      e handleMessage() em receive().
 *
 * MEMÓRIA (2.0.0): o nó não declara messages[] — a lib monta cada envio num
 * único MyMessage interno — e rótulos, nome e versão ficam na Flash. Medido no
 * Nó 99 com a assinatura MySensors ligada: 1945 → 1235 B de RAM estática.
 *
 * IMPORTANTE: Não usa std::vector nem String — compatível com ATmega328P.
 */

#pragma once
#include <Arduino.h>
#include <core/MySensorsCore.h>
#include "M360Config.h"
#include "M360Power.h"

// Converte um array PROGMEM no tipo do campo `label` de M360ItemDef.
// SÓ para arrays declarados com PROGMEM: aplicado a uma string em RAM, compila
// e o nó envia lixo como rótulo. Um literal "..." direto no campo não compila —
// é essa a proteção que o tipo do campo dá.
#define M360_LABEL(progmemStr) (reinterpret_cast<const __FlashStringHelper*>(progmemStr))

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
		M360_PASSIVE,     // reativo: acorda no intervalo para "check-in", mas só lê/atua sob comando
		/*
		Comportamento: O nó acorda a cada intervalo definido, mas NÃO realiza a leitura automática dos sensores.
		Ação ao Acordar: Ele utiliza o smartSleep() para verificar se o gateway possui mensagens pendentes (como comandos V_STATUS ou V_CUSTOM).
		Diferencial: Se não houver comando no gateway, ele volta a dormir imediatamente após o "check-in" do smartSleep. O ciclo de leitura/envio só acontece se o gateway enviar um comando durante a janela de acordado.
		Uso Ideal: Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.
		*/
		M360_REPEATER     // fonte fixa + MY_REPEATER_FEATURE: encaminha mensagens de outros nós
		/*
		Comportamento: Idêntico ao M360_ALWAYS_ON (timer por millis, sem sleep).
		Diferencial: O MySensors mantém uma tabela de roteamento e encaminha automaticamente mensagens
		de nós que estão fora do alcance direto do gateway — tudo transparente via wait(50).
		Requisito: -D MY_REPEATER_FEATURE DEVE estar em build_flags do platformio.ini.
		           sleep() é bloqueado pelo MySensors (retorna MY_SLEEP_NOT_POSSIBLE) quando ativo.
		Apresentação: Sufixo "[REP]" no nome do sketch; nó se registra como S_ARDUINO_REPEATER_NODE.
		Uso Ideal: Nós 12V em posição estratégica (centro da planta, entre nós distantes e gateway).
		Incompatível com: M360_LOW_POWER, M360_PASSIVE (ambos usam sleep).
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
		const __FlashStringHelper* label; // na Flash — ver M360_LABEL()
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
		 * @param lastValues  Buffer float alocado pelo nó: float[count].
		 * @param nNoUpdates  Buffer uint8_t alocado pelo nó: uint8_t[count].
		 *
		 * Não há buffer de mensagens: a lib monta cada envio — dos itens e dos
		 * children reservados 253/254/255 — num único MyMessage interno.
		 */
		M360Node(const M360ItemDef* items,
		         uint8_t            count,
		         float*             lastValues,
		         uint8_t*           nNoUpdates,
		         M360PowerProfile   profile = M360_LOW_POWER);

		// ----- Lifecycle (chamados pelos hooks MySensors) -----

		// Chame dentro de presentation() — apresenta sensores e zera lastValues/nNoUpdates.
		// Nome e versão na Flash: node.begin(F("Nome"), F("1.0")).
		void begin(const __FlashStringHelper* name, const __FlashStringHelper* version);

		// Chame dentro de loop() — lê sensores, envia dados, gerencia sleep
		void process();

		// Chame dentro de receive() — processa intervalo e atuadores
		void handleMessage(const MyMessage& msg);

		// ----- Callbacks do nó -----

		// Registra função de leitura: recebe nodeIndex (índice em items[])
		void onRead(float (*callback)(uint8_t nodeIndex));

		// Registra função de escrita: recebe nodeIndex e estado bool
		void onWrite(void (*callback)(uint8_t nodeIndex, bool state));

		// Envia string de debug pela rede (CHILD_ID_DEBUG, V_TEXT) — visível no
		// MQTT/Node-RED via gateway sem necessidade de acesso serial ao nó.
		// Payload truncado em 24 chars (limite MAX_PAYLOAD_SIZE do MySensors).
		void sendDebug(const char* text);

		// Envia o estado do item `index` com o child e o tipo dele. Para o nó
		// reportar mudanças que acontecem fora do fluxo da lib — ex.: o failsafe
		// que desliga um relé sozinho, ou o canal MUX preemptado (Nó 99).
		void sendItemState(uint8_t index, bool state);

		// ----- Configuração de pinos (opcional, baseado em items[].pin) -----
		void setupPins();

		// ----- Acesso e ajuste do intervalo em runtime -----
		uint16_t getInterval() const {
			return _interval;
		}

		// Retorna o override em ms (0 = inativo — warm-up concluído ou não iniciado).
		uint32_t getIntervalMs() const {
			return _intervalMs;
		}

		// Altera o intervalo de ciclo sem gravar na EEPROM.
		// Uso: warm-up pós-boot (ex.: 30 s inicial → 30 min após 5 min).
		// Valores fora de [M360_MIN_INTERVAL, M360_MAX_INTERVAL] são ignorados.
		void setInterval(uint16_t minutes);

		// Altera o intervalo de ciclo usando milissegundos brutos (sem gravar EEPROM).
		// Sobrepõe temporariamente o intervalo em minutos; use 0 para cancelar o override.
		// Uso típico: warm-up com 30 s (setIntervalMs(30000UL)) logo após o boot.
		void setIntervalMs(uint32_t ms);

	private:
		const M360ItemDef* _items;
		uint8_t            _count;
		// Único buffer de envio. A lib nunca relê a mensagem — o valor sempre vem
		// de _lastValues —, então basta remontar o cabeçalho (child + tipo) a cada send.
		MyMessage          _txMsg;
		float*             _lastValues;
		uint8_t*           _nNoUpdates;
		uint16_t           _interval;    // minutos (persistido na EEPROM)
		uint32_t           _intervalMs;  // override em ms (0 = inativo, não persiste)
		float              _lastBattVoltage;
		uint8_t            _battCycle;

		M360PowerProfile   _profile;
		float (*_readCb)(uint8_t);
		void  (*_writeCb)(uint8_t, bool);

		// Prepara _txMsg para o child/tipo informados e o devolve para o send().
		MyMessage& _msgFor(uint8_t childId, uint8_t valueType);
		// forceAll=true ignora o filtro de "mudou o suficiente" (usado por CMD_FORCE_UPDATE)
		void _readAndSendAll(bool forceAll = false);
		// Envia _lastValues[i] já em cache (sem reler o hardware), usada tanto por
		// _readAndSendAll() quanto pela resposta a C_REQ.
		void _sendCachedValue(uint8_t i);
		void _processBattery();
		void _printNetDiag();
		void _rePresent();
	};
} // namespace M360
