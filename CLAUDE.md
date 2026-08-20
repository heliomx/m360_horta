# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projeto
Sistema IoT de monitoramento agrícola M360 Horta.  
**Plataforma:** PlatformIO · Arduino/AVR · MySensors RF24 · ESP8266  
**Arquitetura:** Monorepo com dois sub-projetos agregados por `extra_configs`:
- **Horta** — gateway em `src/DRY/horta/gateway/`, nós em `src/DRY/horta/nos/`
- **Kit Hélio** — gateway em `src/DRY/kit-helio/gateway/`, nós em `src/DRY/kit-helio/nos/`

O `platformio.ini` da raiz define `src_dir = .`, portanto todo `build_src_filter`
parte de `src/DRY/...`. Os envs vivem nos `platformio.ini` de cada sub-projeto.

---

## Comandos PlatformIO

```bash
# Build de um ambiente específico
pio run -e <env>

# Flash (build + upload)
pio run -e <env> -t upload

# Monitor serial
pio device monitor -e <env>

# Build + upload + monitor em sequência
pio run -e <env> -t upload && pio device monitor -e <env>

# Análise estática da lib M360-DRY (deve ser executado antes de todo commit na lib)
pio check -e check_m360_dry

# Build de todos os envs padrão
pio run
```

### Ambientes disponíveis

Definidos em `src/DRY/horta/platformio.ini` e `src/DRY/kit-helio/platformio.ini`.

| `env`                       | Hardware                    | Porta | Nó |
|-----------------------------|-----------------------------|-------|----|
| `d1_mini_gateway`           | ESP8266 D1 Mini             | COM5  | 0 (Gateway Horta) |
| `nano_01nodeSolo3d`         | Arduino Nano (5V)           | —     | 1  |
| `ProMini_01nodeSolo3d`      | Arduino Pro Mini (5V/16MHz) | —     | 1  |
| `nano_02nodeSolo3d`         | Arduino Nano (5V)           | —     | 2  |
| `ProMini_04noodeSolarMini`  | Arduino Pro Mini (5V/16MHz) | —     | 4  |
| `nano_99reles`              | Arduino Nano (5V)           | COM4  | 99 |
| `nano_99reles_rep`          | Arduino Nano (5V)           | COM4  | 99 (repeater) |
| `d1_mini_kit_helio_gateway` | ESP8266 D1 Mini             | COM5  | 0 (Gateway Kit Hélio) |
| `pro16MHz_miniDHT`          | Arduino Pro Mini (5V/16MHz) | —     | 11 |
| `check_m360_dry`            | — (análise estática)        | —     | —  |

> Os nós **5** (Solo MUX), **13** (ZTS) e **80** (Aqua) foram removidos do
> projeto. Ver `src/DRY/horta/inventario.md`.

---

## Bootstrap de Credenciais (Gateway)

Antes de compilar o gateway, copiar e preencher:

```bash
cp include/M360Credentials.h.example include/M360Credentials.h
```

O arquivo `include/M360Credentials.h` define AP SSID/senha, WiFi STA, MQTT server/usuário/senha, UF e CAR. Está no `.gitignore`.

---

## Skill Obrigatória — MySensors Node Coding & Gateway

Ao trabalhar em qualquer arquivo dentro de `src/DRY/` (nós **ou** gateway), aplicar **obrigatoriamente** os padrões definidos em:

- **Referência consolidada (SSoT):** `.agent/skills/bmad-mysensors-node-coding/SKILL.md`

---

## Regra Obrigatória — Manter o Inventário Sincronizado

Qualquer alteração de código em `src/DRY/horta/` **deve** ser refletida em
[`src/DRY/horta/inventario.md`](src/DRY/horta/inventario.md) na **mesma** entrega.
O inventário é a referência única de nós e child IDs, e o contrato com o
`flows.json` do Node-RED — se ficar defasado, comandos passam a ser descartados
em silêncio pelos nós, sem erro de compilação nem de log.

**Gatilhos que exigem atualizar o inventário:**

