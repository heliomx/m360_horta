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
| `mys_nodes` | global (**memória** — ver nota) | `Mapeia nós` | Registro completo de nós: childs, valores, cadência, status |
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
| `m360_irrig_deadlines` | global | Controladores A/B | **Prazo de desligamento por child** — lido pelo `Reconciliador de Irrigação` (§6.4) |
| `flow_vol_node_{nó}_{child}` | **global** | `Filtro Vazão e Volume` | Volume acumulado (L). Era `flow` e o `Radar M360` lia de outra aba — sempre `undefined` |
| `hist_solo_A`, `hist_solo_B`, `hist_clima`, … | flow | nós `Salvar Histórico *` | Séries de 7 dias para replay dos gráficos |

> ⚠️ **`mys_nodes` NÃO sobrevive a reinício do Node-RED.** O código faz
> `global.get('mys_nodes','file')` dentro de `try/catch`, mas o servidor está
> configurado com `contextStorage: {default: "memory", stores: ["memory"]}` —
> **não existe store `file`**. Toda chamada cai no `catch` e usa memória. O mesmo
> vale para `m360_irrig_deadlines` e para os históricos dos gráficos.
>
> Consequência prática: um restart do processo apaga o registro de nós, os prazos
> de irrigação em curso e as séries dos gráficos. Contra restart durante uma rega,
> a única proteção é o **failsafe do firmware** do Nó 99 (§9). Configurar um store
> `file` em `settings.js` fecharia essa lacuna de uma vez para as três coisas.

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
| Re-apresenta Nó 02 / 04 | child 0, `V_CUSTOM` (48) | `"REPRESENT"` |
| Lê Sensores Nó 02 / 04 / 99 | child 0, `V_CUSTOM` (48) | `"FORCE_UPDATE"` |

> **"Reboot Gateway" foi removido** em 26/08/2026. Não existe caminho MQTT que
> reinicie o gateway: `ESP.restart()` só é chamado pelo portal web
> (`M360Webserver.cpp:200`). O inject caía em `Pub MQTT Injects`, que **ignora o
> campo `action`**, e publicava `.../in/0/0/1/0/2` com payload **vazio** — um
> `C_SET`/`V_STATUS` sem sentido para o nó 0. Botão que prometia o que o firmware
> não implementa.
>
> Os cinco injects de REPRESENT/FORCE_UPDATE estavam com **`wires: [[]]`** — sem
> fio de saída, clicar não fazia nada, em silêncio. Religados a `Pub MQTT Injects`
> na mesma data.
>
> ⚠️ Estes injects de relé publicam **direto**, fora do `Sincronizador ACK`. Além de
> furar Caixa Postal e retentativa, um inject disparado enquanto o Sincronizador
> aguarda pode gerar um eco que **confirma falsamente** o comando pendente (mesmo
> nó, mesmo child, mesmo payload). É bancada — usar com o dashboard parado.

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
| `m360/+/+/out/#` | `Decodificador Nativo` + logger |
| `m360/+/+/out/events` | `Monitor de Falhas` — **não** vai ao logger |
| `m360/+/+/in/#` | logger (marca direção OUT) |
| `m360/+/+/gateway/status` | `Gateway Status + Watchdog` + logger |

> **Por que `out/events` não alimenta o logger (corrigido em 28/08/2026).**
> `m360/+/+/out/#` **também** casa `m360/+/+/out/events` — as duas assinaturas se
> sobrepõem. Com as duas ligadas ao `Tag MQTT IN`, todo evento do gateway entrava
> duas vezes em `global.mqtt_logs`. O sintoma era `node_lost`, `node_reconnected` e
> `child_presentation` repetidos no log com o mesmo `timestamp` interno, e a
> auditoria diária da aba IA Repórter contando `nodeLostEvents` em dobro.
>
> Não era duplicação do firmware: `gateway/status`, publicado pelo mesmo gateway a
> cada 60 s e casado por **uma** assinatura só, aparece uma única vez no log — e
> `publishTransportEvent()` tem ponto de chamada único por evento.
> O `Decodificador Nativo` continua recebendo `/events` pelo `out/#` e o descarta
> sozinho (payload de evento não tem `command`), então o log via `out/#` basta.
>
> Ao mexer nas assinaturas, lembre que `#` casa o nível pai: qualquer nova
> assinatura sob `.../out/` vai colidir com `out/#` do mesmo jeito.

