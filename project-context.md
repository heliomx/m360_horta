# Contexto do Projeto — M360 Horta

Referência viva para sessões de IA e novos colaboradores. Captura decisões de design,
contratos de integração e armadilhas já encontradas — para não rederivá-las.

---

## Stack

```
Nós físicos (AVR)          Gateway (ESP8266)          Nuvem / UI
──────────────────         ──────────────────         ───────────────
M360Node + MySensors  ──►  M360Gateway + MQTT  ──►   Node-RED + Dashboard
(lib/M360-DRY/)            (src/DRY/horta/gateway/)   (src/DRY/horta/nodered/flows.json)
```

Plataforma: PlatformIO · Arduino/AVR + ESP8266 · MySensors RF24 · MQTT · Node-RED

---

## Contrato da Fronteira Gateway → MQTT

> ⚠️ **Os dois gateways compilam com `-D M360_NATIVE_MQTT=1`.**
> Isso troca o envelope JSON pelo **formato nativo MySensors**: os identificadores
> viajam no tópico e o payload é o valor bruto. O esquema JSON adiante só vale
> se a flag for removida do `platformio.ini`.

### Formato nativo (modo em produção)

```
{prefixo}/out/{nodeId}/{sensorId}/{command}/{ack}/{type}    payload bruto, ex: "23.4"
{prefixo}/in/{nodeId}/{sensorId}/{command}/{ack}/{type}     payload bruto, ex: "1"
```
Exemplo — ligar a Solenóide A do nó 99:
`m360/DF/0000/in/99/31/1/0/2` com payload `1`
(`command 1` = C_SET, `type 2` = V_STATUS).

Montado por `Translator::buildNativeTopic()` e lido pelo nó "Decodificador
Nativo MySensors" no Node-RED, que reconstitui o objeto `{nodeId, sensorId,
command, ack, type, payload, direction}` a partir de `parts[4..8]`.

### Tópicos publicados pelo gateway

| Tópico | Quando | Conteúdo |
|---|---|---|
| `m360/{UF}/{CAR}/out/{n}/{s}/{c}/{a}/{t}` | Leitura de sensor, ACK de transporte | Valor bruto |
| `m360/{UF}/{CAR}/out` | Heartbeat do gateway | JSON |
| `m360/{UF}/{CAR}/out/events` | Descoberta de nó, timeout, reconexão | JSON de evento |
| `m360/{UF}/{CAR}/gateway/status` | Métricas periódicas do gateway | JSON de métricas |

### Tópico consumido pelo gateway

| Tópico assinado | Publicado por | Ação |
|---|---|---|
| `m360/{UF}/{CAR}/in/#` | Node-RED / externo | `processMQTTCommandNative()` → `send()` ao nó |

> A assinatura leva `/#` **apenas** em modo nativo — ver `MQTTManager::buildTopicIn()`.

### Esquema JSON — mensagem de sensor (`direction:"sensor"`)

```json
{
  "nodeId":    99,
  "sensorId":  38,
  "command":   1,
  "ack":       0,
  "type":      2,
  "payload":   "1",
  "timestamp": 12345,
  "description": "Status",
  "direction": "sensor"
}
```
> `sensorId 38` = `CHILD_ID_NFT_PUMP` na faixa normativa de atuação (31-40).

### Esquema JSON — ACK de transporte (`direction:"ack"`)

Publicado por `publishTransportAck()` em `libDryGatewayMqtt.cpp` quando
`send(outMsg, true)` retorna `true`.

```json
{
  "nodeId":    99,
  "sensorId":  255,
  "command":   1,
  "ack":       1,
  "type":      2,
  "payload":   "1",
  "timestamp": 12345,
  "description": "Status",
  "direction": "ack"
}
```

**Campo crítico:** `nodeId` é injetado manualmente com `targetNodeId`.
`outMsg.getSender()` retorna `0` (gateway) em mensagens de saída — NÃO usar
como `nodeId` do ACK.

---

## Mecanismo de ACK — Regras Definitivas

### O que MySensors NÃO faz no gateway

`send(outMsg, true)` solicita ACK de transporte RF24. O ACK é consumido
**internamente** pela camada MySensors — **`receive()` NÃO é chamado** para esse retorno.