| Mudança no código | O que atualizar |
|---|---|
| Incluir, remover ou renumerar um `childId` | Tabela do nó + §1 (faixa de children) + §8 se for endereçado pelo Node-RED |
| Alterar `label`, `S_*`, `V_*`, `pin`, `reportIntervalMin`, `wakeOnRadio` ou `flags` | Linha correspondente na tabela do nó |
| Incluir ou remover um nó (env / `MY_NODE_ID`) | §1, seção própria do nó e Apêndice B |
| Trocar o perfil de energia | Cabeçalho da seção do nó e §1 |
| Mudar escala, unidade ou tipo do payload | §2 (tipo do payload) e §9 (escalas) |
| Alterar pinagem do nó | Coluna de pino + `esquema_eletrico.md` do nó |

**Verificação antes de entregar** — conferir que cada `childId` de
`NODE_ITEMS[]` / `nodeItems[]` aparece no inventário com o mesmo `label`,
`S_*`, `V_*` e demais atributos. Divergir aqui é o mesmo que quebrar o contrato
com o Node-RED.

> Alterar um `childId` sem atualizar o `flows.json` faz o nó **ignorar o comando
> silenciosamente** — `M360Node::handleMessage()` casa `childId` exato e não
> responde a IDs desconhecidos. O sintoma é timeout no Sincronizador ACK.

### Resumo das regras críticas para nós legados (`src/DRY/horta/nos/shared/`)

**Nunca escrever manualmente** o que já existe como macro no `node_engine.h`:

| Padrão proibido | Substituição obrigatória |
|---|---|
| Corpo manual de `presentation()` | `NODE_ENGINE_PRESENTATION(name, ver)` |
| 6 declarações de globais repetidas | `NODE_ENGINE_DEFINE_GLOBALS()` |
| Bloco validação/save/send de intervalo | `NODE_ENGINE_HANDLE_INTERVAL(msg)` |
| Loop V_STATUS para atuadores | `NODE_ENGINE_HANDLE_ACTUATORS(msg)` |
| Bloco diagnóstico de rede | `NODE_ENGINE_CHECK_TRANSPORT()` |
| Bloco teste de conectividade | `NODE_ENGINE_TEST_CONNECTIVITY()` |
| Ciclo static battCycle manual | `NODE_ENGINE_PROCESS_BATTERY(N)` |
| Loop `pinMode`/`digitalWrite` | `nodeEngine_setupPins()` |
| Loop `lastValues=NAN` / `nNoUpdates=0` | `nodeEngine_initArrays(lv, nu, cnt)` |
| String `"teste_do_gateway"` hardcoded | `CMD_FORCE_UPDATE` (de `config.h`) |
| `MyMessage messages[N]` fixo | `messages[NODE_ITEMS_COUNT + 2]` |
| Código após `sleep()` | mover `request()`/`wait()` para **antes** |

### Regras críticas do Gateway (`src/DRY/horta/gateway/`)

| Padrão proibido | Regra |
|---|---|
| Tópico MQTT como string literal | Sempre `buildTopicOut(config)` / `buildTopicIn(config)` |
| `EEPROM.put(&config)` direto | `saveConfig()` campo-a-campo com CRC |
| MQTT ou WebServer em `before()` | Exclusivos de `setup()` |
| `setupWiFi()` fora de `before()` | Exclusivo de `before()` |
| `mqttClient.loop()` em modo AP | Checar `WiFi.getMode() == WIFI_AP` primeiro |
| Lógica de infraestrutura em `libDryGatewayMqtt.cpp` | Módulo dedicado em `lib/M360-DRY/` |
| `atoi()` em segmento de tópico | `parseTopicByte()` — `atoi("xx")` vale 0 e o comando some sem rastro |
| Comando malformado descartado com `println` | `reportCommandRejected()` — publica o motivo em `.../out/events` |
| `send()` fora de `dispatchCommand()` | Ponto único: valida, envia e reporta sucesso **ou** falha |
| Montar `JsonDocument` no caminho nativo | `Translator::fromNative()` — os campos já vieram do tópico |

---

## Biblioteca M360-DRY (`lib/M360-DRY/`)

A lib canônica para nós e gateway. **Novos nós usam esta lib**, não os arquivos legados em `src/DRY/horta/nos/shared/`.

### Componentes principais

