---
name: bmad-mysensors-node-coding
description: Padrões obrigatórios de codificação DRY para nós MySensors e gateway MQTT no projeto M360 Horta
---

# MySensors Node Coding Standards — M360 Horta

> [!IMPORTANT]
> **Single Source of Truth (SSoT):** Este arquivo é a ÚNICA referência autoritativa. Padronizado com base no código real em `src/DRY/`.

---

## 1. Arquitetura e Motor DRY

Toda lógica presente em dois ou mais nós pertence à biblioteca `lib/M360-DRY`.
O motor legado (`node_engine`) foi removido em 2026-10-17; o código está no histórico do git, no commit `13671e7`.

### Estrutura de Diretórios
```
src/DRY/
├── horta/                    ← Sub-projeto Horta (platformio.ini próprio)
│   ├── nos/
│   │   ├── lp/               ← Placa M360-LP: firmware comum + config/<nó>/nodeConfig.h
│   │   └── nomeDoNo/         ← hardware próprio: sensorDrivers.h/cpp + nomeDoNo.cpp
│   ├── gateway/              ← ESP8266 (infraestrutura vive em lib/M360-DRY)
│   └── nodered/flows.json
└── kit-helio/                ← Sub-projeto Kit Hélio (platformio.ini próprio)
    ├── nos/
    └── gateway/
```
> O `platformio.ini` da raiz agrega os sub-projetos via `extra_configs` e
> define `src_dir = .`, logo todo `build_src_filter` parte de `src/DRY/...`.

---

## 2. Padrões de Implementação (Nós)

### 2.1 Buffers e rótulos dos nós

**Nós da lib M360-DRY (`M360::M360Node`, 2.0.0+) — sem `messages[]`, rótulos na Flash:**
```cpp
static const char L_TEMP[] PROGMEM = "Temperatura Ar";
static const M360::M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID_TEMP, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 0, 1, M360_LABEL(L_TEMP), false, 0 },
};
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];
static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, lastValues, nNoUpdates, PERFIL);
// presentation(): node.begin(F("Nome"), F("1.0"));
```
> - A lib monta cada envio — inclusive intervalo (254), bateria (255) e debug (253) —
>   num **único `MyMessage` interno**. Para enviar o estado de um item fora do fluxo
>   da lib (failsafe, preempção de MUX), use `node.sendItemState(i, estado)`.
> - `label` é `const __FlashStringHelper*`: um literal `"..."` no `NODE_ITEMS[]` **não
>   compila**, e `begin()` sem `F()` também não — de propósito. `M360_LABEL()` só aceita
>   array `PROGMEM`; sobre uma string em RAM, compila e envia lixo como rótulo.
> - Motivo: ~710 B de SRAM a menos no Nó 99 (medido), espaço que a assinatura
>   MySensors exige.

> - `reportIntervalMin` e `readSamples` **não são lidos** pela lib: preencher `0` e `1`. O filtro de
>   envio é fixo — Δ > 0,05 ou 10 ciclos sem mudança (`M360Node::_readAndSendAll()`).

O ciclo de vida é `before()` → `initSensors()`, `presentation()` → `node.begin()`,
`setup()` → `onRead`/`onWrite`, `loop()` → `node.process()`, `receive()` → `node.handleMessage()`.

---

## 3. Comandos e Eventos Customizados (V_CUSTOM / V_VAR1)

O código implementa padrões específicos para comunicação avançada. Todas as constantes de comando estão em `lib/M360-DRY/src/M360Constants.h`.

| Constante | Payload | Mecanismo | Efeito |
|-----------|---------|-----------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | `V_CUSTOM` / `C_SET` | Leitura imediata de todos os sensores; em PASSIVE liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | `V_CUSTOM` / `C_SET` | Re-anuncia todos os children via `present()` sem resetar estado interno; Serial: `REPRES:OK` |
| `CMD_DEBUG_NET` | `"DEBUG_NET"` | `V_CUSTOM` / `C_SET` | Responde diagnóstico de rede (`P:<parent> D:<dist> R:<S/N>`) pelo child 253 |
| — | Intervalo numérico | `V_VAR1` ou `V_VAR5` em childId 254 | Atualiza e persiste intervalo na EEPROM |

Todos os comandos acima são processados automaticamente por `node.handleMessage(msg)` — a `receive()` do nó não precisa de código adicional.

**Reapresentação (`CMD_REPRESENT`):** O motor chama `_rePresent()` que re-executa `present()` para cada child ID registrado. `lastValues[]` e `nNoUpdates[]` **não** são resetados, evitando burst de mensagens no próximo ciclo. Útil após reinício do broker MQTT ou do Home Assistant.

---

## 4. Gateway — Padrões do `libDryGatewayMqtt.cpp`

- **Node Tracking:** `M360::Registry` (`M360Registry.h`): `node_discovered`, `node_lost` (15 min), `node_reconnected`.
- **Formato MQTT:** os gateways compilam com `M360_NATIVE_MQTT=1`. Subida em
  `m360/{UF}/{CAR}/out/{nodeId}/{childId}/{command}/{ack}/{type}` com o valor bruto no payload;
  descida em `.../in/...` no mesmo formato. Sem a flag, o envelope JSON
  `{ "nodeId", "sensorId", "command", "type", "payload", "timestamp" }` em `.../out`.
- **Events:** Eventos de transporte (reconexão, nó perdido) são publicados em `{topic}/events`.
- **Config:** A configuração do gateway (WiFi/MQTT) fica na EEPROM a partir de **521** (`M360DeviceConfig`, CRC), gravada campo a campo.

---

## 4.1 Inventário — Sincronização Obrigatória