```
send(outMsg, true)
    ↓
[MySensors transport layer aguarda ACK do rádio]
    ↓ ACK recebido
send() retorna true   ← único sinal disponível
receive() NÃO é chamado
```

### O que o gateway DEVE fazer

Publicar o ACK manualmente no MQTT imediatamente após `send()` retornar `true`:

```cpp
bool success = send(outMsg, true);
if (success) publishTransportAck(outMsg, targetNodeId);
```

### Armadilha: outMsg.getSender() == 0

Em mensagens **enviadas** pelo gateway, `getSender()` retorna `0` (ID do próprio
gateway), não o ID do nó de destino. Para construir o JSON de ACK correto:

```cpp
// ERRADO — nodeId seria 0
doc["nodeId"] = outMsg.getSender();

// CORRETO — usar o targetNodeId explicitamente
doc["nodeId"] = targetNodeId;
```

`publishTransportAck()` já implementa este padrão: usa `Translator::toJSON()` como
base e sobrescreve `nodeId` com `targetNodeId` antes de publicar.

---

## Contrato do Sincronizador ACK (Node-RED)

### Entradas esperadas pelo nó função "Sincronizador ACK / Timeout"

| `msg.topic` | Fonte | Ação |
|---|---|---|
| `m360/DF/0000/out` | MQTT in → JSON node → Sincronizador | Verificar se é ACK do comando pendente |
| `control` | Inject "Ativar/Desativar" | Ativar ou desativar sincronismo |
| qualquer outro | UI template, inject de comando | Tratar como novo comando a enfileirar |

### Critério de reconhecimento de ACK

```javascript
var isConfirmation = (
    data.nodeId == targetNode &&
    (targetSensor == 255 || data.sensorId == targetSensor) &&
    (data.direction === 'ack' || data.ack === 1 || (data.command === 1 && data.type === 2))
);
```

- `targetSensor == 255` → broadcast: aceitar qualquer sensorId do mesmo nó
- Três formas de confirmação aceitas em ordem de confiabilidade:
  1. `direction:"ack"` — ACK de transporte publicado pelo gateway (preferencial)
  2. `ack:1` — bit ACK MySensors explícito
  3. `command:1 && type:2` — o nó devolveu V_STATUS (fallback). O `type:2` é
     **obrigatório**: sem ele, qualquer leitura de sensor do mesmo nó (que também
     chega como `command:1`) seria contada como confirmação.

> O gateway só pede ACK de transporte para `V_STATUS`
> (`withAck = (outMsg.getType() == V_STATUS)`), e o Sincronizador só aguarda ACK
> para `command:1 && type:2`. As duas pontas concordam: comandos administrativos
> (REPRESENT, FORCE_UPDATE) são fire-and-forget.

### Cabeamento obrigatório no fluxo Node-RED

```
[MQTT in m360/+/+/out/#]
    → [Decodificador Nativo MySensors]
        → [Sincronizador ACK / Timeout]   ← ACK path (msg.topic == prefix+'/out')
        → [Mapeia nós] / [Filtros ...]    ← roteamento para dashboard/mapa
```

O **Decodificador Nativo MySensors** precisa estar **entre** o MQTT in e o
Sincronizador: é ele que transforma o tópico nativo em objeto e reescreve
`msg.topic` para `{prefixo}/out`, que é como o Sincronizador distingue ACK de
comando novo. Ele também aceita o envelope JSON, caso `M360_NATIVE_MQTT` seja
removido.

---

## Mapeamento de Variáveis MySensors — Decisões de Design

### V_LEVEL usado como umidade de solo (Nós 01, 02 e 05)

Os nós de solo enviam umidade com `type=V_LEVEL (37)`, não `V_PERCENTAGE (3)` —
limitação histórica do firmware.

**Escala:** os nós publicam **ADC bruto (0–1023)**, não percentual. Valor alto =
solo seco (sensor resistivo). O `Motor de Regras Canteiro B` no Node-RED opera
diretamente nessa escala (limiares 350 e 500), portanto **não** existe conversão
para 0–100 em lugar nenhum da cadeia.

