# Skill: Criar Novo Nó da Rede M360 Horta

## Objetivo

Orientar a criação completa de um novo nó da rede **M360 Horta**, garantindo conformidade com:

- Biblioteca `lib/M360-DRY` (padrão canônico — **não** usar `src/DRY/nos/shared/`)
- Padrão MySensors v2.x
- Estrutura PlatformIO do projeto
- Documentação de esquema elétrico e diagrama de blocos
- Configuração do `platformio.ini`

Sempre utilizar como referências primárias:
- Template: `lib/M360-DRY/examples/NodeTemplate/`
- Nós existentes: `src/DRY/nos/` (99nodeReles, 80nodeAqua, 13nodeZTS_UmidadeHall, 01nodeSolo3d)
- API: `lib/M360-DRY/API_REFERENCE.md`

---

# Regras Obrigatórias

## R1 — Macros MySensors exclusivamente no `platformio.ini`

Proibido em qualquer `.h` ou `.cpp`:

```cpp
#define MY_NODE_ID       // ← PROIBIDO no .cpp
#define MY_RADIO_RF24    // ← PROIBIDO no .cpp
#define MY_RF24_CE_PIN   // ← PROIBIDO no .cpp
#define MY_RF24_CSN_PIN  // ← PROIBIDO no .cpp
```

## R2 — Ordem obrigatória de includes

```cpp
#include <Arduino.h>       // sempre primeiro
#include <MySensors.h>     // segundo — antes de qualquer lib que use tipos MySensors
#include <M360.h>          // header centralizador M360-DRY
#include "sensorDrivers.h" // driver local do nó (último)
```

**Por que essa ordem importa:** `MySensors.h` habilita os interrupts globais (`sei()`) durante sua inicialização estática. Se `M360.h` ou `sensorDrivers.h` vierem antes, os construtores globais dessas libs rodam antes do `sei()`, fazendo o `Serial.print()` do banner travar permanentemente ao tentar drenar o TX buffer — os interrupts do UART nunca disparam. Sintoma: banner MySensors exibido parcialmente no Serial e o nó trava sem reset.

## R3 — Ciclo de vida MySensors — responsabilidades fixas por função

| Função | Responsabilidade |
|--------|-----------------|
| `before()` | `Serial.begin()`, `initSensors()` — antes do rádio iniciar |
| `presentation()` | `node.begin("Nome", "Versão")` |
| `setup()` | `node.onRead(cb)`, `node.onWrite(cb)` — após rádio ativo |
| `loop()` | `node.process()` + lógica de timing própria do nó |
| `receive()` | `node.handleMessage(msg)` — trata intervalo, atuadores, FORCE_UPDATE e REPRESENT |

**Nunca** colocar `Serial.begin()` ou `initSensors()` em `setup()`.

## R4 — Declarações de variáveis globais

```cpp
static const M360::M360ItemDef NODE_ITEMS[] = { ... };  // static const
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);
static MyMessage messages[NODE_ITEMS_COUNT + 2];  // +2 obrigatório (intervalo + bateria)
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues, nNoUpdates, PERFIL);
```

## R5 — Contrato de callbacks `onRead` / `onWrite`

O motor `M360Node` chama ambos com **nodeIndex** (posição no array `NODE_ITEMS[]`, não `childId`):

```cpp
// CORRETO — recebe índice no array
float readItem(uint8_t nodeIndex) { ... }
void  writeItem(uint8_t nodeIndex, bool state) { ... }

// ERRADO — nunca passar childId para writeNodeItem
writeNodeItem(NODE_ITEMS[nodeIndex].childId, state); // ← BUG
// CORRETO
writeNodeItem(nodeIndex, state);                      // ← OK
```

## R6 — `wait()` em vez de `delay()` dentro de callbacks MySensors

```cpp
// ERRADO — bloqueia processamento de pacotes do rádio
delay(100);

// CORRETO — processa o stack MySensors durante a espera
wait(100);
```

## R7 — `node.setupPins()` é OMITIDO quando há pinos virtuais MUX

Para nós com MUX CD74HC4067 (encoding virtual `pin = 100 + canal`), `setupPins()` faria `pinMode(104, OUTPUT)` em um pino inexistente. Nesses casos, toda a inicialização de pinos fica em `initSensors()` chamada no `before()`.

## R8 — IDs reservados (nunca usar como childId de sensores)

| childId | Uso reservado |
|---------|--------------|
| 254 | Intervalo (`V_VAR1`) |
| 255 | Bateria (`V_VOLTAGE`) |

## R11 — Comandos `V_CUSTOM` suportados pelo motor M360Node