### 5.2 `Decodificador Nativo MySensors`

Normaliza os dois formatos num objeto único:

```js
{ nodeId, sensorId, command, ack, type, payload, direction, timestamp }
```

`direction` vale `'sensor'` ou **`'transport_ack'`** quando `ack === 1`.

Distribui para **cinco** destinos: `Sincronizador ACK`, `Separar ACK / Leituras`,
`Mapeia nós`, `Monitor de Falhas` e `Telemetria decodificada (Link Out)` — este
último alimenta o `Coletor Telemetria Canteiro B` na aba Irrigação (§6.5).

> Essa rotulagem é deliberada. `ack === 1` **nunca vem de um nó**: é o ACK que o
> gateway fabrica em `publishTransportAck()` assim que o rádio confirma o salto.
> Com o Nó 99 como repetidor, pode vir do repetidor. Confundi-lo com confirmação do
> nó dava "sucesso" com o nó desligado.

### 5.3 `Translator Json` — **removido em 26/08/2026**

Traduzia a mensagem para texto legível com tabelas `C_*`/`S_*`/`V_*`/`I_*` e
diagnóstico agronômico (`MANEJO360`). Foi **excluído**, não apenas desconectado.

Por quê:

- estava com **`wires: [[]]`** — nenhum destino — e era o **primeiro** fio do
  `mqtt in`, então executava a cada mensagem MQTT, antes do decodificador, fazia
  um `global.get('mys_nodes')` e montava uma string grande para jogar fora;
- por ser o primeiro fio, recebia o payload **cru**, não o objeto decodificado:
  `m.command` era `undefined` de qualquer jeito;
- seu `avaliarManejo()` julgava umidade de solo na escala **0–100**
  (`<60` seco, `>90` excesso de água) aplicada a valores **ADC 0–1023 em que alto
  = seco** (§9). O diagnóstico saía **invertido**: 640 ADC — solo seco — era
  rotulado *"⚠ Possível excesso de água"*.

Religá-lo sem corrigir a escala reintroduz o diagnóstico invertido. Se voltar a
fazer falta, o lugar dele é **depois** do `Decodificador Nativo`, e a tabela
`MANEJO360` precisa de uma entrada própria para `V_LEVEL` em ADC.

### 5.4 `Mapeia nós` — o registro

Constrói `mys_nodes` **exclusivamente** a partir do que o nó apresenta. Sem rótulo
hardcoded por `nodeId`, sem dado de demonstração.

Por nó guarda: `childs` (tipo + rótulo vindos de `present()`), `values`,
`sketchName`/`sketchVersion` (de `I_SKETCH_NAME`/`I_SKETCH_VERSION`), `batteryLevel`,
`batteryVoltage`, `intervalMin`, `timeoutSec`, `cycleMs`, `lastSeen`, `lastHeartbeat`,
`lastAck`, `categoria`.

Comportamentos:
- **Cadência observada** — média móvel `0,7·anterior + 0,3·gap` para gaps > 15 s, em `cycleMs`;
- **Child 254** — grava `intervalMin` (só o valor; o timeout é calculado em ponto único);
- **`timeoutSec`** — ponto único, ao fim da função:
  `base = max(intervalMin × 60, cycleMs / 1000)` e `timeoutSec = base + max(120 s, 50 % de base)`;