**No Node-RED:** não há remapeamento `V_LEVEL→V_PERCENTAGE`. Os filtros de solo
aceitam `type` 3, 35 ou 37 diretamente, e identificam o nó por `nodeId` ou pelo
prefixo do rótulo (`A_`, `B_`) vindo da apresentação.

**Solução ideal de longo prazo:** converter no firmware para `V_PERCENTAGE` com
escala 0–100 — exige atualizar simultaneamente os limiares do motor de regras.

---

## Nós Físicos — Referência Rápida

| Nó | Env | Hardware | Perfil | Children | Sensores / Atuadores |
|---|---|---|---|---|---|
| 0 | `d1_mini_gateway` | ESP8266 D1 Mini | — | — | WiFi + MQTT + MySensors GW |
| 01 | `nano_01nodeSolo3d` / `ProMini_01nodeSolo3d` | Nano 5V / Pro Mini 16MHz | LOW_POWER | 1–6 | 6× S_MOISTURE V_LEVEL (canteiro A) |
| 02 | `nano_02nodeSolo3d` | Nano 5V | LOW_POWER | 1–6 | 6× S_MOISTURE V_LEVEL (canteiro B) |
| 04 | `ProMini_04noodeSolarMini` | Pro Mini 16MHz | LOW_POWER | 1, 11, 12 | DHT11 (temp/umid ar) + DS18B20 (temp solo) |
| 99 | `nano_99reles` / `nano_99reles_rep` | Nano 5V | ALWAYS_ON | 11, 12, 21, 31–39 | 9 relés (7 via MUX + 2 nativos) + DHT11 + vazão YF-S201 |
| 11 | `pro16MHz_miniDHT` | Pro Mini 16MHz | ALWAYS_ON | — | Kit Hélio: DHT11 |

> **Faixas normativas de child ID** (`M360Constants.h`): Solo 1–10 · Clima 11–20 ·
> Hidrometria 21–30 · Atuação 31–40 · 253 debug · 254 intervalo · 255 bateria.
> Os child IDs do nó 99 são **contrato com o Node-RED** — alterá-los quebra os
> botões do dashboard e o motor de regras de irrigação.
> Os nós **5** (Solo MUX), **13** (ZTS) e **80** (Aqua) foram removidos do projeto.
> Fontes dos nós 13 e 80 recuperáveis em `0ccf66a`; o nó 5, no histórico do git.

---

## Armadilhas Registradas

### 1. `receive()` não é chamado para ACKs de transporte
Ver seção "Mecanismo de ACK". Publicar manualmente com `publishTransportAck()`.

### 2. `outMsg.getSender()` retorna 0 no contexto de envio do gateway
Ver seção "Armadilha: outMsg.getSender() == 0". Usar `targetNodeId` explicitamente.

### 3. `msg.topic` do Sincronizador depende da fonte
Comandos da UI chegam com `msg.topic` diferente de `m360/.../out`. O Sincronizador
usa `msg.topic` para diferenciar ACK de comando — o cabeamento deve garantir isso.

### 4. `send(msg, true)` bloqueia o ESP8266
`send()` com ACK é síncrono — aguarda o ACK do rádio por até
`MY_TRANSPORT_TIMEOUT_MS × MY_TRANSPORT_RETRIES` ms. Para Nó 99 (always-on),
isso é aceitável. Para nós em sleep, pode causar timeout.

### 5. Nó 99 — pinos virtuais MUX
Atuadores via MUX usam `pin = MUX_CHANNEL_OFFSET + canal` (100–115).
`setupPins()` deve ser omitido; pinos gerenciados em `initSensors()`.

### 6. NRF24L01 no Nano — CE/CSN fixos
Nano não aceita `#define MY_RF24_CE_PIN` para mover CE/CSN. Usar sempre D9/D10.
Periféricos que conflitem com D9/D10 devem ser movidos.

### 7. `messages[]` precisa de `+3` nos nós da lib M360-DRY
`M360Node::begin()` escreve **incondicionalmente** em `_messages[_count]`,
`[_count+1]` e `[_count+2]` — intervalo (254), bateria (255) e debug (253).
Declarar `messages[NODE_ITEMS_COUNT + 2]` provoca escrita fora dos limites do
array, sem erro de compilação. O motor **legado** (`node_engine.h`) escreve
apenas até `[COUNT+1]`, e nele `+2` é o correto — não confundir os dois.