O motor `M360Node` processa os seguintes payloads via `V_CUSTOM` + `C_SET` — todos definidos em `M360Constants.h`:

| Constante | Payload | Efeito |
|-----------|---------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | Leitura imediata de todos os sensores; em `M360_PASSIVE` liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | Re-anuncia todos os children via `present()` sem resetar estado; exibe `REPRES:OK` no Serial |
| `CMD_RESET` | `"RESET"` | Reservado para uso no nó (ex: zerar acumuladores de vazão) |

Para acionar `CMD_REPRESENT` via MQTT, publicar em `m360/{UF}/{CAR}/in`:
```json
{ "nodeId": <ID>, "sensorId": 0, "command": 1, "type": 48, "payload": "REPRESENT" }
```
> `type: 48` = `V_CUSTOM`. `sensorId` pode ser qualquer childId válido do nó.

## R9 — EEPROM — mapa de memória fixo (nunca escrever em 0–511)

| Região | Proprietário |
|--------|-------------|
| 0 – 511 | MySensors Core — **intocável** |
| 512 – 515 | `M360::saveInterval()` / `loadInterval()` |
| 521+ | `DeviceConfig` do gateway ESP8266 |

Nos nós AVR, persistir intervalo **sempre** via `M360::saveInterval()` — nunca `EEPROM.put()` direto.

## R10 — STOP & WAIT

O agente **nunca gera código** sem ter todos os requisitos do Passo 1 respondidos pelo usuário.

---

# Passo 1 — Elicitação de Requisitos (STOP & WAIT)

Apresentar as perguntas a seguir e aguardar resposta completa antes de prosseguir.

---

## 1.1 Identificação

```
Nome do sketch (ex: "nodeValvula"):
MY_NODE_ID (1–253, único na rede):
Descrição funcional breve:
```

## 1.2 Placa

Opções suportadas pelo projeto (AVR + MySensors RF24):

```
Arduino Nano (ATmega328P 5V/16MHz)   ← board: nanoatmega328
Arduino Pro Mini 5V/16MHz            ← board: pro16MHzatmega328
Arduino Pro Mini 3.3V/8MHz           ← board: pro8MHzatmega328
Arduino Mega 2560                    ← board: megaatmega2560
```

> Placas ESP32/ESP8266 são reservadas para o gateway — não usar em nós de campo.

## 1.3 Perfil de Energia

```
[ ] M360_LOW_POWER   — bateria/solar: smartSleep periódico
                       acorda → lê → envia → dorme
[ ] M360_ALWAYS_ON   — fonte fixa 5V/12V: timer por millis, nunca dorme
[ ] M360_PASSIVE     — bateria com sensor pesado (Modbus/RS485):
                       acorda no intervalo apenas para check-in,
                       lê somente se o gateway enviar FORCE_UPDATE
```

## 1.4 Rádio nRF24L01+

```
Pino CE  (default D9):
Pino CSN (default D10):
Potência RF: [ ] LOW  [ ] HIGH (default)  [ ] MAX
Timeout de rádio MY_RADIO_TIMEOUT_MS (deixar em branco para omitir):
```

> ⚠️ Se o nó usa SoftwareSerial (RS485, GPS), verificar conflito com CE/CSN nos pinos 9/10.
> Nó 13 (RS485): CE=D6, CSN=D4 para evitar conflito com SoftwareSerial em D9/D10.

## 1.5 Gestão de VCC dos sensores

```
Existe pino que corta a alimentação dos sensores?
[ ] Sim — Pino: ____  (ex: D3, D7)
[ ] Não
```

Se sim, `powerUp()` e `powerDown()` serão implementados no escopo `namespace M360 {}`.

## 1.6 Log de RSSI periódico

```
Habilitar log de RSSI no Serial? [ ] Sim  [ ] Não
Intervalo (ms): ____ (ex: 30000)
```

Se sim, gerar:
```cpp
#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
static unsigned long lastRssiLog = 0;
#endif
```
e no `loop()`:
```cpp
#if defined(MY_DEBUG) && defined(MY_RSSI_LOG_INTERVAL)
if (millis() - lastRssiLog >= MY_RSSI_LOG_INTERVAL) {
    lastRssiLog = millis();
    Serial.print(F("[RSSI] "));
    Serial.println(transportGetSignalReport(SR_RX_RSSI));
}
#endif
```

## 1.7 Sensores e Atuadores

Para **cada** sensor ou atuador solicitar:

```
childId (0–253, único neste nó):
Tipo (M360_SENSOR / M360_ACTUATOR):
Label (nome exibido no controller, ex: "Temp Estufa"):
Pino físico ou -1 se não aplicável:
```