- **Apresentação de nó** (`command 0`, `sensorId 255`, tipo `S_ARDUINO_NODE` 17 ou
  `S_ARDUINO_REPEATER_NODE` 18) e **`"REPRESENT"`** zeram `childs` e marcam
  `prunePending`;
- **Poda de valores órfãos** — na primeira telemetria (`command 1`) após a rajada de
  apresentação, remove de `values` as chaves sem child declarado, exceto 253/254/255;
- **`ack == 1` é ignorado** ao gravar `values[]` — senão o dashboard mostraria estado de relé que nó nenhum confirmou;
- **TTL** — nó sem contato há 48 h sai do registro;
- **Categoria derivada** dos tipos de child: Solo, Clima, Atuação, Reservatório ou Gateway.

> **Por que `timeoutSec` usa o maior dos dois (corrigido em 28/08/2026).**
> Antes o intervalo declarado no child 254 tinha precedência absoluta, e a cadência
> observada só era usada quando o nó nunca havia anunciado intervalo. Isso quebra em
> nó `M360_ALWAYS_ON`: `M360Node::_readAndSendAll()` só transmite um sensor quando o
> valor muda **ou a cada 10 ciclos** (`staleForced = _nNoUpdates[i] >= 10`,
> `M360Node.cpp`). Com `intervalMin = 1`, o Nó 99 pode ficar ~11 min em silêncio
> perfeitamente legítimo — e o `timeoutSec` de 180 s o marcava OFFLINE em toda
> janela sem mudança de leitura, com alerta de Telegram junto.
>
> Nó `M360_LOW_POWER` não tem esse problema: o `smartSleep()` emite
> `I_PRE_SLEEP_NOTIFICATION` (32) e `I_POST_SLEEP_NOTIFICATION` (33) a cada ciclo,
> então há tráfego garantido por intervalo e os dois valores coincidem — o `max()`
> não muda nada para eles (Nó 4: `intervalMin` 27 → `timeoutSec` 2430 s, igual a antes).
>
> Consequência: `timeoutSec` passa a se autoajustar à cadência real. Um nó que
> começa a falar mais devagar alarga a própria janela — o que **atrasa** a detecção
> de queda real. O piso de ΔT (120 s) e o TTL de 48 h continuam sendo o limite.
>
> **O gateway aplica a mesma fórmula** desde 28/08/2026 (`NodeRegistry::recalcTimeout()`).
> Enquanto ele derivava o limiar só do intervalo declarado, o Nó 99 ficava com 180 s
> contra uma cadência real de 206–430 s: gravar o gateway com o timeout dinâmico
> *apertou* uma janela que antes era um valor fixo de 900 s, acidentalmente seguro.
> São dois relógios independentes sobre os mesmos dados — se divergirem, o nó
> aparece ONLINE de um lado e perdido do outro.
>
> O firmware descarta duas classes de amostra que o `Mapeia nós` também descarta:
> gaps abaixo de 15 s (rajada de apresentação, eco de atuador) e — só no firmware —
> o gap medido a partir de um nó **inativo**, que é tempo de queda e não ritmo de
> reporte. Sem essa segunda guarda, um nó que volta de horas fora esticaria a
> própria janela na proporção do tempo em que esteve ausente. O teto de 2 h
> (`MAX_TIMEOUT_MS`) fecha o caso patológico em que a cadência cresce sozinha.

