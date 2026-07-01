# Contexto do Projeto — M360 Horta

Referência viva para sessões de IA e novos colaboradores. Captura decisões de design,
contratos de integração e armadilhas já encontradas — para não rederivá-las.

---

## Stack

```
Nós físicos (AVR)          Gateway (ESP8266)          Nuvem / UI
──────────────────         ──────────────────         ───────────────
M360Node + MySensors  ──►  M360Gateway + MQTT  ──►   Node-RED + Dashboard
(lib/M360-DRY/)            (src/DRY/gateway/)         (src/nodered/)
```

Plataforma: PlatformIO · Arduino/AVR + ESP8266 · MySensors RF24 · MQTT · Node-RED

---

## Contrato da Fronteira Gateway → MQTT

### Tópicos publicados pelo gateway

| Tópico | Quando | Conteúdo |
|---|---|---|
| `m360/{UF}/{CAR}/out` | Leitura de sensor, ACK de transporte, heartbeat | JSON (ver esquema abaixo) |
| `m360/{UF}/{CAR}/out/events` | Descoberta de nó, timeout, reconexão | JSON de evento |
| `m360/{UF}/{CAR}/gateway/status` | Heartbeat periódico do gateway | JSON de métricas |

### Tópico consumido pelo gateway

| Tópico | Publicado por | Ação |
|---|---|---|
| `m360/{UF}/{CAR}/in` | Node-RED / externo | `processMQTTCommand()` → `send()` ao nó |

### Esquema JSON — mensagem de sensor (`direction:"sensor"`)

```json
{
  "nodeId":    99,
  "sensorId":  16,
  "command":   1,
  "ack":       0,
  "type":      2,
  "payload":   "1",
  "timestamp": 12345,
  "description": "Status",
  "direction": "sensor"
}
```

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
    (data.direction === 'ack' || data.ack === 1 || data.command === 1)
);
```

- `targetSensor == 255` → broadcast: aceitar qualquer sensorId do mesmo nó
- Três formas de confirmação aceitas em ordem de confiabilidade:
  1. `direction:"ack"` — ACK de transporte publicado pelo gateway (preferencial)
  2. `ack:1` — bit ACK MySensors explícito
  3. `command:1` — C_SET de retorno do nó (fallback defensivo)

### Cabeamento obrigatório no fluxo Node-RED

```
[MQTT in m360/.../out]
    → [JSON parser]
        → [Sincronizador ACK / Timeout]   ← ACK path (msg.topic == prefix+'/out')
        → [Separar ACK / Leituras]        ← roteamento para debug/mapa
```

O JSON parser precisa estar **entre** o MQTT in e o Sincronizador para garantir
que `msg.payload` seja objeto (não string) quando chegar ao Sincronizador.

---

## Mapeamento de Variáveis MySensors — Decisões de Design

### V_LEVEL usado como umidade de solo (Nós 01 e 13)

Nós 01 e 13 enviam umidade de solo com `type=V_LEVEL (37)`, não `V_PERCENTAGE (3)`.
Isso é uma limitação histórica do firmware — a escala é 0 (seco) → 100 (água), que
semanticamente é uma percentagem.

**Solução no Node-RED:** remapeamento por `nodeId` no `Translator Json`:

```javascript
var LEVEL_COMO_UMIDADE = {1: true, 13: true};
if (tipo === 'V_LEVEL' && LEVEL_COMO_UMIDADE[m.nodeId]) {
    tipo = 'V_PERCENTAGE'; descricao = 'Percentual'; unidade = '%';
}
```

**Solução ideal de longo prazo:** alterar os NODE_ITEMS dos nós 01 e 13 para usar
`V_PERCENTAGE` diretamente, eliminando o remapeamento no Node-RED.

---

## Nós Físicos — Referência Rápida

| Nó | Hardware | Perfil | Sensores / Atuadores |
|---|---|---|---|
| 0 (Gateway) | ESP8266 D1 Mini | — | WiFi + MQTT + MySensors GW |
| 01 | Pro Mini 3.3V/8MHz | LOW_POWER | 18× S_MOISTURE V_LEVEL (canteiros A/B) |
| 04 | Pro Mini 3.3V/8MHz | LOW_POWER | Clima (futuro) |
| 13 | Nano 5V | PASSIVE | ZTS-3002 (umidade/temp/CE/pH/N/P/K), Hall, LDR, relé |
| 80 | Nano 5V | ALWAYS_ON | pH, EC, DS18B20, ultrassônico, 4× vazão YF-S201 |
| 99 | Nano 5V | ALWAYS_ON | 9 relés (MUX CD74HC4067 + nativos) + DHT11 |

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

---

## Decisões de Design Registradas

| Decisão | Alternativa rejeitada | Motivo |
|---|---|---|
| `send(outMsg, true)` + `publishTransportAck()` | Esperar `receive()` com `isAck()` | `receive()` não é chamado para ACKs de transporte no gateway |
| Remapeamento `V_LEVEL→V_PERCENTAGE` por `nodeId` no Node-RED | Consultar `mys_nodes` em tempo real | Evita race condition: SET pode chegar antes da PRESENTATION ser processada |
| Alias de sensor preservado de `m.payload` na apresentação | `getNomeAmigavel()` genérico | Firmware já define nomes canônicos ("A_1m_10cm", "Umidade ZTS") |
| Fallback do mapa com nomes reais do firmware para Nó 01 | Genéricos "Canal N" | Nomes estão no código-fonte e são estáveis |
