# Funcionalidades do Node-RED — M360 Horta

Referência funcional de [`flows.json`](flows.json). Descreve **o que cada aba faz**,
quais parâmetros governam o comportamento e onde ficam os contratos com o firmware.

> **Este arquivo precisa ser atualizado a cada mudança no Node-RED.**
> Ver [§11 — Regra de manutenção](#11-regra-de-manutenção-obrigatória).

**Servidor:** `https://nr.viridiotech.com.br`
**Broker MQTT:** `mqtt.viridiotech.com.br` (`72.62.142.165:1883`, sem TLS, MQTT 3.1.1)
**Prefixo de tópicos:** `m360/DF/0000` (global `mqtt_topic_prefix`)
**Escopo:** 163 nós Node-RED distribuídos em 7 abas.

---

## 1. Visão geral das abas

| Aba | Papel |
|---|---|
| **MQTT** | Bancada de comandos manuais (injects), watchdog tabular da rede, captura global de erros |
| **Fluxo 2 - Solenóides** | Botões de solenóide A/B/C + painel administrativo de comandos por nó |
| **Fluxo 2 - Bombas** | Botões das bombas NFT e Oxigenação |
| **M360 Horta - ACK Handling** | Núcleo: decodificação MQTT, registro de nós, sincronizador de ACK, gráficos, log MQTT, alertas |
| **Irrigação** | Automação de rega dos canteiros A (timer) e B (motor agroclimático) |
| **IA Repórter (Diário)** | Relatório diário 08:00 via Gemini, enviado por Telegram |
| **Telegram Bot Interativo** | Bot de comandos e boletim meteorológico 4×/dia |

### Dashboard (Node-RED Dashboard 2.0)

Uma única página (`Page 1`) com três grupos:

| Grupo | Conteúdo |
|---|---|
| `Controle Manual` | Botões de atuação, painel administrativo, tabela do watchdog, mapa da rede |
| `Gráficos de Monitoramento` | 7 gráficos de telemetria |
| `Log de Mensagens MQTT` | Tabela de log, exportar CSV, esvaziar |

---

## 2. Contexto compartilhado (variáveis globais e de fluxo)

Estas chaves são o estado vivo do sistema. Alterar seu significado quebra várias abas de uma vez.

| Chave | Escopo | Inicializada em | Conteúdo |
|---|---|---|---|
| `mqtt_topic_prefix` | global | `Configurações Globais M360` | `m360/DF/0000` |
| `mys_nodes` | global (**file**) | `Mapeia nós` | Registro completo de nós: childs, valores, cadência, status |
| `mqtt_logs` | global | `Central Logger MQTT` | Buffer circular de 6000 mensagens MQTT |
| `m360_mailbox` | global | `Sincronizador ACK` | **Caixa Postal** — um comando retido por nó LP, aguardando o despertar |
| `telegram_subscribers` | global | `Configurações Globais M360` | Lista de chats inscritos no bot |
| `TELEGRAM_BOT_TOKEN` / `TELEGRAM_CHAT_ID` | global | `env.get()` do Node-RED | Credenciais do bot |
| `GEMINI_API_KEY` | global | manual | Chave da API Gemini |
| `ack_timeout_ms` | flow | `Configurações Globais M360` | `5000` — timeout base do sincronizador |
| `ack_max_retries` | flow | `Configurações Globais M360` | `3` |
| `nodes_ttl_ms` | flow | `Configurações Globais M360` | `172800000` (48 h) — TTL do registro de nós |
| `ack_sync_enabled` | flow | `Sincronizador ACK` (default `true`) | Liga/desliga a espera de ACK |
| `last_param_values` | flow | `Monitor de Falhas` | Último intervalo confirmado por nó — evita alerta repetido |
| `node_connection_states` | flow | `Monitor de Falhas` | `ONLINE`/`OFFLINE` por nó, para a máquina anti-rajada |
| `last_network_alert_times` | flow | `Monitor de Falhas` | Marcas de cooldown por tipo de alerta |
| `solo_b_readings`, `clima_dht11`, `agro_meteo` | flow | abas de irrigação | Telemetria consolidada para o motor de regras |
| `last_irrig_b_time` | flow | `Motor de Regras Canteiro B` | Marca do soak time |
| `hist_solo_A`, `hist_solo_B`, `hist_clima`, … | flow | nós `Salvar Histórico *` | Séries de 7 dias para replay dos gráficos |

`mys_nodes` usa **armazenamento em arquivo** (`global.get('mys_nodes','file')`) com
fallback para memória — sobrevive a reinício do Node-RED.

---

## 3. Aba **MQTT** — bancada e watchdog

### 3.1 Injects de comando manual

Todos passam por `Pub MQTT Injects (tópico dinâmico)`, que converte o envelope JSON
em **tópico nativo + payload bruto**.

| Inject | Alvo | Payload |
|---|---|---|
| Ligar/Desligar Solenóide A | Nó 99 / child 31 | `"1"` / `"0"` |
| Ligar/Desligar Solenóide B | Nó 99 / child 32 | `"1"` / `"0"` |
| Ligar/Desligar Solenóide C | Nó 99 / child 33 | `"1"` / `"0"` |
| Ligar/Desligar Bomba NFT | Nó 99 / child 38 | `"1"` / `"0"` |
| Ligar/Desligar Bomba Oxi | Nó 99 / child 39 | `"1"` / `"0"` |
| Reboot Gateway | Nó 0 | ação `REBOOT_GATEWAY` |
| Re-apresenta Nó 02 / 04 | child 0, `V_CUSTOM` (48) | `"REPRESENT"` |
| Lê Sensores Nó 02 / 04 / 99 | child 0, `V_CUSTOM` (48) | `"FORCE_UPDATE"` |

### 3.2 `Watchdog da rede` (a cada 60 s)

Varre `mys_nodes` e classifica cada nó numa tabela do dashboard:

| Status | Critério |
|---|---|
| `OFFLINE` | `lastSeen` mais velho que o **timeout dinâmico** do nó |
| `SEM HEARTBEAT` | último heartbeat > 900 s |
| `SEM ACK` | último ACK > 1800 s |
| `ONLINE` | nenhum dos acima |

**Timeout dinâmico**, na ordem de preferência:
1. `n.timeoutSec` — calculado a partir do child 254 anunciado pelo nó;
2. `intervalMin × 60 + Max(120, 50%)` — mesma fórmula do firmware;
3. `cycleMs` observado — cadência medida entre despertares;
4. `450 s` — último recurso.

> Esta é **a mesma fórmula** de `M360::NodeRegistry::registerInterval()`
> (`lib/M360-DRY/src/M360Registry.cpp`). Mudar de um lado sem o outro faz dashboard
> e gateway discordarem sobre quem está offline.

### 3.3 `Global Error Catch`

Nó `catch` de escopo global → debug `Log Erros Críticos`. Captura exceções de qualquer
função das abas.

---

## 4. Abas **Solenóides** e **Bombas** — atuação manual

### 4.1 Caminho de um clique

```
ui-button → Despachante & Feedback Imediato
              ├── pinta o botão de ÂMBAR (pendente)
              └── link out → Sincronizador ACK → Pub MQTT ACK out → mqtt out
```

**O Sincronizador é o único despachante.** Até 26/08/2026 o Despachante tinha um
segundo fio, direto para `Pub MQTT (tópico dinâmico)`, em paralelo ao `link out`.
Esse desvio foi removido, por três motivos:

- publicava o comando **duas vezes** no broker assim que o Sincronizador voltou a
  despachar de fato (ver §5.5, "Discriminação");
- furava a **Caixa Postal**: um comando para nó LP era retido *e* publicado no ar,
  onde se perdia com o nó dormindo;
- tornava invisível a falha do Sincronizador — o comando chegava ao nó pelo desvio,
  então o sintoma aparecia só no feedback visual, longe da causa.

Os nós `Pub MQTT Solenóides` / `Pub MQTT Bombas` continuam nas abas, agora sem
entrada — são a referência do formato de tópico nativo (§4.2).

O retorno vem por `link in → Processador Feedback Visual`:

| `msg.status` | Cor do botão | Duração |
|---|---|---|
| (clique) | âmbar `#f59e0b` | até resolver |
| `success` | verde `#10b981` | fixo no "Ligar" ativo; 1,2 s no "Desligar" |
| `error` | vermelho `#ef4444` | 2 s, depois volta ao estado real |
| `mailbox_enqueued` | índigo `#6366f1` | até o nó LP despertar |
| `mailbox_dispatched` | verde `#10b981` | 1,5 s, depois volta ao estado real |

> Índigo e âmbar são cores distintas de propósito: âmbar é "o rádio está tentando",
> índigo é "o nó está dormindo e nada foi ao ar ainda". Antes de 26/08/2026 os dois
> status de Caixa Postal não eram tratados pelo Processador de Feedback — o botão
> ficava âmbar para sempre, indistinguível de uma falha de ACK.

### 4.2 `Pub MQTT (tópico dinâmico)`

Converte `{nodeId, sensorId, command, ack, type, payload}` em:

```
tópico:  m360/DF/0000/in/{nodeId}/{sensorId}/{command}/{ack}/{type}
payload: String(payload)
```

> **`String(m.payload)` é obrigatório.** O nó resolve `V_STATUS` com
> `MyMessage::getBool()`, que faz `atoi()` — publicar um objeto JSON serializado
> vira `0` e **desliga** o relé com ACK confirmado no caminho todo.
> Ver `inventario.md` §8 e a seção de armadilhas do `CLAUDE.md`.

### 4.3 Painel administrativo (`Processador do Comando UI`)

Três widgets alimentam `flow.ui_selected_*`; o botão **Enviar Comando** dispara a ação.

| Widget | Contexto |
|---|---|
| `Seletor de Nó` (dropdown) | `ui_selected_node_id` — populado por `Atualiza Opções do Dropdown` a partir de `mys_nodes` |
| `Seletor de Ação` (dropdown) | `ui_selected_action` |
| `Intervalo (min)` (number, 1–1440) | `ui_selected_interval` |

Ações disponíveis:

| Ação | Comando emitido |
|---|---|
| **Re-apresentar** | child 0, `V_CUSTOM` (48), `"REPRESENT"` |
| **FORCE_UPDATE** | child 0, `V_CUSTOM` (48), `"FORCE_UPDATE"` |
| **Debug da Rede** | child 0, `V_CUSTOM` (48), `"DEBUG_NET"` |
| **⏱️ Definir Intervalo** | child **254**, `V_VAR1` (24), valor em minutos |

> `Atualiza Opções do Dropdown` faz *dedupe* por assinatura do conjunto de nós e
> nunca envia lista vazia — sem isso o widget engasgava a cada mensagem MySensors.

---

## 5. Aba **ACK Handling** — o núcleo

### 5.1 Entradas MQTT

| Assinatura | Destino |
|---|---|
| `m360/+/+/out/#` | `Decodificador Nativo` + `Translator Json` + logger |
| `m360/+/+/out/events` | `Monitor de Falhas` + logger |
| `m360/+/+/in/#` | logger (marca direção OUT) |
| `m360/+/+/gateway/status` | `Gateway Status + Watchdog` + logger |

### 5.2 `Decodificador Nativo MySensors`

Normaliza os dois formatos num objeto único:

```js
{ nodeId, sensorId, command, ack, type, payload, direction, timestamp }
```

`direction` vale `'sensor'` ou **`'transport_ack'`** quando `ack === 1`.

Distribui para quatro destinos: `Sincronizador ACK`, `Separar ACK / Leituras`,
`Mapeia nós` e `Monitor de Falhas`.

> Essa rotulagem é deliberada. `ack === 1` **nunca vem de um nó**: é o ACK que o
> gateway fabrica em `publishTransportAck()` assim que o rádio confirma o salto.
> Com o Nó 99 como repetidor, pode vir do repetidor. Confundi-lo com confirmação do
> nó dava "sucesso" com o nó desligado.

### 5.3 `Translator Json`

Traduz a mensagem para texto legível com tabelas completas de `C_*`, `S_*`, `V_*`,
`I_*`, e aplica diagnóstico agronômico (`MANEJO360`):

| Grandeza | Faixa ideal | Alerta |
|---|---|---|
| `V_TEMP` | 18–25 °C | fora da faixa |
| `V_PERCENTAGE` (solo) | 60–90 % | seco / excesso de água |
| `V_PH` | 5.8–6.5 | baixo / alto |
| `V_EC` | 1.2–2.2 mS/cm | diluída / concentrada |
| `V_LEVEL` (reservatório) | > 30 % | reservatório baixo |
| `V_VOLTAGE` | > 3.3 V | bateria fraca |

> **Remapeamento `V_LEVEL` → `V_PERCENTAGE`:** feito de forma *programática*, olhando
> se o child foi apresentado como `S_MOISTURE` — não por `nodeId` hardcoded. Os nós
> 01 e 02 usam `V_LEVEL` (37) para umidade de solo por decisão histórica.

### 5.4 `Mapeia nós` — o registro

Constrói `mys_nodes` **exclusivamente** a partir do que o nó apresenta. Sem rótulo
hardcoded por `nodeId`, sem dado de demonstração.

Por nó guarda: `childs` (tipo + rótulo vindos de `present()`), `values`,
`sketchName`/`sketchVersion` (de `I_SKETCH_NAME`/`I_SKETCH_VERSION`), `batteryLevel`,
`batteryVoltage`, `intervalMin`, `timeoutSec`, `cycleMs`, `lastSeen`, `lastHeartbeat`,
`lastAck`, `categoria`.

Comportamentos:
- **Cadência observada** — média móvel `0,7·anterior + 0,3·gap` para gaps > 15 s;
- **Child 254** — grava `intervalMin` e recalcula `timeoutSec`;
- **Re-apresentação** (`command 0` ou `"REPRESENT"`) limpa dados antigos e atualiza o dashboard;
- **`ack == 1` é ignorado** ao gravar `values[]` — senão o dashboard mostraria estado de relé que nó nenhum confirmou;
- **TTL** — nó sem contato há 48 h sai do registro;
- **Categoria derivada** dos tipos de child: Solo, Clima, Atuação, Reservatório ou Gateway.

### 5.5 `Sincronizador ACK / Timeout` + Caixa Postal

Três responsabilidades num nó só: fila serializada de comandos com retry exponencial,
matching de ACK, e **Caixa Postal (Mailbox)** para nós que dormem.

| Parâmetro | Valor |
|---|---|
| Timeout base | `ack_timeout_ms` = 5000 ms |
| Retentativas | `ack_max_retries` = 3 |
| Backoff | `TIMEOUT × 2^tentativa` |
| Liga/desliga | `flow.ack_sync_enabled`, via payload `ENABLE_ACK_SYNC` / `DISABLE_ACK_SYNC` |

#### Caixa Postal — entrega a nós Low Power

Um nó `M360_LOW_POWER` fica acordado ~3 s por ciclo de `smartSleep()`. Comando enviado
fora dessa janela simplesmente se perde. A Caixa Postal resolve isso retendo o comando
até o nó dar sinal de vida.

**Como o nó é classificado LP** (qualquer uma basta):
- `sketchName` contém `[LP]` — sufixo que `M360Node::begin()` acrescenta pelo perfil;
- `nodeId` ∈ {1, 2, 4};
- `cycleMs > 15000` (cadência observada acima de 15 s).

**Fluxo:**

```
comando p/ nó LP
   └─ nó acordou há < 2 s?  ── sim ──> envia imediatamente
                            └─ não ──> global.m360_mailbox[nodeId] = comando
                                        status: 'mailbox_enqueued'

qualquer telemetria do nó LP
   └─ existe m360_mailbox[nodeId]? ── sim ──> despacha AGORA (janela de despertar)
                                              status: 'mailbox_dispatched'
```

A Caixa Postal guarda **um comando por nó** — um novo sobrescreve o pendente.
Cada entrada tem `mqttMsg`, `sensorId`, `value`, `desc` e `enqueuedAt`.

> É por aqui que passa o **Definir Intervalo** para o Nó 4: o comando fica retido e
> é entregue no despertar seguinte, sem depender de acertar a janela na mão.

#### Discriminação entre telemetria e comando novo

A função recebe **duas classes de mensagem na mesma entrada**: telemetria vinda do
`Decodificador Nativo` e comando novo vindo do `link in`. O critério que as separa é:

```js
var isTelemetria = !!(inbound && typeof inbound === 'object' &&
                      inbound.direction !== undefined &&
                      inbound.nodeId !== undefined);
```

`direction` (`'sensor'` ou `'transport_ack'`) é carimbo **exclusivo** do
`Decodificador Nativo` (§5.2). Nenhum comando de UI o possui.

> ⚠️ **Nunca discriminar por `nodeId` + `command`.** Era o teste anterior, e o
> payload de todo botão — `{"nodeId":99,"sensorId":31,"command":1,"type":2,"payload":"0"}`
> — casa com ele. O comando da UI entrava no ramo de telemetria, não encontrava
> nada em `waiting` e morria no `return null` do fim do bloco. Consequência: o
> Sincronizador **nunca entrava em `waiting`**, o eco de aplicação do nó não era
> reconhecido, nenhum timeout era agendado — e o botão ficava **preso em âmbar**,
> sem sucesso nem erro. O comando ainda chegava ao nó pelo fio de desvio (§4.1),
> o que escondia a causa: log do gateway limpo, relé acionado, eco publicado.
> Corrigido em produção em 26/08/2026.
>
> Este é o **espelho** do bug dos injects `ENABLE_ACK_SYNC` / `DISABLE_ACK_SYNC`
> registrado adiante nesta seção: lá, controle caindo no ramo de comando; aqui,
> comando caindo no ramo de telemetria. Os três ramos desta função — **controle**,
> **telemetria**, **comando novo** — têm que ser mutuamente exclusivos por um campo
> próprio, nunca por ausência de campo alheio. Ao mexer em qualquer um, reler os três.

#### Matching de ACK

**Critério de confirmação** — o eco de aplicação do nó, e nada além dele:

```js
data.nodeId == targetNode &&
(targetSensor == 255 || data.sensorId == targetSensor) &&
data.command === 1 && data.type === 2 && data.ack === 0 &&
String(data.payload) === String(currentItem.payload)
```

Cada termo carrega um caso real:

| Termo | Por quê |
|---|---|
| `type === 2` | leituras de sensor também chegam com `command === 1`; sem filtrar por `V_STATUS`, o relatório de clima do Nó 99 confirmaria qualquer comando pendente |
| `ack === 0` | descarta o ACK de transporte; aceitá-lo dava "confirmado" com o nó desligado e os 3 retries nunca disparavam |
| `payload` igual | evita casar com eco de outra origem — p.ex. o failsafe do Nó 99, que envia `0` sozinho |

**Bypass** — comandos que **não** são de relé seguem direto, sem esperar ACK:
`!ack_sync_enabled`, `targetNode === 255`, `command === 3`, ou `!(command===1 && type===2)`.
É por aí que passam REPRESENT, FORCE_UPDATE, DEBUG e **Definir Intervalo** — depois de
passarem, quando aplicável, pela Caixa Postal.

> Consequência: o painel reporta "Enviado" imediatamente para essas ações. Não é
> confirmação de que o nó aplicou — para intervalo, a confirmação real é o eco do
> child 254 em `.../out`, que o `Monitor de Falhas` transforma no alerta
> *"PARÂMETRO CONFIRMADO: Intervalo de Envio"*.

**Controle manual do sincronismo** — dois injects na aba ACK Handling:

| Inject | Payload (string) |
|---|---|
| `Ativar Sincronismo` | `ENABLE_ACK_SYNC` |
| `Desativar Sincronismo (Bypass)` | `DISABLE_ACK_SYNC` |

> Ambos emitiam payload **booleano** com `topic: 'control'`, formato que a versão
> anterior desta função tratava. Ficaram órfãos quando a função passou a comparar
> `msg.payload` com as strings acima: caíam no ramo de "novo comando", não encontravam
> `nodeId` e eram descartados em silêncio. Corrigido em produção em 26/08/2026.
> Ao mexer no contrato de controle desta função, **conferir os dois injects juntos.**

### 5.6 Watchdog do gateway

| Nó | Cadência | Função |
|---|---|---|
| `Gateway Status + Watchdog` | por mensagem | marca `gateway_last_seen`; detecta transição offline→online |
| `Verifica Gateway Offline` | 60 s | alerta se > 120 s sem mensagem |
| `Limpeza TTL mys_nodes` | 300 s | remove nós expirados |
| `Gatilho Mapa de Rede` | 60 s | redesenha o mapa |

### 5.7 Gráficos

`Separar ACK / Leituras` (switch por `payload.direction`) distribui as leituras para
seis filtros. Cada filtro alimenta um `Salvar Histórico *` (retenção **7 dias**,
teto de 3000 pontos) que por sua vez alimenta o gráfico.

| Filtro | Gráfico | Origem |
|---|---|---|
| `Filtro Umidade Solo Canteiro A` | Umidade do Solo — Canteiro A | Nó 01, childs 1–6 |
| `Filtro Umidade Solo Canteiro B` | Umidade do Solo — Canteiro B | Nó 02, childs 1–6 |
| `Filtro Temp/Hum Ar` | Clima — Temp. e Umidade do Ar | Nó 99, childs 11/12 |
| `Filtro Clima SolarMini` | Clima — Nó 04 SolarMini | Nó 04, childs 1/11/12 |
| `Filtro Bateria SolarMini` | Bateria — % e Tensão | `I_BATTERY_LEVEL` + child 255 |
| `Filtro Vazão e Volume` | Vazão (L/s) + Volume acumulado (L) | Nó 99, child 21 (`V_FLOW`) |

Os filtros reconhecem o sensor **pelo tipo apresentado** (`S_MOISTURE`, `S_TEMP`…),
com `nodeId`/`sensorId` apenas como reforço — um nó reapresentado continua sendo
plotado sem edição de flow.

`Replay Histórico Gráficos` e `Replay Solo B no Deploy` repovoam os gráficos após um
deploy, a partir das séries em `flow` e de uma semente embutida no próprio nó.

### 5.8 Log MQTT

`Tag MQTT IN` / `Tag MQTT OUT` marcam a direção e convergem em `Central Logger MQTT`:

- buffer circular de **6000** entradas em `global.mqtt_logs`;
- tabela do dashboard mostra as **100 mais recentes**, com *throttle* de **800 ms**
  para não travar a UI;
- **Exportar CSV** via `GET /api/mqtt-log/export` (`Content-Disposition: attachment`);
- **Esvaziar Log** limpa o buffer.

### 5.9 `Monitor de Falhas de Comunicação & Alertas Telegram`

Máquina de estados anti-rajada. Cooldowns: **5 min** para `node_lost`, **2 min** para
`node_reconnected`, **30 s** para apresentação de sketch, **60 s** para falha de ACK.

Recebe de três fontes: eventos do gateway (`.../out/events`), telemetria decodificada
(4ª saída do `Decodificador Nativo`) e respostas de status do `Sincronizador ACK`.

Alertas emitidos:

| Origem | Gatilho | Mensagem |
|---|---|---|
| Eventos | `node_lost` | Nó inativo, com motivo, timeout aplicado e RSSI |
| Eventos | `node_reconnected` | Comunicação restabelecida |
| Eventos | `node_sketch_name` | Nó identificado na rede |
| Telemetria | child 254, `command 1`, `ack != 1` | **Parâmetro confirmado: intervalo aplicado na EEPROM do nó** |
| Sincronizador | `mailbox_enqueued` | Comando enfileirado — nó em hibernação |
| Sincronizador | `mailbox_dispatched` | Nó despertou, comando despachado |
| Sincronizador | `success` (child 254) | Comando de intervalo enviado |
| Sincronizador | `success` (child < 200) | Atuação confirmada, com estado LIGADO/DESLIGADO |
| Sincronizador | `error` | Timeout após 3 tentativas |
| Gateway | transição de estado | Gateway offline/online |

O texto do `node_lost` já é escrito em termos de **Intervalo + ΔT**, consumindo
`intervalMin`/`timeoutSec` do registro.

O alerta de intervalo só dispara quando o valor **muda** (`last_param_values`), então o
eco periódico do child 254 não vira spam.

---

## 6. Aba **Irrigação**

### 6.1 Canteiro A — timer fixo

Cron **07:00** e **17:00**. Liga o child 31 do Nó 99 por **300 s** (padrão), agenda o
desligamento por `setTimeout` e notifica início/fim no Telegram.
Manejo tradicional, sem consultar sensor. Parcela 6×1 m, 35 pés de alface.

### 6.2 Canteiro B — motor agroclimático

Avaliado a cada **5 min**. Integra Solo 3D (Nó 2), clima interno (Nó 99, DHT11) e
Open-Meteo. Cinco camadas em série; qualquer uma pode abortar a rega:

| Camada | Regra |
|---|---|
| **1 — Soak time** | Bloqueia por 15 min (20 min se T < 18 °C) após a última rega |
| **2 — Sanitização** | Só leituras dos últimos 30 min, faixa 0–1023 ADC; usa a **mediana** |
| **3.1 — Capacidade de campo** | Mediana < 350 ADC → solo úmido, standby |
| **3.2 — Chuva iminente** | Prob. > 70 % adia, **exceto** se mediana ≥ 700 ADC (estresse crítico) |
| **3.3 — Pico de radiação** | Radiação > 700 W/m² ou 12h–13h bloqueia, exceto se mediana ≥ 650 ADC |
| **3.4 — Anti-fúngico noturno** | 20h–05h com (T − ponto de orvalho) < 1,5 °C bloqueia, exceto se mediana ≥ 650 ADC |
| **4 — Balanço hídrico** | Lâmina FAO-56 com fator atmosférico |
| **5 — Corte mínimo** | Duração ≤ 180 s → não vale a pena, standby |

**Cálculo da lâmina (camada 4):**

```
lâmina_mm = ((mediana − 350) / 500) × 3,5 × F_atmo      teto 4,5 mm
F_atmo    = 1,0  +0,15 se VPD > 1,8 kPa
                 +0,10 se vento > 20 km/h
                 +0,15 se T > 25 °C e UR < 18 %
volume_L  = lâmina_mm × 6,0                   (canteiro de 6 m², 1 mm = 1 L/m²)
duração_s = volume_L / (2,5/60)               (gotejamento 2,5 L/min), cap 600 s
```

Comando final: child **32** do Nó 99, `payload "1"` → `setTimeout` → `"0"`.

### 6.3 Open-Meteo

Consulta a cada **30 min** para `-15,963944, -47,804028`. `Processar Variáveis
Agroclimáticas` extrai da hora corrente: ET₀ (FAO), VPD, radiação instantânea, vento,
ponto de orvalho, temperatura, umidade, temperatura e umidade do solo. Analisa
**janela de 4 h** para probabilidade máxima de chuva e precipitação acumulada, e
soma a ET₀ do dia. Resultado em `flow.agro_meteo`.

---

## 7. Aba **IA Repórter (Diário)**

Cron **08:00**. `Radar M360` agrega as últimas 24 h de `mqtt_logs` — irrigações,
volume, temperatura mín/máx/média, umidade, eventos `node_lost`/`node_reconnected`,
comandos enviados vs. confirmados — e compara com o snapshot D-1
(`m360_daily_metrics_prev`). Envia o contexto ao **Gemini 2.5 Flash**, salva as
métricas do dia como novo D-1 e distribui o texto por Telegram para o chat principal
**e todos os inscritos**.

---

## 8. Aba **Telegram Bot Interativo**

`Telegram Receiver` → `Tratar Comandos e Convites Telegram`:

| Comando | Efeito |
|---|---|
| `/start convite` | Inscreve o chat na lista de assinantes |
| `/start` | Boas-vindas e inscrição |
| `/tempo` | Condições atuais |
| `/previsao` | Previsão agroclimática |

Inscritos vivem em `global.telegram_subscribers` (`chatId`, `name`, `origin`,
`registeredAt`). Um cron às **06h, 11h, 16h e 21h** consulta a Open-Meteo e envia o
`Formatar Boletim Meteorológico 4x Dia`.

---

## 9. Contratos com o firmware

Alterar qualquer linha abaixo exige mudança **simultânea** nos dois lados.

| Contrato | Node-RED | Firmware |
|---|---|---|
| Formato do tópico nativo | `Pub MQTT (tópico dinâmico)` | `processMQTTCommandNative()`, `Translator::buildNativeTopic()` |
| `payload` sempre string | `String(m.payload)` | `MyMessage::getBool()` → `atoi()` |
| Child 254 = intervalo | `Processador do Comando UI`, `Mapeia nós` | `M360_CHILD_ID_INTERVAL`, `M360Node::_announceInterval()` |
| Child 255 = bateria | `Filtro Bateria SolarMini` | `M360_CHILD_ID_BATTERY` |
| Child 253 = debug | `getChildDesc()` | `M360Node::sendDebug()` |
| `ack == 1` = ACK de transporte | `Decodificador Nativo` | `publishTransportAck()` |
| Fórmula Intervalo + ΔT | `Watchdog da rede`, `Mapeia nós`, `Monitor de Falhas` | `NodeRegistry::registerInterval()` |
| Faixa de intervalo 1–1440 | widget `Intervalo (min)` | `M360_MIN_INTERVAL` / `M360_MAX_INTERVAL` |
| Comandos `V_CUSTOM` | `Processador do Comando UI` | `CMD_REPRESENT`, `CMD_FORCE_UPDATE`, `CMD_DEBUG_NET` |
| Sufixo `[LP]` no sketch name | detecção de nó LP na Caixa Postal | `M360Node::begin()` — sufixo por `M360PowerProfile` |
| Janela de despertar ~3 s | Caixa Postal despacha na telemetria | `M360_MIN_AWAKE_MS` + `smartSleep()` |
| Child IDs por nó | filtros e coletores | `inventario.md` |

---

## 10. Segurança

- **Credenciais do Telegram** vêm de `env.get()` do Node-RED, com fallback para
  `global`. Não há token no `flows.json`. Versões anteriores do arquivo tinham o
  token embutido — ele permanece no histórico do Git e **deve ser rotacionado**.
- **`GEMINI_API_KEY`** é lida de `global`; o literal no código é o placeholder
  `'COLE_AQUI'`.
- **`nodered/.env`** (usuário e senha do servidor) está no `.gitignore`. Nunca versionar.

---

## 11. Regra de manutenção (obrigatória)

> **Toda alteração no Node-RED exige atualizar este arquivo na mesma entrega.**

O fluxo de trabalho é o definido no `CLAUDE.md` / `AGENTS.md`:

1. A alteração é feita **no servidor de produção**, via ferramentas MCP Node-RED.
2. [`flows.json`](flows.json) é atualizado no Git como cópia fiel de produção.
3. **Este arquivo é atualizado para descrever a mudança** — mesma entrega, mesmo commit.

### Gatilhos que exigem atualizar este documento

| Mudança no Node-RED | O que revisar aqui |
|---|---|
| Criar, remover ou renomear uma aba | §1 |
| Criar ou remover variável global / de fluxo | §2 |
| Incluir, remover ou alterar um inject de comando | §3.1 |
| Alterar limiar do watchdog ou fórmula de timeout | §3.2 e §9 |
| Incluir ou alterar botão, dropdown ou ação administrativa | §4.3 |
| Alterar o caminho de despacho de um botão ou as cores de feedback | §4.1 |
| Alterar como o Sincronizador separa controle / telemetria / comando | §5.5 ("Discriminação") |
| Alterar assinatura MQTT | §5.1 |
| Mudar regra de decodificação, registro ou matching de ACK | §5.2 – §5.5 |
| Incluir, remover ou alterar filtro/gráfico | §5.7 |
| Alterar retenção, throttle ou export do log | §5.8 |
| Alterar cooldown ou texto de alerta | §5.9 |
| Alterar qualquer limiar de irrigação | §6 |
| Alterar prompt, cron ou métricas do IA Repórter | §7 |
| Incluir ou alterar comando do bot Telegram | §8 |
| Tocar em qualquer item de §9 | §9 **e** o firmware correspondente |
| Introduzir credencial ou segredo | §10 |

### Verificação antes de entregar

- Cada aba de `flows.json` tem seção correspondente aqui.
- Todo limiar numérico citado neste documento bate com o valor em `flows.json`.
- Nenhum contrato de §9 mudou de um lado só.
- Nenhum segredo foi introduzido no `flows.json`.

> Um documento defasado é pior que documento nenhum: as regras de irrigação e os
> critérios de ACK são invisíveis no JSON e **só existem aqui em forma legível**.
> Se divergirem, a próxima sessão de debug parte de premissa errada.

---

## Referências

- [`inventario.md`](../inventario.md) — nós, child IDs e contrato com estes fluxos
- [`CLAUDE.md`](../../../../CLAUDE.md) — regras de agente, armadilhas MySensors ↔ MQTT ↔ Node-RED
- [`lib/M360-DRY/`](../../../../lib/M360-DRY/) — firmware do gateway e dos nós
