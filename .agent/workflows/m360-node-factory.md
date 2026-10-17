# Skill: Criar Novo Nó da Rede M360 Horta

## Objetivo

Orientar a criação completa de um novo nó da rede **M360 Horta**, garantindo conformidade com:

- Biblioteca `lib/M360-DRY` (padrão canônico — **não** usar `src/DRY/horta/nos/shared/`)
- Padrão MySensors v2.x
- Estrutura PlatformIO do projeto
- Documentação de esquema elétrico e diagrama de blocos
- Configuração do `platformio.ini`

Sempre utilizar como referências primárias:
- Template: `lib/M360-DRY/examples/NodeTemplate/`
- Nós existentes: `src/DRY/horta/nos/` (`lp/` — firmware comum da M360-LP; 99nodeReles, 04noodeSolarMini, 01nodeSolo3dNano, 02nodeSolo3dNano)
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
| `presentation()` | `node.begin(F("Nome"), F("Versão"))` — sem `F()` não compila |
| `setup()` | `node.onRead(cb)`, `node.onWrite(cb)` — após rádio ativo |
| `loop()` | `node.process()` + lógica de timing própria do nó |
| `receive()` | `node.handleMessage(msg)` — trata intervalo, atuadores, FORCE_UPDATE e REPRESENT |

**Nunca** colocar `Serial.begin()` ou `initSensors()` em `setup()`.

## R4 — Declarações de variáveis globais

```cpp
static const char L_X[] PROGMEM = "Label";               // um array PROGMEM por rótulo
static const M360::M360ItemDef NODE_ITEMS[] = { ... };  // static const; label = M360_LABEL(L_X)
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);
static float     lastValues[NODE_ITEMS_COUNT];           // sem messages[] (M360-DRY 2.0.0)
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, lastValues, nNoUpdates, PERFIL);
```

## R5 — Contrato de callbacks `onRead` / `onWrite`

O motor `M360Node` chama os callbacks com **nodeIndex** (posição no array
`NODE_ITEMS[]`), **nunca** com `childId`:

```cpp
float readItem(uint8_t nodeIndex) { ... }
void  writeItem(uint8_t nodeIndex, bool state) { ... }
```

O que o callback repassa ao driver depende da convenção adotada pelo nó — as
duas são válidas, desde que o `.h` e o `.cpp` concordem:

```cpp
// Convenção A — driver indexado por posição (Nó 4; nos Nós 01/02 a função vive no próprio .cpp)
//   float readNodeItem(uint8_t nodeIndex);
return readNodeItem(nodeIndex);

// Convenção B — driver endereçado por pino, real ou virtual MUX (nó 99)
//   void writeNodeItem(uint8_t pin, bool state);
writeNodeItem(NODE_ITEMS[nodeIndex].pin, state);
```

```cpp
// ERRADO em qualquer convenção — childId não é nem índice nem pino
writeNodeItem(NODE_ITEMS[nodeIndex].childId, state); // ← BUG
```

> Desde o realinhamento normativo, os `childId` do nó 99 vão de 31 a 39 — usar
> um deles como índice de array estouraria os buffers imediatamente.

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
| 253 | Debug remoto (`V_TEXT`) — `M360Node::sendDebug()` |
| 254 | Intervalo (`V_VAR1`) |
| 255 | Bateria (`V_VOLTAGE`) |

## R11 — Comandos `V_CUSTOM` suportados pelo motor M360Node

O motor `M360Node` processa os seguintes payloads via `V_CUSTOM` + `C_SET` — todos definidos em `M360Constants.h`:

| Constante | Payload | Efeito |
|-----------|---------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | Leitura imediata de todos os sensores; em `M360_PASSIVE` liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | Re-anuncia todos os children via `present()` sem resetar estado; exibe `REPRES:OK` no Serial |
| `CMD_DEBUG_NET` | `"DEBUG_NET"` | Responde diagnóstico de rede (`P:<parent> D:<dist> R:<S/N>`) via child 253 |