> **Por que a tabela de childs é zerada na apresentação (corrigido em 28/08/2026).**
> O comentário do código prometia esse reset desde sempre, mas ele nunca existiu:
> o registro só acrescentava childs. Resultado — o Nó 99 acumulou **28** childs, os
> canônicos (11, 12, 21, 31–39) convivendo com a numeração de firmwares antigos
> (0–6 e 16–23), e o Nó 2 ficou com um child 0 duplicando o child 1.
>
> O reset é feito **apenas** na apresentação de nó tipo 17/18, que é a primeira
> mensagem do boot, ~2 s antes das apresentações de child. Não ampliar a condição
> para "qualquer `sensorId 255` com `command 0`": o nó também envia tipo 30 (versão
> da lib) **depois** dos childs, e zerar ali apagaria o que acabou de ser apresentado.
>
> **`values` não é zerado junto**, e isso é deliberado: atuador não responde `C_REQ`
> (`M360Node::handleMessage()` só atende `kind == M360_SENSOR`), então o estado dos
> 9 relés do Nó 99 não voltaria sozinho — o dashboard ficaria sem indicação de
> solenóide até a próxima rega. Por isso a poda é adiada para a primeira telemetria
> depois da rajada, quando `childs` já está completo.
>
> Nó offline não se cura sozinho: a limpeza acontece no boot seguinte dele. Para
> forçar agora, mande **Re-apresentar** pelo dashboard — o ACK de transporte do
> gateway carrega o payload `REPRESENT` de volta pelo `out/`, e é ele que dispara
> o reset aqui.

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

**Carimbo de `command` / `type` nos status** — toda mensagem da saída 2
(`success`, `error`, `mailbox_*`) carrega `command` e `type` do comando que a
originou. Na fila de ACK são sempre `1`/`2` (só entra comando de relé); no bypass
são os valores reais. É o que permite ao `Monitor de Falhas` separar atuação de
comando administrativo sem adivinhar pela faixa do `sensorId` (§5.9).

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

### 5.6 Watchdog do gateway e mapa de rede

> **Critério único de "nó OFFLINE".** O `Processador do Mapa` usava
> `3 × cycleMs` limitado a `[300, 21600]`, enquanto o `Watchdog da rede` (aba MQTT)
> usava `timeoutSec` (Intervalo + ΔT, §9). O mesmo nó podia aparecer **ONLINE** no
> mapa e **OFFLINE** na tabela. Desde 26/08/2026 ambos usam a fórmula do §9.
>
> **Status do gateway no mapa.** Era o literal `'ONLINE'`: o mapa mostrava o
> gateway no ar enquanto o `Verifica Gateway Offline` — mesma aba, mesmo escopo de
> flow — disparava alerta de OFFLINE no Telegram. Agora lê `flow.gateway_online`
> (e mostra `DESCONHECIDO` antes do primeiro contato).

#### Nós do watchdog

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

> **Todo filtro começa com `if (Number(m.command) !== 1 || Number(m.ack) === 1) return null;`**
> (exceção: `Filtro Bateria SolarMini`, que trata os dois caminhos de bateria —
> `command 3`/`I_BATTERY_LEVEL` e `command 1`/`V_VOLTAGE` — e por isso só descarta
> `ack === 1`).
>
> A saída `direction === 'sensor'` do switch **inclui apresentação (`command 0`) e
> mensagens internas (`command 3`)**. Sem essa guarda:
> - numa apresentação, o payload é o **rótulo** do child — `parseFloat("B_1m_10cm")`
>   = `NaN` — e ia direto para o gráfico e para o histórico. **Todo REPRESENT
>   envenenava as séries de solo.**
> - `command 3` + `type 0` é `I_BATTERY_LEVEL`, que casava com o teste de
>   temperatura do `Filtro Temp/Hum Ar`: o nível de bateria era plotado como °C.
>
> Na mesma passagem caíram duas condições mortas: `child.tipo === 'V_LEVEL'`
> (`tipo` guarda sempre um `S_*`) e `m.type === 35`, que é `V_VOLUME` na tabela
> `V_*` — era o número de `S_MOISTURE` (tabela `S_*`) usado numa comparação de
> tipo de variável. O teste correto de solo é `type === 37` (`V_LEVEL`) ou
> `type === 3` (`V_PERCENTAGE`), ou `child.tipo === 'S_MOISTURE'`.