| Header | Classe/Struct | Responsabilidade |
|---|---|---|
| `<M360.h>` | — | Header centralizador (importa tudo) |
| `M360Node.h` | `M360::M360Node` | Motor de ciclo de vida do nó (AVR) |
| `M360ItemDef` | struct | Definição declarativa de sensor/atuador |
| `M360PowerProfile` | enum | `M360_LOW_POWER` / `M360_ALWAYS_ON` / `M360_PASSIVE` |
| `M360Gateway.h` | `M360::M360Gateway` | Orquestrador do loop do gateway (ESP8266) |
| `M360Translator.h` | `M360::Translator` | Serialização/deserialização JSON ↔ MyMessage |
| `M360Registry.h` | `M360::Registry` | Rastreamento de nós ativos e timeouts |
| `M360Config.h` | `M360::M360DeviceConfig` | Config EEPROM do gateway (offset 521+) |

### Padrão de nó com lib M360-DRY

Todo nó usa dois arquivos:

- **`withLibDRY/noX.cpp`** — **Puramente declarativo.** Define `NODE_ITEMS[]`, buffers estáticos, instancia `M360Node`, implementa os 5 hooks MySensors (`before`, `presentation`, `setup`, `loop`, `receive`). Não contém leitura de hardware.
- **`sensorDrivers.h/cpp`** — **Camada física.** Implementa as leituras brutas (ADC, I2C, Modbus etc.) e é conectada ao motor via callbacks `onRead` / `onWrite`.

Estrutura mínima de um nó:

```cpp
// noX.cpp
#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

static const M360::M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, "Label", false, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

static MyMessage messages[NODE_ITEMS_COUNT + 3];  // SEMPRE +3 (intervalo, bateria, debug)
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_LOW_POWER);

namespace M360 { void powerUp() { ... } void powerDown() { ... } }

void before()       { initSensors(); }
void presentation() { node.begin("Nome", "1.0"); }
void setup()        { node.onRead(readNodeItem); node.onWrite(writeNodeItem); }
void loop()         { node.process(); }
void receive(const MyMessage& msg) { node.handleMessage(msg); }
```

### Perfis de energia

| Perfil | Modo | Uso |
|---|---|---|
| `M360_LOW_POWER` | `smartSleep()` — acorda por watchdog | Sensores de campo a bateria (Nós 01, 04, 13) |
| `M360_ALWAYS_ON` | Timer por `millis()`, sem sleep | Atuadores em fonte fixa (Nós 80, 99) |
| `M360_PASSIVE` | `smartSleep()` — lê só sob comando | Sensores pesados (Modbus RS485) |

---

## Arquitetura do Gateway

**Fluxo de inicialização (`before()` → `setup()` → `loop()`):**

1. `before()`: EEPROM init → `loadConfig()` → WiFi STA (config válida) **ou** WiFi AP (A0 em GND ou config inválida)
2. `setup()`: `setupWebServer()` → `setupMQTT()` (apenas modo STA) → `gateway.begin(...)`
3. `loop()`: delega tudo para `M360::M360Gateway::loop()` — WiFi reconnect, MQTT reconnect, `server.handleClient()`, LEDs, heartbeat, node timeouts

**MQTT topics:**
- Upstream (rádio → broker): `m360/{UF}/{CAR}/out` (JSON com `nodeId`, `sensorId`, `command`, `type`, `payload`)
- Downstream (broker → rádio): `m360/{UF}/{CAR}/in`
- Eventos: `m360/{UF}/{CAR}/out/events`

**Modo AP / Factory Reset:**
- A0 em GND durante o boot → modo configuração (SSID: `M360-Config`, portal em `192.168.4.1`)
- Factory reset via web portal ou mantendo A0 em GND por 3 s nos primeiros 30 s

**LEDs de status:** Verde = WiFi+MQTT OK | Amarelo piscando = WiFi OK, sem MQTT | Vermelho piscando = sem WiFi

---

## Estrutura Relevante