Para acionar `CMD_REPRESENT` via MQTT (gateway em `M360_NATIVE_MQTT=1`), os campos vão no tópico e o payload é a string bruta:
```
tópico:  m360/{UF}/{CAR}/in/<ID>/0/1/0/48
payload: REPRESENT
```
> `48` = `V_CUSTOM`. O childId pode ser qualquer um válido do nó. Um envelope JSON publicado no tópico
> raiz `.../in` é rejeitado pelo gateway (`reportCommandRejected`, "número de níveis inválido").

## R12 — Atualizar o inventário na mesma entrega

Qualquer alteração de código em `src/DRY/horta/` **deve** ser refletida em
`src/DRY/horta/inventario.md`. Criar um nó novo exige acrescentar:

- a linha do nó em §1 (env, placa, perfil, faixa de children)
- uma seção própria com **todos** os children, um a um, informando `childId`,
  `label`, `kind`, `S_*`, `V_*`, tipo do payload, `pin`, `reportIntervalMin`,
  `wakeOnRadio` e `flags`
- se algum child for endereçado pelo Node-RED, a linha correspondente em §8
  (contrato com o `flows.json`)

> O inventário é o contrato com o Node-RED. Um `childId` que não exista lá — ou
> que exista com outro número — leva a comandos **descartados em silêncio** pelo
> nó, sem erro de compilação nem de log.

---

## R9 — EEPROM — mapa de memória fixo (nunca escrever em 0–511)

| Região | Proprietário |
|--------|-------------|
| 0 – 511 | MySensors Core — **intocável** |
| 512 – 515 | `M360::saveInterval()` / `loadInterval()` |
| 521+ | `DeviceConfig` do gateway ESP8266 |

Nos nós AVR, persistir intervalo **sempre** via `M360::saveInterval()` — nunca `EEPROM.put()` direto.

## R10 — STOP & WAIT

O agente **nunca gera código** sem ter todos os requisitos do Passo 1 respondidos pelo usuário.

## R13 — Warm-Up Pós-Boot obrigatório para `M360_LOW_POWER`

Todo nó com perfil `M360_LOW_POWER` **deve** implementar o padrão warm-up, contado em **ciclos**
(referência completa: `lib/M360-DRY/API_REFERENCE.md` §4):

```cpp
#define WARMUP_INTERVAL_MS 30000UL   // 30 s entre ciclos
#define WARMUP_CYCLES      10        // 10 ciclos × 30 s ≈ 5 min de relógio

static uint8_t warmupCycles = 0;
static bool    warmupDone   = false;

void setup() {
    node.onRead(readNodeItem);
    if (node.setIntervalMs(WARMUP_INTERVAL_MS)) {   // fase 1: 30 s
        Serial.println(F("WARMUP:30s"));
    }
}

void loop() {
    node.process();   // LOW_POWER: uma chamada = um ciclo (lê → envia → dorme)

    if (!warmupDone && ++warmupCycles >= WARMUP_CYCLES) {
        warmupDone = true;
        if (node.getIntervalMs() > 0) {
            node.setIntervalMs(0);                      // fase 2: intervalo da EEPROM
            Serial.println(F("WARMUP:done->eeprom"));
        } else {
            Serial.println(F("WARMUP:done->backend"));  // C_SET V_VAR1 já cancelou
        }
    }
}
```

**Por quê:** após um reboot, sem warm-up o nó demora 1 intervalo completo (ex.: 30 min) para enviar
a primeira leitura. O warm-up garante dados rápidos e confirma a reconexão com o gateway.

**Regras que não se negociam:**