O agente escolhe `S_*` e `V_*` mais adequados e apresenta para confirmação:

```
S_TEMP / V_TEMP       — temperatura
S_HUM / V_HUM         — umidade relativa
S_MOISTURE / V_LEVEL  — umidade de solo (0–100%)
S_WATER / V_FLOW      — vazão (L/s)
S_WATER_QUALITY / V_PH  — pH
S_WATER_QUALITY / V_EC  — condutividade elétrica
S_BINARY / V_STATUS   — atuador liga/desliga
S_CUSTOM / V_VAR1..5  — grandeza sem tipo padrão
```

Opções adicionais por item:

```
readSamples (1 = leitura única, 3–10 = média):
reportIntervalMin (0 = sempre envia se mudou):
wakeOnRadio (true = solicita estado ao gateway após acordar):
flags bit0 (true = multiplica valor × 100 antes de enviar):
```

> Se usar MUX CD74HC4067: informe o número do canal MUX (0–15).
> O agente codifica automaticamente como `pin = MUX_CHANNEL_OFFSET + canal` (ex: canal 3 → pin 103).

---

⚠️ Não prosseguir enquanto todas as informações acima não forem fornecidas e confirmadas.

---

# Passo 2 — Análise de Conflitos e Artefatos Existentes

## 2.1 Validação de conflitos

Verificar no projeto inteiro:

| Conflito | Como verificar |
|----------|---------------|
| `MY_NODE_ID` duplicado | Buscar em todos os `platformio.ini` envs |
| `childId` duplicado no mesmo nó | Conferir lista fornecida no Passo 1 |
| Pino físico duplicado no mesmo nó | Conferir lista fornecida no Passo 1 |
| Conflito de pino com rádio nRF24 | CE e CSN definidos devem ser únicos |
| Conflito de pino com Serial (D0/D1) | Pinos D0 e D1 reservados para UART |
| Conflito de pino com interrupções | D2=INT0, D3=INT1 — confirmar se necessário |

Emitir relatório antes de gerar qualquer código.

## 2.2 Artefatos existentes

Verificar se já existe pasta `src/DRY/nos/<nomeNó>/`:

- **Sim**: analisar o que pode ser aproveitado
- **Não**: criar a estrutura completa

---

# Passo 3 — Estrutura de Arquivos a Criar

```
src/DRY/nos/<nomeNó>/
├── withLibDRY/
│   └── <nomeNó>.cpp        ← sketch declarativo (NUNCA hardwareaquí)
├── sensorDrivers.h          ← interface pública do driver
├── sensorDrivers.cpp        ← implementação física dos sensores
├── esquema_eletrico.md      ← documentação de pinagem
└── diagrama_blocos.svg      ← diagrama visual do hardware
```

> O subdiretório **sempre** se chama `withLibDRY` (DRY maiúsculo).

---

# Passo 4 — Geração do Driver (`sensorDrivers.h` e `.cpp`)

## 4.1 `sensorDrivers.h`

Estrutura mínima:

```cpp
#pragma once
#include <Arduino.h>

// ===== PINOS =====
#define PIN_XXX  N    // Descrever função

// ===== CHILD IDs =====
#define CHILD_ID_XXX  N

// ===== CONSTANTES =====
// (Thresholds, offsets de calibração, etc.)

// ===== INTERFACE DO DRIVER =====
void  initSensors();
void  powerUpSensors();    // Presente apenas se houver pino VCC controlado
void  powerDownSensors();  // Presente apenas se houver pino VCC controlado
float readNodeItem(uint8_t nodeIndex);   // Parâmetro é índice no NODE_ITEMS[], NÃO childId
void  writeNodeItem(uint8_t nodeIndex, bool state);  // Omitir se não há atuadores
```

## 4.2 `sensorDrivers.cpp`

Regras de implementação:

- O parâmetro de `readNodeItem` e `writeNodeItem` é o **índice no array NODE_ITEMS[]**
- Para nós com MUX, usar `IS_MUX_CH(nodeIndex)` e `MUX_CH(nodeIndex)` definidos no `.h`
- Nunca usar `delay()` dentro destas funções — usar `wait()` se necessário
- `switch(nodeIndex)` deve ter `default: return NAN;` para sensores

### Exemplo para nó de sensor simples

```cpp
float readNodeItem(uint8_t nodeIndex) {
    switch (nodeIndex) {
        case 0: return sensor.readTemperature();
        case 1: return sensor.readHumidity();
        default: return NAN;
    }
}

void writeNodeItem(uint8_t nodeIndex, bool state) {
    if (nodeIndex == 2) {  // índice do relé em NODE_ITEMS[]
        digitalWrite(PIN_RELAY, state ? HIGH : LOW);
    }
}
```