```
src/DRY/
├── horta/                       Sub-projeto Horta (platformio.ini próprio)
│   ├── gateway/                 libDryGatewayMqtt.cpp (usa M360::M360Gateway)
│   ├── nodered/flows.json       Fluxos Node-RED (dashboard + irrigação + ACK)
│   ├── inventario.md            Nós e child IDs — contrato com o Node-RED
│   └── nos/
│       ├── shared/              Motor legado (node_engine.h) — não usar em novos nós
│       ├── 01nodeSolo3dNano/    Solo resistivo — 6 canais nativos (Nó 1, LP)
│       ├── 02nodeSolo3dNano/    Solo resistivo — 6 canais nativos (Nó 2, LP)
│       ├── 04noodeSolarMini/    DHT11 + DS18B20, alimentação solar (Nó 4, LP)
│       └── 99nodeReles/         9 atuadores (7 MUX + 2 nativos) + DHT11 + vazão YF-S201 (Nó 99, ON)
└── kit-helio/                   Sub-projeto Kit Hélio (platformio.ini próprio)
    ├── gateway/                 libDryGatewayMqtt.cpp
    └── nos/miniDHT/             DHT11 (Nó 11, ON)

lib/M360-DRY/             Biblioteca canônica (M360Node, M360Gateway, M360Translator…)
include/
    M360Credentials.h     Credenciais locais (gitignored)
    M360Credentials.h.example  Template de credenciais
```

> A infraestrutura do gateway (LEDs, webserver, WiFi, MQTT) vive em `lib/M360-DRY/`.
> O antigo diretório `gateway/ngm/` e o subdiretório `withLibDRY/` não existem mais.

---

## Convenções de Código

- **Linguagem dos comentários e logs Serial:** português
- **IDs reservados:** 254 = Intervalo (`V_VAR1`), 255 = Bateria (`V_VOLTAGE`), 253 = Debug remoto (`V_TEXT`, apenas lib M360-DRY — ver `M360Node::sendDebug()` / `CMD_DEBUG_NET`)
- **EEPROM nós (AVR):** endereços 512–515 via `M360Config` / `nodeEngine_saveInterval()` — nunca `EEPROM.put()` direto
- **EEPROM gateway (ESP):** região 0–511 = MySensors (reservado), 512–520 = M360NodeConfig, 521+ = `DeviceConfig`/`M360DeviceConfig` com CRC
- **Perfil de energia:** definir exatamente um dos dois — nunca ambos simultaneamente
- **Precisão float:** 1 casa decimal (`set(val, 1)`)
- **Solo:** escala 0 (seco) → 100 (água)
- **JSON gateway:** `DynamicJsonDocument(512)` para mensagens, `(384)` para heartbeat, `(256)` para eventos
- **Macros MY_\*:** definidas exclusivamente no `platformio.ini` — nunca no `.cpp` após `#include <MySensors.h>`
- **Pinos virtuais MUX (Nó 99):** `pin = MUX_CHANNEL_OFFSET + canal` (100–115) — `setupPins()` deve ser omitido; pinos gerenciados em `initSensors()`

---

## Armadilhas Críticas da Integração MySensors ↔ MQTT ↔ Node-RED

> Estas regras custaram sessões de debugging. Leia antes de tocar em qualquer
> código de gateway ou Node-RED.

### ACK de transporte MySensors no gateway

`send(outMsg, true)` solicitaria eco do destino. O eco **é consumido internamente**
pela camada MySensors — **`receive()` nunca é chamado** para esse retorno. Por isso
o ACK precisa ser publicado à mão, com `publishTransportAck()`.

O gateway usa **`send(outMsg, false)`** deliberadamente, para não bloquear o loop
nem colidir pacotes. Todo o envio passa por `dispatchCommand()`, ponto único:

```cpp
const bool success = send(outMsg, false);
if (success) publishTransportAck(outMsg, targetNodeId);
else         publishTransportEvent("command_send_failed", details, targetNodeId);
```

> **O que esse `success` significa de fato:** apenas o auto-ACK de hardware do
> RF24 do **próximo salto** — não confirmação de que o nó recebeu ou aplicou o
> comando. Com o Nó 99 em modo repetidor na rede, o ACK pode vir do repetidor.
> A confirmação real é o **eco de aplicação do nó** (`M360Node.cpp`, `handleMessage()`
> envia `send(_messages[i].set(state))` depois de acionar o atuador).