Qualquer alteração de código em `src/DRY/horta/` **deve** ser refletida em
`src/DRY/horta/inventario.md` na **mesma** entrega. O inventário é a referência
única de nós e child IDs, e o contrato com o `flows.json` do Node-RED.

Gatilhos: incluir/remover/renumerar `childId`; alterar `label`, `S_*`, `V_*`,
`pin`, `reportIntervalMin`, `wakeOnRadio` ou `flags`; incluir/remover nó;
trocar perfil de energia; mudar escala, unidade ou tipo do payload; alterar
pinagem.

> Renumerar um `childId` sem atualizar o `flows.json` faz o nó **ignorar o
> comando silenciosamente** — `handleMessage()` casa `childId` exato e não
> responde a IDs desconhecidos. Não há erro de compilação nem de log; o sintoma
> é timeout no Sincronizador ACK.

---

## 5. Checklist Anti-Padrões (Verificar antes de Commit)

- [ ] ❌ **Macro `MY_*` em `.cpp`/`.h`** → só no `platformio.ini` (env ou `[common]`).
- [ ] ❌ **EEPROM.put() direto no nó** → Use `M360::saveInterval()`.
- [ ] ❌ **Hardcoding de strings de comando** → Use as constantes de `M360Constants.h` (`CMD_FORCE_UPDATE`…).
- [ ] ❌ **Perfil de energia ambíguo** → exatamente um `M360::M360_*` no construtor; na LP, um `LP_POWER_*`.
- [ ] ❌ **`messages[]` num nó M360-DRY ou rótulo em RAM** → a lib 2.0.0 não recebe buffer; rótulo em `PROGMEM` + `M360_LABEL()` (ver §2.1).
- [ ] ❌ **Código depois de `sleep()`** → `request()`/`wait()` vão **antes** do `sleep()`; o que vem depois só roda no próximo despertar.
- [ ] ❌ **`src/DRY/horta/` alterado sem atualizar `inventario.md`** → Ver §4.1.

---

## 6. Referência Rápida MySensors API (v2.x)

Este guia resume os elementos core da biblioteca MySensors necessários para o desenvolvimento de nós.

### 6.1 Ciclo de Vida do Nó
- **`before()`**: Chamado antes de `begin()`. Útil para configurar pinos e inicializar sensores que precisam estar prontos para o rádio.
- **`presentation()`**: Onde o nó se apresenta ao gateway e registra seus sensores (`present()`).
- **`setup()`**: Inicialização padrão do Arduino após o MySensors estar pronto.
- **`loop()`**: Loop principal. Evite `delay()` longos para não bloquear o transporte de rádio. Use `wait()` se necessário.
- **`receive(const MyMessage &message)`**: Callback acionada ao receber mensagens (C_SET ou C_REQ).

### 6.2 Funções Core da API
- **`present(uint8_t childId, uint8_t type, [const char *alias])`**: Registra um sensor (S_...).
- **`send(MyMessage &msg, [bool ack])`**: Envia uma mensagem. `ack=true` solicita confirmação.
- **`request(uint8_t childId, uint8_t type, [uint8_t destination])`**: Solicita um valor ao controller.
- **`wait(uint32_t ms)`**: Pausa o loop mas continua processando rádio e mensagens.
- **`sleep(uint32_t ms / uint8_t interrupt, uint8_t mode, uint32_t ms)`**: Coloca o rádio e a MCU em baixo consumo.

### 6.3 Tipos Comuns (S_..., V_..., I_...)
| Categoria | Tipo (Exemplos) | Uso |
| :--- | :--- | :--- |
| **Sensores (S_)** | `S_TEMP`, `S_HUM`, `S_BINARY`, `S_WATER`, `S_CUSTOM` | Usado em `present()`. |
| **Variáveis (V_)** | `V_TEMP`, `V_HUM`, `V_STATUS`, `V_FLOW`, `V_CUSTOM` | Usado em `send()` / `request()`. |
| **Internas (I_)** | `I_BATTERY_LEVEL`, `I_TIME`, `I_SKETCH_NAME` | Configuração e status interno. |

### 6.4 Macros de Configuração (neste projeto, só no `platformio.ini`)
- **Radio**: `MY_RADIO_RF24` (rede da estufa, `[common] radio_flags`) ou `MY_RADIO_RFM95` (redes LoRa novas, `[lora_net]`); ver workflow `m360-node-factory`, Passo 1.4.
- **Node**: `MY_NODE_ID 254`, `MY_REPEATER_FEATURE`.
- **Gateway**: `MY_GATEWAY_ESP8266`, `MY_GATEWAY_MQTT_CLIENT`.
- **Debug**: `MY_DEBUG`.

---

## 7. Padrões de Qualidade e Ferramentas (Core MySensors)

Para alinhar o desenvolvimento com o core do MySensors, os seguintes padrões devem ser observados (baseados no `style.cfg` oficial):

- **Indentação**: Sempre use **Tabs**.
- **Chaves**: Sempre use chaves `{}`, mesmo em declarações de uma única linha (padrão `1tbs`).
- **Nomenclatura**: Use `camelCase` para funções e variáveis. Evite notação húngara (ex: `iCount`).
- **Largura de Linha**: Máximo de 100 caracteres.
- **Macros**: Use parênteses extras e identação em definições multi-linha.
- **Análise Estática**: O código deve ser compatível com `cppcheck` (evitar warnings de estilo e portabilidade).

---
*Referência conferida contra os nós atuais (`nos/lp/`, `01`/`02nodeSolo3dNano`, `04noodeSolarMini`, `99nodeReles`), a lib M360-DRY 2.1.0 e os padrões oficiais do MySensors Core.*