### Exemplo para nó com MUX CD74HC4067

```cpp
#define MUX_CHANNEL_OFFSET 100
#define IS_MUX_CH(p)       ((p) >= MUX_CHANNEL_OFFSET && (p) < (MUX_CHANNEL_OFFSET + 16))
#define MUX_CH(p)          ((uint8_t)((p) - MUX_CHANNEL_OFFSET))

// nodeIndex → NODE_ITEMS[nodeIndex].pin (valor virtual 100+N)
void writeNodeItem(uint8_t nodeIndex, bool state) {
    int pin = NODE_ITEMS[nodeIndex].pin;
    if (IS_MUX_CH(pin)) {
        muxWrite(MUX_CH(pin), state);
    } else {
        digitalWrite(pin, state ? LOW : HIGH);  // Active-LOW para relés
    }
}
```

---

# Passo 5 — Geração do Sketch Principal (`withLibDRY/<nomeNó>.cpp`)

Basear estritamente no template `lib/M360-DRY/examples/NodeTemplate/NodeTemplate.cpp`.

## Estrutura completa obrigatória

```cpp
/*
 * <nomeNó>.cpp — Nó <ID>: <Descrição>
 *
 * Hardware: <placa> + <componentes>
 * Alimentação: <fonte>
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

// ===== CHILD IDs =====
// (importados de sensorDrivers.h ou definidos aqui)

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID_X, M360::M360_SENSOR,   S_TEMP,   V_TEMP,   -1, 1, 3, "Label", false, 0 },
    { CHILD_ID_Y, M360::M360_ACTUATOR, S_BINARY, V_STATUS,  5, 0, 1, "Rele",  true,  0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocação estática — sem heap) =====
static MyMessage messages[NODE_ITEMS_COUNT + 2];  // +2: intervalo e bateria
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::<PERFIL>);

// ===== CALLBACKS DE ENERGIA (apenas se houver PIN_POWER_SENSORS) =====
namespace M360 {
    void powerUp()   { powerUpSensors();   }
    void powerDown() { powerDownSensors(); }
}

// ===== MYSENSORS HOOKS =====

void before() {
    Serial.begin(MY_BAUD_RATE);
    initSensors();
}

void presentation() {
    node.begin("<NomeAmigavel>", "1.0");
}

void setup() {
    node.onRead(readNodeItem);
    node.onWrite(writeNodeItem);  // Omitir se não há atuadores
    // node.setupPins();  // Omitir se há pinos virtuais MUX (100+)
}

void loop() {
    node.process();
}

void receive(const MyMessage& msg) {
    node.handleMessage(msg);
}
```

> **Se não há pino VCC controlado**: omitir o bloco `namespace M360 {}` inteiro.
> **Se não há atuadores**: omitir `node.onWrite()` e `writeNodeItem`.
> **Se há MUX CD74HC4067**: omitir `node.setupPins()` conforme R7.

---

# Passo 6 — Atualização do `platformio.ini`

Adicionar o novo environment. Exemplo para Arduino Nano:

```ini
; =======================================================
; ENV: <DESCRIÇÃO DO NÓ>
; =======================================================
[env:<nomeEnv>]
extends = common
platform = atmelavr
board = nanoatmega328
framework = arduino

build_src_filter =
    +<DRY/nos/<nomeNó>/withLibDRY/*.cpp>
    +<DRY/nos/<nomeNó>/sensorDrivers.cpp>

monitor_speed = ${common.monitor_speed}

lib_deps =
    mysensors/MySensors@^2.3.2
    ; adicionar bibliotecas específicas do nó aqui

build_flags =
    ${common.flags}
    -D MY_NODE_ID=<ID>
    -D MY_RF24_CE_PIN=<CE>
    -D MY_RF24_CSN_PIN=<CSN>
    ; -D MY_RADIO_TIMEOUT_MS=500   ← apenas para nós ALWAYS_ON
    ; -D MY_DEBUG                   ← apenas em desenvolvimento
    ; -D MY_RSSI_LOG_INTERVAL=30000 ← se RSSI log habilitado no Passo 1.6
```

> ⚠️ A flag correta é `MY_RF24_CSN_PIN` (com N), não `MY_RF24_CS_PIN`.
> ⚠️ `MY_RADIO_TIMEOUT_MS` impacta negativamente nós LOW_POWER — usar só em ALWAYS_ON.

---

# Passo 7 — Criação dos Diagramas