| Regra | Por quê |
|---|---|
| Contar **ciclos**, nunca `millis()` | `smartSleep()` para o `millis()` no sono: "5 min de `millis()`" viram ~60 ciclos (~35 min de relógio). E o `millis()` corre enquanto `_begin()` espera o gateway — numa queda geral um warm-up por tempo nasce expirado em `setup()` |
| Encerrar com `setIntervalMs(0)`, **nunca** `setInterval(<constante>)` | `setInterval(30)` descarta o intervalo que o operador persistiu na EEPROM (`begin()` já o carregou). O nominal de fábrica vai no env: `-D M360_DEFAULT_INTERVAL=<min>` |
| Checar o retorno de `setIntervalMs()` | Devolve `false` em `ALWAYS_ON`/`REPEATER` (no-op) e fora de `[M360_MIN_INTERVAL_MS, MAX]` |

**Precedência:** um `C_SET V_VAR1` recebido durante o warm-up cancela o override na hora e persiste
o novo intervalo na EEPROM — o comando externo sempre tem prioridade. `setIntervalMs(0)` não toca em
`_interval`, então a transição posterior é inócua.

**`_intervalMs` não persiste** na EEPROM — é zerado no constructor a cada boot.
O nó deve rearmar o warm-up em `setup()` a cada vez que inicializa.

> Nós `M360_ALWAYS_ON` e `M360_PASSIVE` **não** usam este padrão: `ALWAYS_ON` envia já no
> primeiro `process()` (`lastRun == 0`) e `setIntervalMs()` rejeita o perfil; `PASSIVE` só lê sob
> comando externo.

---

# Passo 0 — Em que hardware o nó vai rodar?

Antes de elicitar sensores, decidir o **caminho**:

| Caminho | Quando | O que se cria |
|---|---|---|
| **A — Placa M360-LP** (`hardware/m360-lp/`, Pro Mini 3,3 V / 8 MHz) | Nó sensor da família de placas: sondas de solo pelo 74HC4051, DS18B20, DHT, bateria/solar ou fonte 5 V | **Só** `src/DRY/horta/nos/lp/config/<nó>/nodeConfig.h` + um `[env:lp_XX]`. Nenhum `.cpp` novo |
| **B — Hardware próprio** (Nano, Pro Mini 16 MHz, M360-ACT, protótipo) | Qualquer outro caso | Pasta `src/DRY/horta/nos/<nomeNó>/` com `.cpp` + `sensorDrivers` (Passos 3–7) |

> Decisão de 2026-10-14 (ver `nos/lp/README.md`): na LP, **um firmware por placa, uma
> configuração por nó**. Nem `.cpp` por nó (os Nós 01 e 02 eram cópias byte a byte com
> seis rótulos de diferença), nem binário genérico configurado em campo (todos os drivers
> linkados sempre, rótulos fora da Flash, e depende de pareamento no gateway e backend —
> o spine `m360-generic-nodes-autoconfig` continua como evolução futura, não como base).

## 0.A — Caminho A: nó na M360-LP

### Estrutura (já existe — não recriar)

```
src/DRY/horta/nos/lp/
├── lpNode.cpp            nó declarativo comum (hooks, M360Node, warm-up, corte de bateria)
├── lpPins.h              pinagem fixa da placa + códigos LP_SRC_* para o campo `pin`
├── lpDrivers.h/.cpp      VSW, ordem de ligar/desligar, despacho das leituras
├── drivers/              soilMux.cpp · dht.cpp · onewire.cpp · battery.cpp (cada um atrás de #if)
└── config/<nó>/nodeConfig.h   ← o ÚNICO arquivo que o nó novo acrescenta
```

### `nodeConfig.h` — o que o agente escreve

