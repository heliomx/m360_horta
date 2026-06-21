/*
 * NodeTemplate.cpp — Modelo Arquitetural para Nós M360-DRY
 *
 * ESTRUTURA DE DOIS ARQUIVOS:
 *   NodeTemplate.cpp    — DECLARATIVO: IDs, labels, perfil de energia, hooks MySensors.
 *                         Nunca contém acesso direto a pinos ou bibliotecas de sensor.
 *   sensorDrivers.h/.cpp — FÍSICO: leitura de hardware, calibração, controle de pinos.
 *
 * PARA CRIAR UM NOVO NÓ:
 *   1. Copie esta pasta para src/DRY/nos/<nomeNo>/withLibDRY/
 *   2. Renomeie NodeTemplate.cpp para <nomeNo>.cpp
 *   3. Preencha NODE_ITEMS[] e ajuste os CHILD IDs
 *   4. Implemente readNodeItem() e writeNodeItem() em sensorDrivers.cpp
 *   5. Adicione o environment em platformio.ini (com build_src_filter)
 *
 * REGRAS CRÍTICAS:
 *   - MY_* macros definidas EXCLUSIVAMENTE no platformio.ini
 *   - Serial.begin() e initSensors() SEMPRE em before(), nunca em setup()
 *   - messages[] SEMPRE com tamanho NODE_ITEMS_COUNT + 2
 *   - Todas as variáveis globais são static
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== CHILD IDs =====
// Valores permitidos: 0–253. IDs 254 (intervalo) e 255 (bateria) são reservados pelo motor.
#define CHILD_TEMP   0
#define CHILD_HUM    1
// #define CHILD_RELAY  2  // descomente se o nó tiver atuador

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// Colunas: childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
//
// pin      : pino físico (-1 se leitura via código no driver, 100+ para canais MUX CD74HC4067)
// intervalMin: intervalo mínimo de reporte em minutos (0 = envia sempre que mudar)
// samples  : número de amostras para média antes do envio (1 = leitura única)
// wakeOnRadio: true → solicita estado ao gateway após acordar (útil para atuadores)
// flags    : bit0=1 → multiplica valor × 100 antes de enviar (ex: para tensão em cV)
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ CHILD_TEMP, M360::M360_SENSOR,   S_TEMP,   V_TEMP,   -1, 1, 3, "Temperatura", false, 0 },
	{ CHILD_HUM,  M360::M360_SENSOR,   S_HUM,    V_HUM,    -1, 1, 3, "Umidade",     false, 0 },
	// { CHILD_RELAY, M360::M360_ACTUATOR, S_BINARY, V_STATUS, 5, 0, 1, "Rele", true, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS ESTÁTICOS (gerenciados pelo M360Node — nunca acessar diretamente) =====
// messages: +2 obrigatório para os canais reservados intervalo (254) e bateria (255)
static MyMessage messages[NODE_ITEMS_COUNT + 2];
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
// Escolha o perfil de energia adequado:
//   M360::M360_LOW_POWER  — bateria/solar: smartSleep periódico (acorda → lê → envia → dorme)
//   M360::M360_ALWAYS_ON  — fonte fixa 5V/12V: timer por millis(), nunca dorme
//   M360::M360_PASSIVE    — bateria + sensor pesado (Modbus/RS485): dorme e lê só sob FORCE_UPDATE
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_LOW_POWER);

// ===== CALLBACKS DE ENERGIA =====
// Implemente APENAS se o nó tem um pino que corta a alimentação dos sensores (ex: PIN_POWER_SENSORS).
// Se não houver pino VCC controlado, remova este bloco inteiro.
// IMPORTANTE: implementar DENTRO do namespace M360 {} — funções fora do namespace não sobrescrevem.
namespace M360
{
	void powerUp()
	{
		powerUpSensors();
	}

	void powerDown()
	{
		powerDownSensors();
	}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void before()
{
	// before() é executado ANTES do rádio MySensors inicializar.
	// É o único lugar correto para Serial.begin() e initSensors().
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation()
{
	// begin() apresenta todos os sensores do NODE_ITEMS[] e inicializa messages[].
	// Adiciona sufixo ao nome: [LP], [ON] ou [PAS] conforme o perfil.
	node.begin("M360 Template", "1.0");
}

void setup()
{
	// setup() é executado APÓS o rádio estar ativo.
	// Registrar callbacks — onRead e onWrite recebem nodeIndex (índice no NODE_ITEMS[]).
	node.onRead(readNodeItem);
	// node.onWrite(writeNodeItem);  // descomente apenas se houver atuadores em NODE_ITEMS[]

	// node.setupPins();
	// Descomente APENAS se os pinos em NODE_ITEMS[].pin são todos físicos reais (≥ 0 e < 100).
	// OMITIR quando houver pinos virtuais MUX (pin = 100 + canal): setupPins() chamaria
	// pinMode() com valores inválidos (ex: 104) no ATmega328P.
}

void loop()
{
	node.process();
}

void receive(const MyMessage& msg)
{
	// handleMessage() processa automaticamente:
	//   - Alteração de intervalo via V_VAR1 (childId 254)
	//   - Comando FORCE_UPDATE via V_CUSTOM
	//   - Atuadores via V_STATUS (chama writeNodeItem(nodeIndex, state))
	node.handleMessage(msg);
}