> **`Filtro Vazão e Volume`** — a mensagem de volume passou a carregar `ts`. Sem
> ele, `Salvar Histórico Volume` avaliava `(now - undefined) < CUTOFF` → `NaN` →
> `false` e **descartava toda entrada, inclusive a recém-inserida**: `hist_volume`
> ficava eternamente vazio enquanto o acumulador marcava centenas de litros. Os
> sete `Salvar Histórico *` agora também aplicam `ts = msg.ts || Date.now()`.
>
> O acumulador migrou de `flow` para **`global`** (`flow_vol_node_{nó}_{child}`):
> o `Radar M360` da aba IA lia a chave com `flow.get()` de **outra aba**, onde ela
> nunca existiu — o volume saía sempre `0` no relatório diário.
>
> ⚠️ O acumulador **nunca zera**: "Consumo de Água (Litros)" é total vitalício, não
> diário. Um reset periódico continua pendente.

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

> **Cada publicação é registrada uma vez só (corrigido em 28/08/2026).**
> `Pub MQTT ACK out` alimentava o `mqtt out` **e** o `Tag MQTT OUT`. A mesma
> publicação virava duas linhas: uma na saída da função e outra ~200 ms depois,
> quando o comando voltava do broker pela assinatura `in/#`. Parecia comando
> duplicado no log — e não era. Os outros três publicadores (`Solenóides`,
> `Bombas`, `Injects`) nunca fizeram isso: dependem só da assinatura, que é a
> fonte mais fiel porque registra o que de fato chegou ao broker.
> Ao criar um novo publicador, **não** ligue a função ao `Tag MQTT OUT`.

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
| Eventos | `node_lost` | Nó inativo, com motivo e timeout aplicado |
| | | O motivo cita o `timeoutSec` que o `Mapeia nós` aplicou, mais o intervalo declarado **e** a cadência observada. Antes recalculava a janela a partir de `intervalMin` sozinho, o que passou a contradizer o total desde a mudança de §5.4 (`> 620 s (Intervalo: 1 min + ΔT: 2 min)`) |
| Eventos | `node_reconnected` | Comunicação restabelecida |
| Eventos | `node_sketch_name` | Nó identificado na rede |
| Telemetria | child 254, `command 1`, `ack != 1` | **Parâmetro confirmado: intervalo aplicado na EEPROM do nó** |
| Sincronizador | `mailbox_enqueued` | Comando enfileirado — nó em hibernação |
| Sincronizador | `mailbox_dispatched` | Nó despertou, comando despachado |
| Sincronizador | `success` (child 254) | Comando de intervalo enviado |
| Sincronizador | `success` + `command 1` + `type 2` + `0 < child < 200` | Atuação confirmada, com estado LIGADO/DESLIGADO |
| Sincronizador | `success` **não**-atuação | Comando administrativo despachado (diz explicitamente que **não** aguarda ACK) |
| Sincronizador | `error` | Timeout após 3 tentativas |
| Gateway | transição de estado | Gateway offline/online |

O texto do `node_lost` já é escrito em termos de **Intervalo + ΔT**, consumindo
`intervalMin`/`timeoutSec` do registro.

O alerta de intervalo só dispara quando o valor **muda** (`last_param_values`), então o
eco periódico do child 254 não vira spam.

> **O teste `isAtuacao` (`command === 1 && type === 2`) não é redundante.** A
> condição anterior era só `cId < 200`, e o painel administrativo manda
> `sensorId: 0`. Resultado: apertar **REPRESENT** disparava
> *"🎯 COMANDO EXECUTADO: Atuação Confirmada — Atuador: Child 0 — Estado:
> **DESLIGADO (0)** — ✅ ACK recebido com sucesso"*. Falso nos três pontos: não é
> atuação; o valor era a string `"REPRESENT"` (que `String(msg.value) === '1'`
> avalia como falso, virando "DESLIGADO"); e esse comando sai pelo **bypass**,
> sem ACK nenhum. Corrigido em 26/08/2026.
>
> Para isso o `Sincronizador ACK` passou a **carimbar `command` e `type`** em todas
> as mensagens de status que emite (§5.5). Quem consumir esses status daqui em
> diante deve discriminar por esses campos, nunca pela faixa do `sensorId`.