```cpp
#pragma once
#define LP_NODE_NAME     "LP-<Nome>"     // F() exige literal: fica numa macro
#define LP_NODE_VERSION  "1.0.0"

// Alimentação — exatamente uma (lpNode.cpp falha a compilação se houver 0 ou 2):
#define LP_POWER_FIXED_5V 1     // 5 V em J_VIN → M360_ALWAYS_ON
// #define LP_POWER_BATTERY 1   // TP4056+DW01 em J_VIN → M360_LOW_POWER + bateria pelo A6
                                //   opcionais: LP_VBAT_CUTOFF (3,0 V), LP_VBAT_GAIN/OFFSET

// Drivers compilados (0 = o arquivo e a lib dele ficam fora do binário):
#define LP_HAS_SOIL     1
#define LP_HAS_DHT      0       // LP_DHT_TYPE DHT11 (padrão) ou DHT22
#define LP_HAS_ONEWIRE  0

#ifdef LP_NODE_ITEMS_SCOPE      // só lpNode.cpp vê este bloco; os drivers só leem as flags
static const char L_X[] PROGMEM = "A_1m_10cm";
// childId | kind | S_* | V_* | pin = LP_SRC_* | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
    { 1,  M360::M360_SENSOR, S_MOISTURE, V_LEVEL, LP_SRC_SOIL(0),  0, 1, M360_LABEL(L_X), false, 0 },
    { 11, M360::M360_SENSOR, S_TEMP,     V_TEMP,  LP_SRC_DHT_TEMP, 1, 1, ...,             false, 0 },
    { 12, M360::M360_SENSOR, S_HUM,      V_HUM,   LP_SRC_DHT_HUM,  1, 1, ...,             false, 0 },
    { 7,  M360::M360_SENSOR, S_TEMP,     V_TEMP,  LP_SRC_DS18B20,  1, 1, ...,             false, 0 },
};
#endif
```

O campo `pin` **não é um pino**: é a fonte da leitura (`LP_SRC_SOIL(ch)` = canal 0–7 do
74HC4051, `LP_SRC_DHT_TEMP`, `LP_SRC_DHT_HUM`, `LP_SRC_DS18B20`). Convenção B de R5, a mesma
do pino virtual do Nó 99, e por isso `setupPins()` é omitido (R7). Canal ↔ porta J1
(`ARQUITETURA_FAMILIA.md` §7): ch0/ch1 estaca de 1 m (10/30 cm), ch2/ch3 de 3 m, ch4/ch5 de
5 m; ch6/ch7 saem pela J2 (reserva).

### Env — o que o agente acrescenta ao `src/DRY/horta/platformio.ini`

```ini
[env:lp_XX]
extends = lp_base
build_flags =
    ${lp_base.build_flags}
    -D MY_NODE_ID=<ID>
    -D MY_DEBUG                         ; só em desenvolvimento
    ; -D M360_DEFAULT_INTERVAL=30       ; só LP_POWER_BATTERY (R13)
    -Isrc/DRY/horta/nos/lp/config/nodeXX
```

`lp_base` já traz placa, `lib_ldf_mode = chain+`, as libs e os pinos de rádio (RFM95W:
`MY_RFM95_RST_PIN=8`; nRF24 legado no J5: `MY_RF24_CE_PIN=2`). O rádio ativo vem de
`[common] radio_flags`, nunca do env do nó.

### O que o hardware impõe e o firmware comum já cumpre — não reescrever

| Regra | Onde está |
|---|---|
| VSW (D4) liga em **LOW**; sobe no primeiro item e desce no último | `lpNode.cpp::readItem()` + `lpDrivers.cpp` |
| S0–S2 em LOW **antes** de cortar o VSW; A1/A2 em `INPUT` puro com o VSW desligado | `powerDownSensors()` |
| D3 (reed) em `INPUT_PULLUP`, nunca saída; ímã força leitura e envio de todos os children (polling em `ALWAYS_ON`, `setWakeInterrupt()` em `LOW_POWER`); A4/A5 ficam sem TWI até existir driver I2C | `lpPins.h`, `lpNode.cpp`, `README.md` |
| Bateria = 3 × A6 × VCC(bandgap)/1023, corte de TX abaixo de `LP_VBAT_CUTOFF` | `drivers/battery.cpp`, `lpNode.cpp::loop()` |
| Warm-up pós-boot (R13) | `lpNode.cpp`, só com `LP_POWER_BATTERY` |

### Entregáveis do caminho A