## 7.1 `esquema_eletrico.md`

Documento Markdown com tabela de conexões. Usar como referência:
`src/DRY/nos/80nodeAqua/esquema_eletrico.md`

Conteúdo mínimo:
- Tabela pino → componente → sinal
- Alimentação (VCC/GND)
- Observações de conflito ou lógica Active-LOW
- Tabela de Child IDs com tipo MySensors

## 7.2 `diagrama_blocos.svg`

Diagrama visual SVG representando:
- MCU (placa)
- Módulo rádio nRF24L01+
- Sensores e atuadores
- Alimentação e pino VCC controlado (se houver)

Usar como referência: `src/DRY/nos/80nodeAqua/diagrama_blocos.svg`

---

# Passo 8 — Checklist de Anti-Padrões (pré-entrega)

Verificar cada item antes de entregar o código:

| # | Anti-padrão | Verificação |
|---|-------------|-------------|
| 1 | `#define MY_*` em `.cpp` ou `.h` | Buscar `MY_` fora do `platformio.ini` |
| 2 | `messages[N]` com N fixo em vez de `NODE_ITEMS_COUNT + 2` | Conferir declaração |
| 3 | `NODE_ITEMS[]` sem `static const` | Confirmar declaração |
| 4 | Buffers sem `static` | `messages`, `lastValues`, `nNoUpdates` |
| 5 | `Serial.begin()` em `setup()` | Deve estar em `before()` |
| 6 | `initSensors()` em `setup()` | Deve estar em `before()` |
| 7 | `delay()` dentro de `readNodeItem()` ou `writeNodeItem()` | Substituir por `wait()` |
| 8 | `writeNodeItem(NODE_ITEMS[i].childId, ...)` | Deve ser `writeNodeItem(nodeIndex, ...)` |
| 9 | `node.setupPins()` com pinos virtuais MUX (100+) | Omitir; init em `initSensors()` |
| 10 | `EEPROM.put()` direto no nó | Usar `M360::saveInterval()` |
| 11 | `CHILD_ID_*` como índice de array (`lastValues[CHILD_ID_X]`) | Usar `nodeIndex` ou constante `IDX_*` |
| 12 | `MY_RF24_CS_PIN` em vez de `MY_RF24_CSN_PIN` | Verificar flag no `.ini` |
| 13 | `build_src_filter` com casing inconsistente | `withLibDRY` (DRY maiúsculo) |
| 14 | childId 254 ou 255 em `NODE_ITEMS[]` | Reservados para intervalo e bateria |
| 15 | Ordem errada de includes (`M360.h` antes de `MySensors.h`) | Causa banner truncado + hang; ordem correta: `Arduino.h` → `MySensors.h` → `M360.h` → `sensorDrivers.h` |

---

# Entregáveis Obrigatórios

Ao finalizar apresentar lista de arquivos criados/modificados e walkthrough:

## Arquivos

```
src/DRY/nos/<nomeNó>/withLibDRY/<nomeNó>.cpp
src/DRY/nos/<nomeNó>/sensorDrivers.h
src/DRY/nos/<nomeNó>/sensorDrivers.cpp
src/DRY/nos/<nomeNó>/esquema_eletrico.md
src/DRY/nos/<nomeNó>/diagrama_blocos.svg
platformio.ini  (env adicionado)
```

## Resumo de configuração

```
✓ MY_NODE_ID      : <valor>
✓ Perfil          : M360_<PERFIL>
✓ Placa           : <board>
✓ CE / CSN        : D<X> / D<Y>
✓ Sensores        : <N> items em NODE_ITEMS[]
✓ Atuadores       : <N> items
✓ VCC controlado  : <sim/não> — pino <X>
✓ RSSI log        : <sim/não>
```

## Walkthrough técnico

Explicar:
1. Escolha do perfil de energia e implicação no `loop()`
2. Mapeamento nodeIndex → hardware em `readNodeItem()`/`writeNodeItem()`
3. Como o motor `M360Node` gerencia variação de valor e `forceUpdate` (após 10 ciclos sem mudança)
4. Comportamento de intervalo: como o gateway altera o intervalo via `V_VAR1` e onde é persistido na EEPROM
5. Bateria: como `_processBattery()` reporta automaticamente via childId 255
6. Se PASSIVE: como o FORCE_UPDATE dispara leitura via `handleMessage()` → `_readAndSendAll()`
7. Reapresentação: como o comando `REPRESENT` (V_CUSTOM) dispara `_rePresent()` que re-anuncia todos os children via `present()` sem resetar `lastValues` — útil após reinício do broker ou do Home Assistant