---

## 6. Aba **Irrigação**

### 6.0 Caminho do comando — **mudou em 26/08/2026**

```
Controlador A / Motor B / Reconciliador
   └── envelope {nodeId, sensorId, command, type, payload}
         └── link out → Sincronizador ACK → mqtt out
```

Antes, os dois controladores publicavam **direto** num `mqtt out` com o tópico
literal `m360/DF/0000/in/99/3X/1/0/2`. Três defeitos de uma vez:

1. **Sem retentativa.** Um OFF perdido no rádio deixava a válvula aberta, sem
   erro, sem alerta, sem reenvio. Agora o Sincronizador confirma pelo eco de
   aplicação do nó e **reenvia até 3×** (§5.5).
2. **Prefixo `m360/DF/0000` hardcoded** em 2 function nodes, 2 `mqtt out` e 1
   `mqtt in`, enquanto todo o resto usa `global.mqtt_topic_prefix`. Trocar UF/CAR
   no gateway fazia a irrigação comandar o tópico errado **em silêncio**, com o
   dashboard continuando a funcionar. Os nós `Publicar Comando Solenóide A/B`
   foram **removidos**.
3. **Sem dono do desligamento** — ver §6.4.

> Efeito colateral desejável: como o comando agora passa pelo Sincronizador, os
> botões da aba Solenóides passam a refletir o estado da irrigação automática
> (o `buttonId` vem nulo e o feedback casa pelo `sensorId`).

### 6.1 Canteiro A — timer fixo

Cron **07:00** e **17:00**. Liga o child 31 do Nó 99 por **300 s** (padrão), registra
o prazo em `m360_irrig_deadlines`, agenda o desligamento por `setTimeout` e notifica
início/fim no Telegram.
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

### 6.4 `Reconciliador de Irrigação` — rede de segurança do OFF

Tick de **30 s**. Existe porque o desligamento era agendado por `setTimeout()`
**dentro do function node que ligou a válvula**: qualquer redeploy do Node-RED
destrói esse timer e a válvula fica aberta indefinidamente. Este nó não depende de
nenhum timer em memória do nó de origem.

| Camada | Gatilho | Ação |
|---|---|---|
| 1 — Prazo vencido | `m360_irrig_deadlines[child].until` já passou | envia OFF pelo Sincronizador e avisa no Telegram (`FINALIZADA … fechada pelo Reconciliador`) |
| 2 — Válvula órfã | `mys_nodes[99].values[child] === '1'` por **> 11 min** sem prazo dono | envia OFF, `node.error()` e alerta `RECONCILIADA` |

Detalhes que não são arbitrários:

- **O limiar da camada 2 (660 s) é deliberadamente maior que o failsafe de 600 s
  do firmware** (`maxOnSecondsFor()` em `99nodeReles.cpp`). Com o firmware
  gravado, o nó desliga sozinho primeiro e este ramo **nunca dispara**. Ele só age
  quando o failsafe do nó não está presente ou não funcionou.
- **Bombas NFT (38/39) são ignoradas** — regime contínuo por projeto, exatamente
  como o firmware define (`return 0` = sem limite).
- O `setTimeout` dos controladores **checa o registro antes de mandar o OFF**: se o
  Reconciliador já fechou, o timer não manda um OFF duplicado.

> ⚠️ **O que isto NÃO cobre.** O registro de prazos vive em contexto `global` de
> **memória** (§2) — sobrevive a *deploy*, **não** a restart do processo. E
> `C_REQ` só responde para `M360_SENSOR` (`M360Node.cpp:157`), então **não há como
> perguntar ao nó o estado atual de um relé** para ressincronizar depois de um
> restart. Contra restart durante uma rega, a única proteção é o failsafe do
> firmware. Fechar essa lacuna exige um store `file` em `settings.js` **e** suporte
> a `C_REQ` em atuadores no firmware.