```
src/DRY/horta/nos/lp/config/nodeXX/nodeConfig.h
src/DRY/horta/platformio.ini   ([env:lp_XX])
src/DRY/horta/inventario.md    (§1 + seção do nó — R12; na coluna Pino, "LP chN")
```

Depois: `pio run -e lp_XX` e o checklist do Passo 8 (itens 18–20). Usar o Passo 1 apenas
para elicitar child IDs, rótulos, alimentação e sensores (1.1, 1.3, 1.7, 1.8); os itens
1.2, 1.4, 1.5 e 1.6 já estão decididos pela placa.

## 0.B — Caminho B: hardware próprio

Seguir os Passos 1 a 8 abaixo.

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

Opções suportadas pelo projeto (AVR + MySensors; rádio de `[common] radio_flags`):

```
Arduino Nano (ATmega328P 5V/16MHz)   ← board: nanoatmega328
Arduino Pro Mini 5V/16MHz            ← board: pro16MHzatmega328
Arduino Pro Mini 3.3V/8MHz           ← board: pro8MHzatmega328
Arduino Mega 2560                    ← board: megaatmega2560
Placa M360-LP (Pro Mini 3,3V/8MHz)   ← caminho A do Passo 0: env lp_XX, sem .cpp novo
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
[ ] M360_REPEATER    — fonte fixa + -D MY_REPEATER_FEATURE no env:
                       ALWAYS_ON que encaminha mensagens de outros nós
```

## 1.4 Rádio

O rádio ativo (nRF24L01+ ou RFM95W) vem de `[common] radio_flags` no
`src/DRY/horta/platformio.ini`, nunca do env do nó — para placas que aceitam os dois rádios
(família M360), cujo env usa `${common.flags}`. Hardware próprio **só com nRF24** usa
`${proto_rf24.flags}` no lugar de `${common.flags}`, como os protótipos em campo. Abaixo, só os pinos.

### nRF24L01+

```
Pino CE  (default D9):
Pino CSN (default D10):
Potência RF: [ ] LOW  [ ] HIGH (default)  [ ] MAX
```

### RFM95W (rádio ativo hoje)

```
Pino de RESET do rádio (MY_RFM95_RST_PIN; LP = 8, ACT = 17/A3, GW = não ligado):
DIO0 / NSS: padrão da lib (D2 / D10 no AVR; D1 / D8 no ESP8266) — só definir se fugir disso
```