### 8. `readSamples` do `M360ItemDef` não é implementado
O campo existe na struct mas a biblioteca nunca o lê: `_readCb(i)` é chamado uma
única vez por item, por ciclo. Médias devem ser feitas dentro do driver.
Corolário útil: drivers **consumptivos** (que zeram um acumulador ao ler, como o
contador de pulsos de vazão do nó 99) são seguros nesse contrato.

### 10. `inventario.md` deve acompanhar toda mudança em `src/DRY/horta/`
`src/DRY/horta/inventario.md` é a referência única de nós e child IDs e o
contrato com o `flows.json`. Regra do projeto: qualquer alteração de código em
`src/DRY/horta/` é refletida no inventário na **mesma** entrega — inclusão,
remoção ou renumeração de child, mudança de `label`/`S_*`/`V_*`/`pin`/atributos,
inclusão ou remoção de nó, troca de perfil de energia, ou mudança de escala e
unidade. Registrada em `CLAUDE.md`, `AGENTS.md`, no SKILL (SSoT) e como R12 do
workflow `m360-node-factory`.

### 9. Child IDs do nó 99 são contrato com o Node-RED
O dashboard e o motor de irrigação endereçam os atuadores por número fixo
(31/32/33 solenóides, 38/39 bombas) e leem o clima nos children 11/12.
Renumerar o firmware sem atualizar o `flows.json` faz os comandos serem
descartados **em silêncio** — `M360Node::handleMessage()` casa `childId` exato e
não responde a IDs desconhecidos. O sintoma é timeout no Sincronizador ACK.
Após renumerar, disparar **REPRESENT** para o `Mapeia nós` limpar os children
antigos do `mys_nodes`.

---

## Decisões de Design Registradas

| Decisão | Alternativa rejeitada | Motivo |
|---|---|---|
| `send(outMsg, true)` + `publishTransportAck()` | Esperar `receive()` com `isAck()` | `receive()` não é chamado para ACKs de transporte no gateway |
| Identificar o nó de solo por `nodeId` **ou** prefixo do rótulo (`A_`/`B_`) | Depender só de `mys_nodes` em tempo real | Evita race condition: o SET pode chegar antes da PRESENTATION ser processada |
| Alias de sensor preservado de `m.payload` na apresentação | `getNomeAmigavel()` genérico | Firmware já define nomes canônicos ("A_1m_10cm", "Umidade ZTS") |
| Fallback do mapa com nomes reais do firmware para Nó 01 | Genéricos "Canal N" | Nomes estão no código-fonte e são estáveis |
| Versionamento no **GitHub** | Repositório no Google Drive com link simbólico | Sincronização de arquivos sobre um `.git` corrompe o índice e faz arquivos desaparecerem |
| Isolamento de build via `workspace_dir` no TEMP | Manter `.pio` dentro da árvore do projeto | Evita lock de arquivos e poluição do repositório |

---

## Repositório e Infraestrutura de Build

- **Repositório central: GitHub.** O versionamento e a sincronização entre
  máquinas são feitos por `git push` / `git pull`, não por sincronização de
  arquivos.
- **Descontinuado:** o repositório **não** fica mais no Google Drive e **não** é
  mais acessado por link simbólico local. O clone é uma pasta comum de trabalho.
- **Consequência prática:** trabalho não commitado existe apenas na máquina
  local. Commitar cedo e com frequência é o único mecanismo de proteção.
- **Isolamento de compilação:** `workspace_dir = ${sysenv.TEMP}/pio_builds/${sysenv.USERNAME}/m360_horta` no `platformio.ini` raiz mantém os binários fora
  da árvore versionada.
- **Monorepo:** o `platformio.ini` raiz agrega `src/DRY/horta/platformio.ini` e
  `src/DRY/kit-helio/platformio.ini` via `extra_configs`, e define `src_dir = .`
  — por isso todo `build_src_filter` começa em `src/DRY/...`.