### 6.5 `Coletor Telemetria Canteiro B`

Consome o objeto **já decodificado** pelo `Decodificador Nativo` via link in
(§5.2). Antes esta aba tinha o próprio `mqtt in` (com prefixo literal) e o próprio
parser, que exigia `parts.length >= 9` — ou seja, **só entendia o tópico nativo**.

> Em modo JSON o coletor ficava mudo, `solo_b_readings` nunca enchia, o Motor de
> Regras respondia *"Sem leituras válidas no Solo B"* e o **Canteiro B nunca
> irrigava** — sem erro, sem log, sem alerta. Era também um segundo parser para o
> mesmo dado, que podia divergir do principal.

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
| Fórmula do timeout: `base = max(intervalo declarado, cadência observada)`, `timeout = base + max(2 min, 50 %)` | `Watchdog da rede`, `Mapeia nós` (`cycleMs`), `Monitor de Falhas` | `NodeRegistry::recalcTimeout()` — média móvel 0,7/0,3 em `NodeStatus.cycleMs`, alimentada por `update()` |
| Silêncio legítimo de nó `ALWAYS_ON` | `max(intervalMin, cycleMs)` no `Mapeia nós` (§5.4) | `staleForced = _nNoUpdates[i] >= 10` em `M360Node::_readAndSendAll()` — mexer no `10` muda o pior caso de silêncio |
| `rssi` no evento do gateway | `Monitor de Falhas` rotula "Wi-Fi do gateway" | `publishTransportEvent()` passa **`WiFi.RSSI()` do gateway** — é o enlace Wi-Fi do ESP8266, **não** o do rádio RF24 do nó. Até 28/08/2026 o alerta dizia "Último RSSI"/"Sinal RSSI", o que se lia como qualidade do enlace do nó e sustentou um diagnóstico inteiro de "link budget saudável". O nRF24L01+ não tem RSSI (só RPD de 1 bit); RSSI de rádio exigiria `MY_SIGNAL_REPORT_ENABLED` |
| Faixa de intervalo 1–1440 | widget `Intervalo (min)` | `M360_MIN_INTERVAL` / `M360_MAX_INTERVAL` |
| Comandos `V_CUSTOM` | `Processador do Comando UI` | `CMD_REPRESENT`, `CMD_FORCE_UPDATE`, `CMD_DEBUG_NET` |
| Sufixo `[LP]` no sketch name | detecção de nó LP na Caixa Postal | `M360Node::begin()` — sufixo por `M360PowerProfile` |
| Janela de despertar ~3 s | Caixa Postal despacha na telemetria | `M360_MIN_AWAKE_MS` + `smartSleep()` |
| Child IDs por nó | filtros e coletores | `inventario.md` |
| **Teto de tempo ligado dos solenóides** | `BACKSTOP_MS` = 660 s no `Reconciliador de Irrigação` | `maxOnSecondsFor()` = 600 s em `99nodeReles.cpp` |
| **Bombas NFT sem teto** | Reconciliador ignora childs 38/39 | `maxOnSecondsFor()` → `return 0` |
| Escala de umidade de solo | filtros, `Coletor Telemetria B`, Motor de Regras | `readNodeItem()` devolve **ADC bruto 0–1023**, alto = seco |

> O contrato do teto tem uma **ordem** embutida: o número do Node-RED tem que ser
> **maior** que o do firmware. Inverter isso faz o Node-RED fechar a válvula antes
> do nó e mascarar um failsafe de firmware que parou de funcionar.

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
| Alterar o caminho de comando da irrigação ou o Reconciliador | §6.0, §6.4 **e** §9 |
| Alterar guarda de `command`/`ack` em qualquer filtro de gráfico | §5.7 |
| Alterar escopo de contexto de qualquer chave | §2 (e conferir quem lê de outra aba) |
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