> ⚠️ **No Arduino Nano, CE/CSN são fixos em D9/D10.** A placa não aceita
> remapeamento via `#define MY_RF24_CE_PIN` / `MY_RF24_CSN_PIN`. Se o nó usa
> SoftwareSerial (RS485, GPS) ou outro periférico que dispute D9/D10, mova o
> **periférico**, nunca o rádio.
> Em Pro Mini e ESP8266 o remapeamento funciona (o protótipo do gateway, `d1_mini_gateway`, usa
> `MY_RF24_CE_PIN=2`/`CSN=15`; a M360-GW, `d1_mini_gw`, usa `MY_RF24_CE_PIN=5`).

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
S_MOISTURE / V_LEVEL  — umidade de solo (ADC bruto 0–1023, alto = seco — convenção dos Nós 1/2)
S_WATER / V_FLOW      — vazão (L/min — convenção do projeto, ver nó 99)
S_WATER_QUALITY / V_PH  — pH
S_WATER_QUALITY / V_EC  — condutividade elétrica
S_BINARY / V_STATUS   — atuador liga/desliga
S_CUSTOM / V_VAR1..5  — grandeza sem tipo padrão
```

Opções adicionais por item:

```
reportIntervalMin (0 = sempre envia se mudou):
wakeOnRadio (true = solicita estado ao gateway após acordar):
flags bit0 (true = multiplica valor × 100 antes de enviar):
```

> ⚠️ O campo `readSamples` do `M360ItemDef` **não é lido pela biblioteca** —
> `_readCb(i)` é chamado uma única vez por item, por ciclo. Preencher com `1`.
> Se o nó precisa de média, faça a amostragem dentro do próprio driver.
> Consequência importante: um driver **consumptivo** (que zera um acumulador ao
> ler, como o contador de pulsos de vazão) é seguro nesse contrato.

> Se usar MUX CD74HC4067: informe o número do canal MUX (0–15).
> O agente codifica automaticamente como `pin = MUX_CHANNEL_OFFSET + canal` (ex: canal 3 → pin 103).

---

## 1.8 Warm-Up Pós-Boot (apenas para `M360_LOW_POWER`)

```
Duração do warm-up (ciclos, padrão 10):
Intervalo de warm-up (segundos, padrão 30):
Intervalo nominal de fábrica (minutos, opcional): → vira `-D M360_DEFAULT_INTERVAL=<min>` no env
```

> Se o usuário não informar, usar os padrões: 30 s / 10 ciclos. O nominal é sempre o valor da
> EEPROM (child 254); a constante de fábrica só vale com EEPROM virgem e nunca entra no `.cpp`.
> Para `M360_ALWAYS_ON` e `M360_PASSIVE`, omitir — ver R13.

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
| Conflito de pino com o rádio | nRF24: CE e CSN; RFM95W: DIO0, NSS e RESET — todos únicos |
| Conflito de pino com Serial (D0/D1) | Pinos D0 e D1 reservados para UART |
| Conflito de pino com interrupções | D2=INT0, D3=INT1 — confirmar se necessário |

Emitir relatório antes de gerar qualquer código.

## 2.2 Artefatos existentes

Verificar se já existe pasta `src/DRY/horta/nos/<nomeNó>/`:

- **Sim**: analisar o que pode ser aproveitado
- **Não**: criar a estrutura completa

---

# Passo 3 — Estrutura de Arquivos a Criar

```
src/DRY/horta/nos/<nomeNó>/
├── <nomeNó>.cpp             ← sketch declarativo (NUNCA hardware aqui)
├── sensorDrivers.h          ← interface pública do driver
├── sensorDrivers.cpp        ← implementação física dos sensores
├── esquema_eletrico.md      ← documentação de pinagem
└── diagrama_blocos.svg      ← diagrama visual do hardware
```

> O sketch fica **direto na pasta do nó**. O subdiretório `withLibDRY/` foi
> abandonado — nenhum nó do projeto o utiliza.
> **Caminho A (M360-LP): nada disto é criado** — só `nos/lp/config/<nó>/nodeConfig.h` (Passo 0).

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
// Escolha UMA convenção (ver R5) e mantenha .h e .cpp coerentes — nunca childId:
float readNodeItem(uint8_t nodeIndex);              // Convenção A: índice em NODE_ITEMS[]
void  writeNodeItem(uint8_t pin, bool state);       // Convenção B: pino real ou virtual MUX
                                                    // Omitir write se não há atuadores
```

## 4.2 `sensorDrivers.cpp`

Regras de implementação:

- O parâmetro de `readNodeItem` e `writeNodeItem` segue a convenção escolhida em R5: **índice**
  no `NODE_ITEMS[]` (A) ou **pino** real/virtual `NODE_ITEMS[i].pin` (B) — nunca `childId`
- Para nós com MUX (convenção B), `IS_MUX_CH(pin)` e `MUX_CH(pin)` recebem o **pino**, não o índice
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

# Passo 5 — Geração do Sketch Principal (`<nomeNó>.cpp`)

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

// ===== RÓTULOS (na Flash) =====
// label é const __FlashStringHelper*: literal "..." direto no NODE_ITEMS[] não compila.
static const char L_X[] PROGMEM = "Label";
static const char L_Y[] PROGMEM = "Rele";