### `outMsg.getSender()` retorna 0 no contexto de envio

Em mensagens **enviadas** pelo gateway, `getSender()` retorna `0` (ID do gateway),
não o nó de destino. O JSON de ACK deve usar `targetNodeId` explicitamente —
passar no 3º parâmetro de `Translator::toJSON(msg, isAck, nodeIdOverride)`.
`publishTransportAck()` em `libDryGatewayMqtt.cpp` já faz isso.

O mesmo vale para o tópico no modo nativo: `Translator::buildNativeTopic()` usa
`getSender()` e **não serve** para ACK — o tópico é montado com `targetNodeId`.

### Payload de `V_STATUS`: só `"0"` e `"1"` — tudo mais **desliga**

O nó lê o valor com `MyMessage::getBool()`, que para payload `P_STRING` faz
**`atoi()`**. `"true"`, `"ON"` ou qualquer lixo viram `0` — ou seja, **desligam** o
relé em vez de ligar, indistinguíveis de um OFF legítimo em qualquer log.

No modo nativo (`M360_NATIVE_MQTT=1`), `nodeId`/`sensorId`/`command`/`ack`/`type`
vão no **tópico** e o payload MQTT é só o valor bruto:

```
tópico:  m360/DF/0000/in/99/31/1/0/2
payload: 1
```

Publicar o envelope JSON como payload faz o gateway repassá-lo inteiro como
string ao nó, `atoi()` devolve 0 e **o relé nunca liga** — com ACK confirmado e
notificação de sucesso no caminho todo. Foi exatamente esse o defeito da aba
Irrigação do Node-RED. `Translator::validate()` hoje rejeita isso no gateway, mas
o `mqtt out` do Node-RED serializa objeto para JSON em silêncio: **conferir que
`msg.payload` é string** ao criar qualquer comando de atuação.

### `V_LEVEL` (37) nos Nós 01 e 13 significa umidade de solo

Esses nós usam `V_LEVEL` para solo por decisão histórica. No Node-RED, o
`Translator Json` remapeia `V_LEVEL → V_PERCENTAGE` por `nodeId`. Não mudar sem
atualizar simultaneamente o firmware dos nós.

### Sincronizador ACK — critério de matching

Para comandos broadcast (`sensorId=255`), o ACK pode voltar com qualquer sensorId
do mesmo nó. Condição correta:

A confirmação é o **eco de aplicação do nó**, não o ACK de transporte:

```javascript
data.nodeId == targetNode &&
(targetSensor == 255 || data.sensorId == targetSensor) &&
data.command === 1 && data.type === 2 && data.ack === 0 &&
String(data.payload) === String(currentItem.payload)
```

Cada termo carrega um caso real:

| Termo | Por quê |
|---|---|
| `data.type === 2` | `C_SET` vale 1 e leituras de sensor também chegam como `command === 1`. Sem filtrar por `V_STATUS`, o relatório de clima do Nó 99 — que sai a cada minuto — confirmaria qualquer comando pendente para o nó |
| `data.ack === 0` | Descarta o ACK de transporte que o gateway fabrica em `publishTransportAck()`. Ele só prova que o **próximo salto** respondeu — com o Nó 99 em modo repetidor, pode vir do repetidor. Aceitá-lo dava "confirmado" com o nó desligado, e os 3 retries nunca disparavam na falha de última milha |
| `payload` igual | Evita casar com um eco de outra origem para o mesmo child — p.ex. o failsafe do Nó 99, que envia `0` sozinho ao estourar o tempo máximo ligado |

O `Decodificador Nativo` rotula `ack === 1` como **`direction: 'transport_ack'`**
(não `'ack'`), justamente para que essa distinção não dependa de convenção tácita.
`Mapeia nós` também ignora `ack == 1` ao gravar `values[]`, senão o dashboard
mostraria estado de relé que nenhum nó confirmou.

> Consequência esperada: comando para um child que **não é atuador** (o nó só
> ecoa `kind == M360_ACTUATOR`) agora falha com erro após 3 tentativas, em vez de
> ser falsamente confirmado. É o diagnóstico correto.