// ===== DEFINIÇÃO DOS ITENS DO NÓ =====
// childId | kind | presentType | valueType | pin | intervalMin | samples | label | wakeOnRadio | flags
static const M360::M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID_X, M360::M360_SENSOR,   S_TEMP,   V_TEMP,   -1, 1, 3, M360_LABEL(L_X), false, 0 },
    { CHILD_ID_Y, M360::M360_ACTUATOR, S_BINARY, V_STATUS,  5, 0, 1, M360_LABEL(L_Y), true,  0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

// ===== BUFFERS (alocação estática — sem heap; sem messages[] desde a M360-DRY 2.0.0) =====
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

// ===== INSTÂNCIA DO MOTOR =====
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, lastValues, nNoUpdates,
                           M360::<PERFIL>);

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
    node.begin(F("<NomeAmigavel>"), F("1.0"));
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
> **Se `M360_LOW_POWER`**: incluir o bloco warm-up de R13 em `setup()` e `loop()`.

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
    +<src/DRY/horta/nos/<nomeNó>/>

monitor_speed = ${common.monitor_speed}

lib_deps =
    mysensors/MySensors@^2.3.2
    ; adicionar bibliotecas específicas do nó aqui

build_flags =
    ${common.flags}
    -D MY_NODE_ID=<ID>
    ; rádio ativo vem de [common] radio_flags — aqui só os pinos que fogem do padrão:
    ; -D MY_RFM95_RST_PIN=<RST>     ← RFM95W (obrigatório se o RESET estiver ligado)
    ; -D MY_RF24_CE_PIN=<CE>        ← nRF24 (ficam inertes com o RFM95W ativo)
    ; -D MY_RF24_CSN_PIN=<CSN>
    ; -D MY_DEBUG                   ← apenas em desenvolvimento
    ; -D MY_RSSI_LOG_INTERVAL=30000 ← se RSSI log habilitado no Passo 1.6
```

> ⚠️ A flag correta é `MY_RF24_CSN_PIN` (com N), não `MY_RF24_CS_PIN`.
> ⚠️ Não existe `MY_RADIO_TIMEOUT_MS` no MySensors 2.3.2: um `-D` dela não faz nada.
> Se o nó usa bibliotecas escolhidas por `#if` (como a LP), acrescentar `lib_ldf_mode = chain+`.

---

# Passo 7 — Criação dos Diagramas

## 7.1 `esquema_eletrico.md`

Documento Markdown com tabela de conexões. Usar como referência:
`src/DRY/horta/nos/99nodeReles/esquema_eletrico.md`

Conteúdo mínimo:
- Tabela pino → componente → sinal
- Alimentação (VCC/GND)
- Observações de conflito ou lógica Active-LOW
- Tabela de Child IDs com tipo MySensors

## 7.2 `diagrama_blocos.svg`

Diagrama visual SVG representando:
- MCU (placa)
- Módulo de rádio (RFM95W no mezanino M360-RF ou nRF24L01+)
- Sensores e atuadores
- Alimentação e pino VCC controlado (se houver)

Usar como referência: `src/DRY/horta/gateway/diagrama_blocos.svg`

---

# Passo 8 — Checklist de Anti-Padrões (pré-entrega)

Verificar cada item antes de entregar o código:

| # | Anti-padrão | Verificação |
|---|-------------|-------------|
| 1 | `#define MY_*` em `.cpp` ou `.h` | Buscar `MY_` fora do `platformio.ini` |
| 2 | Rótulo literal `"..."` no `NODE_ITEMS[]`, ou `M360_LABEL()` sobre string em RAM | `label` é `const __FlashStringHelper*`: o literal não compila; `M360_LABEL()` sobre RAM compila e envia lixo — usar array `PROGMEM` |
| 3 | `NODE_ITEMS[]` sem `static const` | Confirmar declaração |
| 4 | Buffers sem `static` | `lastValues`, `nNoUpdates` (sem `messages[]` desde a M360-DRY 2.0.0) |
| 5 | `Serial.begin()` em `setup()` | Deve estar em `before()` |
| 6 | `initSensors()` em `setup()` | Deve estar em `before()` |
| 7 | `delay()` dentro de `readNodeItem()` ou `writeNodeItem()` | Substituir por `wait()` |
| 8 | `writeNodeItem(NODE_ITEMS[i].childId, ...)` | `childId` não é índice nem pino: usar `nodeIndex` ou `NODE_ITEMS[i].pin`, conforme a convenção do driver (R5) |
| 9 | `node.setupPins()` com pinos virtuais MUX (100+) | Omitir; init em `initSensors()` |
| 10 | `EEPROM.put()` direto no nó | Usar `M360::saveInterval()` |
| 11 | `CHILD_ID_*` como índice de array (`lastValues[CHILD_ID_X]`) | Usar `nodeIndex` ou constante `IDX_*` |
| 12 | `MY_RF24_CS_PIN` em vez de `MY_RF24_CSN_PIN` | Verificar flag no `.ini` |
| 13 | `build_src_filter` com caminho errado | Deve partir de `src/DRY/horta/nos/` — o `src_dir` do projeto é a raiz do repo |
| 14 | childId 254 ou 255 em `NODE_ITEMS[]` | Reservados para intervalo e bateria |
| 15 | Ordem errada de includes (`M360.h` antes de `MySensors.h`) | Causa banner truncado + hang; ordem correta: `Arduino.h` → `MySensors.h` → `M360.h` → `sensorDrivers.h` |
| 16 | `src/DRY/horta/` alterado sem atualizar `inventario.md` | Ver R12 — inventário é contrato com o Node-RED |
| 17 | `M360_LOW_POWER` sem warm-up, warm-up medido por `millis()`, ou transição com `setInterval(<constante>)` | Ver R13 — sem warm-up o nó demora 1 intervalo para a 1ª leitura; `millis()` para no sono; a constante descarta o intervalo da EEPROM |
| 18 | Nó na M360-LP com `.cpp` próprio, ou `lpNode.cpp`/`drivers/` editados para um nó só | Caminho A do Passo 0: o nó é só `config/<nó>/nodeConfig.h` + env; o comum muda para todos ou não muda |
| 19 | `nodeConfig.h` sem `#ifdef LP_NODE_ITEMS_SCOPE` em volta de `NODE_ITEMS[]`, ou com `LP_HAS_*` ausente em vez de `0` | Os drivers incluem o `nodeConfig.h` antes de `M360.h`; `#if` de macro ausente vale 0, mas explícito é o contrato |
| 20 | Na LP, `pin` com número de pino físico (A0, 5…) ou `node.setupPins()` | `pin` carrega `LP_SRC_*`; pino real ali faz `pinMode` em linha de campo ou de rádio |

---

# Entregáveis Obrigatórios

Ao finalizar apresentar lista de arquivos criados/modificados e walkthrough:

## Arquivos

```
src/DRY/horta/nos/<nomeNó>/<nomeNó>.cpp
src/DRY/horta/nos/<nomeNó>/sensorDrivers.h
src/DRY/horta/nos/<nomeNó>/sensorDrivers.cpp
src/DRY/horta/nos/<nomeNó>/esquema_eletrico.md
src/DRY/horta/nos/<nomeNó>/diagrama_blocos.svg
src/DRY/horta/platformio.ini  (env adicionado)
src/DRY/horta/inventario.md   (nó e children acrescentados — R12)
```

## Resumo de configuração

```
✓ MY_NODE_ID      : <valor>
✓ Perfil          : M360_<PERFIL>
✓ Placa           : <board>
✓ Rádio / pinos   : <RFM95W: RST D<X>> ou <nRF24: CE D<X> / CSN D<Y>>
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
8. Se LOW_POWER: como o warm-up (R13) funciona — fase 1 (N ciclos de 30 s) vs fase 2 (`setIntervalMs(0)` → intervalo da EEPROM, com eco no child 254), por que se conta ciclos e não `millis()`, e como um `C_SET V_VAR1` durante o warm-up tem prioridade e cancela o override
