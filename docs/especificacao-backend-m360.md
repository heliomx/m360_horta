# Especificação do Backend M360 Horta (substituto do Node-RED)

**Versão:** 1.3 — 2026-10-17 (1.1: seção 15, provisionamento; 1.2: D-09 com grade de 64 canais; 1.3: reed das placas M360 — eventos `factory_reset`, `REJOIN`, leitura forçada; fonte de clima = Nó 4; RF-PRV-09 conforme o firmware)
**Status:** proposta para aprovação do dono do projeto
**Escopo:** backend que assume todas as funções hoje executadas pelo Node-RED
(`src/DRY/horta/nodered/flows.json`), com o Node-RED desativado ao final da migração.

> Este documento é **normativo**: diz o que o backend tem de fazer e como será
> aceito. O guia [`Backend Definitivo M360.md`](Backend%20Definitivo%20M360.md)
> continua sendo a referência de **lições e armadilhas** do backend temporário.
> Onde os dois divergirem sobre um número, vale a tabela de parâmetros da §9
> deste documento; onde divergirem sobre child IDs, vale sempre o
> [`inventario.md`](../src/DRY/horta/inventario.md).

**Fontes de verdade que este documento não duplica:**

| Assunto | Documento |
|---|---|
| Child IDs, labels, `S_*`/`V_*`, escalas, failsafe do Nó 99 | [`src/DRY/horta/inventario.md`](../src/DRY/horta/inventario.md) §7–§9 |
| Armadilhas de integração MySensors ↔ MQTT | [`CLAUDE.md`](../CLAUDE.md), seção "Armadilhas Críticas" |
| Contrato MQTT do gateway (código) | [`lib/M360-DRY/src/M360Translator.cpp`](../lib/M360-DRY/src/M360Translator.cpp), [`M360MQTT.cpp`](../lib/M360-DRY/src/M360MQTT.cpp) |
| Motor agroclimático do Canteiro B (fundamento agronômico) | [`motor_agroclimatico_manejo360.md`](motor_agroclimatico_manejo360.md) |
| Implementação de referência a ser substituída | [`flows.json`](../src/DRY/horta/nodered/flows.json) (7 abas, 51 function nodes; commit `9c8b768` de 2026-09-24, alinhado ao servidor nessa data) |

---

## 1. Objetivo

Substituir o Node-RED por um serviço de produção que:

1. Ingira toda a telemetria MQTT do gateway e a **persista** em banco de dados.
2. Mantenha o registro de nós e children **dirigido pela apresentação** dos próprios nós.
3. Despache comandos aos atuadores com **confirmação pelo eco de aplicação**, retentativa, fila por nó e caixa postal para nós de baixo consumo.
4. Execute a irrigação dos canteiros A e B com **agendamento durável**, que sobrevive a reinício do processo.
5. Vigie gateway e nós e **notifique** pelo Telegram, com bot interativo, boletim meteorológico e relatório diário.
6. Exponha **API e interface web** que substituam o dashboard do Node-RED.
7. Seja **testável sem a estufa ligada**, contra um broker local e um simulador de nó.

Fora do escopo: firmware dos nós e do gateway, placas, Node-RED do Kit Hélio
(sub-projeto separado, mesmo contrato MQTT, pode usar a mesma instância do backend
como segunda instalação).

---

## 2. Contexto e restrições

### 2.1 Topologia

```
Nós MySensors (nRF24 ou RFM95W)              Backend (este documento)
  Nó 1  Solo A   ALWAYS_ON                      ┌────────────────────────┐
  Nó 2  Solo B   ALWAYS_ON      Gateway ESP8266 │ ingestor · registro    │
  Nó 4  SolarMini LOW_POWER  ──►  (Nó 0)  ──►   │ despachante · regras   │──► Telegram
  Nó 99 Atuação  ALWAYS_ON/REP   WiFi → MQTT    │ agendador · API · UI   │──► Open-Meteo, LLM
                                 broker         └────────────────────────┘
```

- O backend **só fala MQTT** com a estufa. Não há caminho alternativo ao gateway.
- **Um gateway por propriedade** (decisão de 2026-09-10). O prefixo de tópico
  `m360/{UF}/{CAR}` identifica a instalação. O backend trata cada prefixo como
  uma **instalação** distinta e nunca hardcoda `m360/DF/0000`.
- Broker atual: `mqtt.viridiotech.com.br:1883` (sem TLS), usuário e senha. Node-RED
  atual em `nr.viridiotech.com.br`. O backend será implantado no mesmo servidor
  (ver §11).

### 2.2 Contrato MQTT que o backend consome

O gateway compila com `M360_NATIVE_MQTT=1` (modo ativo). O backend **deve**
suportar o nativo e **deve** aceitar o envelope JSON (modo alternativo), como o
Decodificador do Node-RED já faz.

| Tópico | Segmentos | Payload | Conteúdo |
|---|---:|---|---|
| `m360/{UF}/{CAR}/out/{node}/{child}/{cmd}/{ack}/{type}` | 9 | valor bruto | Telemetria, apresentação, internas, ecos de atuação |
| `m360/{UF}/{CAR}/out` | 4 | JSON | Heartbeat do gateway (a cada 60 s) ou envelope JSON no modo alternativo |
| `m360/{UF}/{CAR}/out/events` | 5 | JSON | Eventos de transporte e de validação de comando |
| `m360/{UF}/{CAR}/gateway/status` | 5 | JSON | Métricas MQTT do gateway (`type: "mqtt_metrics"`) |
| `m360/{UF}/{CAR}/in/#` | — | — | Comandos de **outros** emissores (auditoria durante a convivência com o Node-RED) |

Campos relevantes já verificados no código:

- Heartbeat: `nodeId:0, command:3, type:22, event:"heartbeat", uptime, rssi, wifiRssi, ip, version, source:"gateway"`. `timestamp` e `uptime` são **segundos desde o boot**, não epoch.
- Status: `isConnected, connectionTime, reconnectCount, lastErrorCode, lastError, uptime, source, type:"mqtt_metrics"`.
- Evento: `event, nodeId, timestamp (uptime), rssi, details?`. Catálogo: `gateway_presented, node_discovered, node_reconnected, node_lost, node_presentation, child_presentation, node_sketch_name, node_sketch_version, node_registration_request, node_config_request, node_heartbeat, command_rejected, command_send_failed, factory_reset`. O `factory_reset` sai **antes** de o gateway apagar a configuração e reiniciar em AP: é a última mensagem do prefixo antes de o gateway sumir (RF-PRV-09).
- Debug remoto dos nós: `out/{node}/253/1/0/47` (`V_TEXT`), payload texto ≤ 24 caracteres. Hoje: resposta do `DEBUG_NET` (`P:<pai> D:<dist> R:<S/N>`), `REJOIN` e `REJOIN:OK` (repareamento por reed, RF-ING-10).
- `ack=1` em qualquer mensagem de saída é o **ACK de transporte fabricado pelo gateway**, nunca o nó.

### 2.3 Contrato MQTT que o backend produz

```
tópico:  m360/{UF}/{CAR}/in/{node}/{child}/{cmd}/0/{type}
payload: valor bruto (string), ≤ 25 bytes
```

Validação espelhada do gateway (`Translator::validate()`): exatamente 8 barras,
segmentos numéricos, `cmd ∈ {1,2,3}`, `node ∈ 1..255`, payload ≤ 25 bytes e, para
`C_SET`/`V_STATUS`, payload **exatamente** `"0"` ou `"1"`. O backend valida
**antes** de publicar; o gateway valida de novo e publica `command_rejected`.

### 2.4 Restrições do firmware que moldam o backend

| Fato | Consequência para o backend |
|---|---|
| `getBool()` faz `atoi()`: qualquer payload que não seja `"1"` **desliga** o relé | Serialização tipada; teste automatizado de todos os comandos de atuação |
| Atuadores não são reportados por intervalo; só eco de comando, eco de preempção e failsafe | Estado de relé é persistido e reconciliado; `C_REQ` a atuador **não responde** |
| MUX do Nó 99: children 31–37 são **exclusão mútua**; o firmware ecoa `V_STATUS=0` do preemptado antes do eco do comandado | Grupo de concorrência serializado; eco de preempção é estado real |
| Failsafe no nó: 600 s solenóides (31–33), 120 s peristálticas (34–37), sem limite bombas (38–39) | Nenhuma ação pode durar ≥ limite; `V_STATUS=0` não solicitado é verdade |
| Nós `LOW_POWER` só recebem na janela de despertar (`smartSleep`) | Caixa postal por nó; comando a LP é "pendente até despertar", nunca interativo |
| `M360Node` só reenvia sensor sem variação a cada 10 ciclos | Timeout de inatividade de nó `[ON]/[REP]` é 10× o intervalo declarado |
| M360-ACT (`act_99`) **não tem DHT**: children 11/12 só existem no Nano | Fonte de clima do motor do Canteiro B é **configurável** (§7.3); em produção, desde 2026-10-17, é o **Nó 4** (children 11/12, `LOW_POWER`, 30 min) |
| `forceUpdate()` (comando `FORCE_UPDATE`, reed da LP, repareamento da ACT) reenvia só **sensores** e bateria; atuadores nunca | Nenhuma leitura forçada produz eco de relé; RF-CMD-11 não é disparado por ela |
| Reed da M360-LP (ímã pela parede) força leitura e envio de todos os children; em `LOW_POWER` acorda o nó fora do ciclo | Mensagem fora de hora é normal: libera a caixa postal (RF-CMD-09) e não pode distorcer a cadência (RF-REG-08) |
| Reed da M360-ACT mantido 3 s: repareamento sem reboot (`REJOIN` → busca de pai → reapresentação dos children → `REJOIN:OK`) | Reapresentação **sem** a apresentação do nó (child 255): não zera a tabela (RF-REG-02); relés não são reenviados |
| Reed/botão do gateway mantido 3 s nos 30 s iniciais do `loop()`: `factory_reset` e configuração apagada | O prefixo some; o próximo tráfego do gateway virá de outro prefixo ou de nenhum (RF-PRV-09) |
| Relógio do gateway é uptime | Toda leitura é carimbada com o horário de recepção do backend, fuso `America/Sao_Paulo` |

---

## 3. Requisitos funcionais

Numeração: `RF-<área>-<nn>`. "Deve" é obrigatório; "pode" é opcional.

### 3.1 Ingestão (ING)

- **RF-ING-01** Assinar, por instalação configurada, `{prefix}/out/#`, `{prefix}/gateway/status` e `{prefix}/in/#`, com QoS 1. Discriminar o tipo de mensagem **pelo número de segmentos do tópico**, nunca pelo conteúdo.
- **RF-ING-02** Decodificar o formato nativo (9 segmentos) e o envelope JSON (4 segmentos com `nodeId` e `command`). Rejeitar e registrar em métrica qualquer tópico com segmento não numérico.
- **RF-ING-03** Normalizar toda mensagem para o registro interno `{instalacao, received_at, nodeId, childId, command, ack, type, payload, direction}` com `direction ∈ {sensor, transport_ack}` (`transport_ack` ⇔ `ack = 1`).
- **RF-ING-04** Persistir **toda** mensagem decodificada em `message_log` (tópico, direção IN/OUT, payload bruto, horário), inclusive comandos publicados pelo próprio backend e por terceiros em `{prefix}/in/#`. Retenção configurável (padrão 30 dias).
- **RF-ING-05** Persistir como **leitura** (`reading`) apenas mensagens com `command = 1` e `ack = 0` cujo payload seja numérico. Apresentação (`command 0`) e internas (`command 3`) nunca viram leitura — no Node-RED, um `REPRESENT` colocava o rótulo `"B_1m_10cm"` como `NaN` nos gráficos.
- **RF-ING-06** Gravar `value_raw` (o que veio do rádio) e `value_si` (convertido pela escala da §9.2) com a unidade. Umidade de solo fica em ADC bruto 0–1023 (alto = seco) em `value_raw`; a conversão para percentual é derivada do tipo de apresentação `S_MOISTURE`, nunca de lista fixa de nós.
- **RF-ING-07** Consumir `{prefix}/out/events` e persistir cada evento em `gateway_event`. Hoje o Node-RED descarta todos eles.
- **RF-ING-08** Consumir heartbeat e `gateway/status`; atualizar `gateway.last_seen`, `uptime`, `rssi`, `ip`, `version`, métricas MQTT. Detectar **reboot do gateway** (uptime menor que o anterior) e registrar evento interno `gateway_rebooted`.
- **RF-ING-09** Operar com várias instalações (`UF/CAR`) na mesma instância, isolando registro, comandos, regras e notificações por instalação. Tráfego de um prefixo **não configurado** não é descartado: cria a instalação em estado `PENDENTE` (RF-PRV-03).
- **RF-ING-10** Debug remoto (child 253, `V_TEXT`): persistir cada texto como evento de nó (`gateway_event` com `event = node_debug`, `node_id`, `details` = texto) — nunca como leitura. `REJOIN` gera o evento interno `node_rejoin_started`; `REJOIN:OK`, `node_rejoined` (com a duração desde o `REJOIN`). Resposta de `DEBUG_NET` é correlacionada ao comando `debug_net` em voo (RF-CMD-07) e exibida no detalhe do comando.

### 3.2 Registro de nós e children (REG)

Substitui `mys_nodes`. Dirigido pela apresentação do nó, sem lista estática.

- **RF-REG-01** Criar o nó na primeira mensagem recebida (`status = NEW`). Manter `first_seen, last_seen, last_heartbeat, last_transport_ack, message_count`.
- **RF-REG-02** Apresentação de nó (`command 0, childId 255, type 17 ou 18`) **zera a tabela de children** do nó e marca poda pendente de valores órfãos. Não zerar em `type 30` (versão da lib), que chega depois dos children.
- **RF-REG-03** Apresentação de child (`command 0, childId ≠ 255`) grava `presentation_type` (`S_*`) e `label` (payload). Child que envia leitura sem ter sido apresentado recebe placeholder `S_CUSTOM` / `"Sensor <id>"`.
- **RF-REG-04** `V_CUSTOM` com payload `REPRESENT` observado em qualquer direção também zera a tabela de children (fantasmas de renumeração).
- **RF-REG-05** Poda de valores órfãos: na primeira leitura após a rajada de apresentação, remover `last_value` de children que não constam da nova tabela, preservando 253/254/255. Nunca apagar o estado dos atuadores junto com a tabela (não há como reconsultá-los).
- **RF-REG-06** Internas (`command 3`): `type 0` → `battery_level` (%); `type 11` → `sketch_name`; `type 12` → `sketch_version`; `type 22` → `last_heartbeat`; `type 32/33` → marca de ciclo de sono (`LP`). Leitura em child 255 → `battery_voltage`; em child 254 → `interval_min` (confirmação de intervalo).
- **RF-REG-07** Perfil de energia derivado do sufixo do `sketch_name`: `[LP]`, `[ON]`, `[PAS]`, `[REP]`. Fallback por `cycle_ms` observado > 15 s ⇒ `LP`.
- **RF-REG-08** Cadência observada `cycle_ms` por média móvel exponencial (0,7 anterior / 0,3 novo) de intervalos entre mensagens maiores que 15 s. Com `interval_min` conhecido, intervalos menores que metade dele **não** entram na média: são acordadas fora do ciclo (reed da M360-LP, `FORCE_UPDATE`) e encurtariam o timeout do nó.
- **RF-REG-09** Timeout de inatividade por nó, em ponto único: `base = interval_min × 60`; se perfil `[ON]`/`[REP]`, `base ×= 10`; `base = max(base, cycle_ms/1000)`; `timeout_s = base + max(120, 0,5 × base)`; padrão 450 s sem dados. Espelha `NodeRegistry::recalcTimeout()` do gateway.
- **RF-REG-10** Status derivado: `SEM DADOS` (nunca visto), `OFFLINE` (idade > `timeout_s`), `SEM HEARTBEAT` (> 900 s sem `type 22`, só para nós que já enviaram heartbeat), `SEM ACK` (> 1800 s sem `ack=1`), senão `ONLINE`. Um só critério para API, UI e alertas — no Node-RED a tabela de watchdog e o mapa divergiam.
- **RF-REG-11** TTL: nó sem contato há mais de 48 h (configurável) sai da lista ativa (`status = EXPIRED`), sem apagar histórico. Varredura a cada 300 s.
- **RF-REG-12** Categoria derivada dos tipos de children (solo, clima, atuação, água, gateway), para a UI.
- **RF-REG-13** Nós dos **dois** sub-projetos (Horta e Kit Hélio) aparecem separados por instalação.

### 3.3 Comandos (CMD)

Substitui `Sincronizador ACK`, `Despachante`, `Pub MQTT` e `Caixa Postal`.

- **RF-CMD-01** Todo comando nasce de um registro `command` com estado inicial `PENDENTE` e passa pela máquina de estados da §5.3. Nenhum `publish` para `{prefix}/in/...` fora do despachante.
- **RF-CMD-02** Tipos de comando aceitos pela API: `atuar(node, child, 0|1)`, `ler(node, child)` (`C_REQ`), `represent(node)`, `force_update(node)`, `debug_net(node)` (todos `C_SET`/`V_CUSTOM` em child 0), `set_interval(node, 1..1440)` (`C_SET`/`V_VAR1` em child 254). Qualquer outro é rejeitado na API.
- **RF-CMD-03** Validação antes de publicar (§2.3). `atuar` só aceita child cujo `presentation_type` seja de atuador (`S_BINARY`, `S_SPRINKLER` etc.) **ou** que esteja na lista configurada de atuadores da instalação; comando a child desconhecido é rejeitado com motivo.
- **RF-CMD-04** Confirmação de `atuar` **só** pelo eco de aplicação: mensagem com `nodeId` e `childId` iguais, `command = 1`, `type = 2`, `ack = 0` e `payload` igual ao comandado. ACK de transporte (`ack = 1`) nunca confirma; é só registrado em `command.transport_ack_at`.
- **RF-CMD-05** Retentativa de `atuar`: timeout 5 s, backoff ×2 (5, 10, 20 s), 3 tentativas; depois `FALHOU`. Parâmetros configuráveis por instalação.
- **RF-CMD-06** `command_rejected` recebido em `out/events` dentro da janela do comando → estado `REJEITADO` com `details`; `command_send_failed` → conta como tentativa falha e dispara a próxima.
- **RF-CMD-07** Comandos administrativos (`represent`, `force_update`, `debug_net`, `ler`, `set_interval`) não esperam eco: estado `ENVIADO` ao publicar. Para `set_interval`, a leitura subsequente de child 254 é correlacionada ao comando (`CONFIRMADO` se igual ao pedido, `DIVERGENTE` se o nó respondeu outro valor — o nó sempre responde o vigente).
- **RF-CMD-08** **Fila por nó**, FIFO. Dentro do Nó 99, os children 31–37 formam um **grupo de exclusão mútua**: nunca há dois comandos em voo no grupo, e um `atuar(…,1)` para um canal do grupo marca os demais como `OFF` ao receber o eco de preempção. Children 38 e 39 são independentes entre si e do grupo.
- **RF-CMD-09** **Caixa postal (LP):** comando para nó de perfil `LP` que não transmitiu nos últimos 2 s fica em `AGUARDANDO_DESPERTAR`. Qualquer mensagem do nó libera a fila dele imediatamente (janela de despertar). A caixa é **FIFO por nó** — hoje o Node-RED guarda um só comando por nó e sobrescreve o anterior sem aviso. Expiração configurável (padrão 24 h) com estado `EXPIRADO`. Na M360-LP o instalador pode **forçar a janela** aproximando o ímã do reed: o nó acorda, transmite e a fila é liberada (útil no comissionamento, C12).
- **RF-CMD-10** Comando originado por regra traz `origin = rule:<id>`; por usuário, `origin = user:<id>`; por reconciliador, `origin = reconciler`. Toda transição de estado é persistida com horário.
- **RF-CMD-11** Eco de aplicação **não solicitado** (sem comando em voo para o child) atualiza o estado do atuador e gera evento interno `actuator_unsolicited_off` quando o payload é `0` — é o failsafe ou a preempção falando. Se havia uma `irrigation_run` ativa para o child, ela é encerrada com `end_reason = failsafe_or_preempt`.
- **RF-CMD-12** Reconciliação de estado dos atuadores no startup: carregar o último estado persistido, marcar como `incerto` se o gateway ou o Nó 99 rebootaram depois dele (uptime menor), e aguardar ecos. Opcionalmente, `OFF` explícito em todos os atuadores de grupo no boot, atrás de flag de configuração (padrão desligado, porque interrompe rega em curso).
- **RF-CMD-13** Chave operacional `comandos.confirmacao_habilitada` (equivalente ao `ENABLE/DISABLE_ACK_SYNC`): com ela desligada o despachante publica e marca `ENVIADO` sem esperar eco. Uso só em diagnóstico; o estado fica visível na UI e em `/api/health`.

### 3.4 Irrigação (IRR)

Substitui a aba `Irrigação` e o `Reconciliador`.

- **RF-IRR-01** Toda rega é um registro `irrigation_run` com `bed, child, started_at, planned_seconds, deadline_at, mode, rationale, state, ended_at, end_reason, volume_l`. O **OFF é uma `scheduled_action` persistida** com `run_at = deadline_at`, executada pelo agendador (§5.4). Nunca um temporizador em memória.
- **RF-IRR-02** `planned_seconds` é limitado a `failsafe_s(child) − margem` (margem padrão 15 s). Pedido acima do limite é truncado e registrado em `rationale`. O limite por child vem da configuração (§9.1) e reflete o firmware.
- **RF-IRR-03** **Canteiro A — controle tradicional:** cron `07:00` e `17:00` (`America/Sao_Paulo`), 300 s, child 31, modo `Timer Fixo (Manejo Tradicional)`. Não usa sensores. É a parcela de controle do experimento: não acrescentar inteligência.
- **RF-IRR-04** **Canteiro B — motor agroclimático:** avaliação a cada 300 s, child 32, pipeline de cinco camadas da §7.3 com os parâmetros da §9.3. Cada avaliação persiste um registro `rule_evaluation` com entradas, camada que decidiu e resultado — hoje isso só existe como `node.status` no editor do Node-RED.
- **RF-IRR-05** **Soak time:** nenhuma nova rega do Canteiro B antes de 15 min (`temp ≥ 18 °C`) ou 20 min (`temp < 18 °C`) após o início da última; `last_irrigation_at` é persistido por canteiro.
- **RF-IRR-06** Início da rega = comando `atuar(…,1)` via despachante. Se o comando **falhar** (§3.3), a `irrigation_run` termina com `end_reason = command_failed`, a `scheduled_action` de OFF é cancelada e o Telegram é avisado. Hoje o Node-RED segue contando o tempo de uma válvula que nunca abriu.
- **RF-IRR-07** **Reconciliador** a cada 30 s: (a) fecha runs com `deadline_at` vencido cujo OFF não executou (rede de segurança do agendador); (b) **backstop**: solenóide 31–33 com estado `ON` há mais de 660 s sem run ativa recebe OFF e alerta `RECONCILIADA` com texto de que o failsafe do firmware não atuou. Bombas 38/39 ficam fora do backstop por projeto.
- **RF-IRR-08** Rega manual pela API/UI (`POST /api/irrigation/beds/{bed}/run` com duração) segue o mesmo caminho: run, OFF agendado, soak time, Telegram. Duração sujeita a RF-IRR-02.
- **RF-IRR-09** Habilitar/desabilitar cada regra (A, B) por configuração e pela API, sem redeploy. Durante a migração (§10) a regra de um lado tem de estar desligada antes de ligar do outro.
- **RF-IRR-10** O volume consumido por run é atribuído integrando a vazão (RF-VOL-01) do child de vazão associado ao canteiro enquanto a run está ativa.

### 3.5 Volume e contadores (VOL)

- **RF-VOL-01** Para todo child de vazão (`type 34 V_FLOW` ou apresentado como `S_WATER`), integrar `dV = (vazao_L_min / 60) × dt_s`, com `0 < dt ≤ 300 s`; `dt` maior indica lacuna e não é extrapolado. Contador `counter(node, child, 'volume_l')` persistido a cada atualização.
- **RF-VOL-02** Série de vazão em L/s e série de volume acumulado disponíveis para gráficos e relatórios, por qualquer nó com hidrômetro, não só o 99.
- **RF-VOL-03** Totais diários e por run calculados do banco (`SUM` por janela), nunca de acumulador em RAM.

### 3.6 Vigilância (WDG)

- **RF-WDG-01** Gateway `OFFLINE` quando `now − last_seen > 120 s` (heartbeat ou `gateway/status`), avaliado a cada 60 s. Transições geram eventos internos `gateway_offline` / `gateway_online` com `last_contact`.
- **RF-WDG-02** Status de nós recalculado a cada 60 s pelo critério único de RF-REG-10; transição para `OFFLINE` ou de volta gera evento interno. Os eventos `node_lost`/`node_reconnected` do **gateway** são persistidos e também alimentam o alerta, com deduplicação por estado (um alerta por transição).
- **RF-WDG-03** Quando o gateway está `OFFLINE`, a UI e a API marcam **todos** os dados como "sem contato" em vez de exibir valores velhos como atuais.

### 3.7 Notificações e Telegram (NOT)

Substitui `Monitor de Falhas`, `Formatador Telegram`, `Telegram Bot Interativo`, boletim e relatório. Toda saída passa por uma interface `Notifier` com implementação Telegram; segredos via configuração (§5.6).

- **RF-NOT-01** Lista de assinantes persistida (`telegram_subscriber`: chat_id, nome, origem, registrado_em). Chat principal configurado é sempre assinante.
- **RF-NOT-02** Comandos do bot: `/start` (registra), `/start convite` (registra com origem `convite`), `/id`, `/status` (clima da `climate_source` — Nó 4 em produção —, vazão e alimentação do Nó 99, status dos nós), `/tempo` ou `/previsao` (último boletim agroclimático), `/ajuda`. Desconhecido → resposta padrão.
- **RF-NOT-03** Alertas de rede com cooldown: `node_lost` 5 min por nó e só na transição; `node_reconnected` 2 min e só se houve `lost` antes; `node_sketch_name` 30 s; `gateway_offline` 5 min; `gateway_online` sempre na transição; `factory_reset` sempre, sem cooldown, e suprime o `gateway_offline` que vem em seguida (o gateway saiu do prefixo de propósito); `node_rejoined` 30 s por nó, com a duração do repareamento; `node_rejoin_started` sem `node_rejoined` em 5 min → alerta de repareamento sem conclusão. Texto inclui descrição do nó (`sketch_name vVersão`), motivo com o `timeout_s` realmente aplicado, RSSI e horário.
- **RF-NOT-04** Alertas de comando: `AGUARDANDO_DESPERTAR` e despacho da caixa postal; `CONFIRMADO` de atuação (cooldown 3 s por node/child) com rótulo do child; comando administrativo despachado (texto distinto: "não aguarda ACK"); `set_interval` confirmado pela leitura do child 254 (só quando o valor muda); `FALHOU` (cooldown 60 s por node/child).
- **RF-NOT-05** Irrigação: mensagem em `INICIADA`, `FINALIZADA` e `RECONCILIADA`, com canteiro, duração formatada, modo e, para o Canteiro B, os parâmetros agroclimáticos da avaliação. Horário no fuso `America/Sao_Paulo`.
- **RF-NOT-06** **Boletim meteorológico** às 06:00, 11:00, 16:00 e 21:00 com dados da Open-Meteo (§6.4): temperatura, orvalho, umidade, VPD, radiação, ET₀, vento, chuva nas próximas 6 h e recomendação (chuva ≥ 60 % e ≥ 2 mm → reduzir rega; VPD > 1,8 → estresse hídrico; VPD < 0,4 e umidade > 85 % → ventilar; senão condições favoráveis). Enviado a todos os assinantes.
- **RF-NOT-07** **Relatório diário às 08:00**: métricas das últimas 24 h calculadas **do banco** (volume irrigado, nº de regas, temperatura mín/méd/máx, umidade média, eventos `node_lost`/`reconnected`, comandos enviados/confirmados/falhos, nós e status) mais o snapshot D-1 persistido, enviados a um provedor de LLM atrás de interface (implementação inicial Gemini `gemini-2.5-flash`, prompt "Radar M360" da aba `IA Repórter`). Se o LLM falhar, enviar o resumo numérico sem narrativa. Nunca enviar logs brutos acima do limite configurado (70 000 caracteres).
- **RF-NOT-08** Toda notificação enviada é registrada (`notification`: tipo, destino, texto, resultado). Falha de envio não derruba o fluxo que a originou.

### 3.8 Interface web e API (UI/API)

Substitui o dashboard do Node-RED (grupos Controle Manual, Gráficos, Log MQTT, Mapa de Rede, Watchdog). Interface **servida pelo próprio backend**, autenticada, com atualização em tempo real por SSE.

- **RF-UI-01** **Controle manual:** botões ligar/desligar para cada atuador do Nó 99 (31–33, 38–39 obrigatórios; 34–37 atrás de confirmação, por serem dosagem). Cor por estado do comando: pendente (âmbar), confirmado (verde, mantido enquanto o atuador estiver `ON`), erro (vermelho por 2 s), caixa postal (índigo). O estado inicial vem do estado persistido do atuador.
- **RF-UI-02** **Painel administrativo:** seletor de nó (lista dinâmica do registro), ação (`REPRESENT`, `FORCE_UPDATE`, `DEBUG_NET`, `SET_INTERVAL`), campo de intervalo 1..1440 validado no cliente e no servidor, botão enviar, e botão "reapresentar toda a rede".
- **RF-UI-03** **Gráficos** (janela padrão 7 dias, zoom livre, dados do banco): umidade de solo Canteiro A (Nó 1, 6 séries por rótulo do child), Canteiro B (Nó 2), clima do Nó 4 (temperatura do ar, umidade, temperatura do solo — é a fonte de clima em produção), clima do Nó 99 (só existe no Nano), bateria (tensão e nível por nó), vazão (L/s) e volume acumulado. Séries nomeadas pelo `label` apresentado pelo nó.
- **RF-UI-04** **Mapa da rede:** gateway (status, uptime, IP, RSSI, versão) e cards por nó com categoria, status, último contato (absoluto e relativo), bateria, firmware e lista de children com último valor formatado por tipo (`S_BINARY` → LIGADO/DESLIGADO, unidades da §9.2).
- **RF-UI-05** **Tabela de vigilância:** por nó, status, idade do último contato, do heartbeat e do ACK, `timeout_s` aplicado.
- **RF-UI-06** **Log MQTT:** últimas N mensagens (padrão 100) com data, direção, tópico, payload; filtro por nó/tópico; exportação CSV do período (`Timestamp, Direction, Topic, Payload`, aspas duplas escapadas); botão limpar **não** apaga o banco, só a visão.
- **RF-UI-07** **Irrigação:** runs em andamento com contagem regressiva e botão de parada; histórico de runs com volume; última avaliação do motor B com a camada que decidiu; parâmetros das regras editáveis com validação (§9.3) e trilha de quem alterou.
- **RF-UI-08** **Comandos:** lista com estado, tentativas, origem e horários; detalhe com o eco que confirmou.
- **RF-API-01** API REST com JSON, autenticada por token, documentada por OpenAPI. Endpoints mínimos na §6.2.
- **RF-API-02** Fluxo de eventos em tempo real (`GET /api/stream`, SSE) com leituras, estados de comando, estados de atuadores, status de nós e gateway.
- **RF-API-03** `GET /api/health` com broker conectado, última mensagem recebida, agendador vivo, banco acessível, chave de confirmação (RF-CMD-13), versão.

---

## 4. Requisitos não funcionais

Cada item responde a uma falha observada no Node-RED (ver guia §8).

| ID | Requisito | Critério verificável |
|---|---|---|
| **RNF-01 Durabilidade** | Toda ação futura é linha persistida; o processo pode ser morto a qualquer instante sem perder nem duplicar uma ação | Teste: iniciar rega de 60 s, matar o processo em 20 s, subir em 70 s → OFF enviado uma vez, run `FINALIZADA` com `end_reason = late_by_reconciler` |
| **RNF-02 Persistência** | Leituras, registro, comandos, runs, contadores, soak time, assinantes e snapshot D-1 em banco relacional | Reinício não altera volume acumulado nem `last_irrigation_at` |
| **RNF-03 Idempotência** | Reexecutar uma `scheduled_action` já executada não publica de novo | Chave única por ação; estado `EXECUTADA` checado antes de publicar |
| **RNF-04 Configuração** | Prefixos, broker, segredos, parâmetros de regra e limites de failsafe fora do código; falta de segredo obrigatório impede o start | Startup com `TELEGRAM_BOT_TOKEN` ausente falha com mensagem clara (hoje degrada em silêncio) |
| **RNF-05 Observabilidade** | Log estruturado (JSON) com `command_id` do pedido ao desfecho; métricas (comandos por estado, mensagens por tópico, latência eco, filas) expostas em `/metrics` | Um comando falho é rastreável em ≤ 3 consultas |
| **RNF-06 Testabilidade** | Testes rodam contra broker local (Mosquitto em container) e **simulador de nó** que reproduz eco, preempção do MUX, failsafe e sono LP | Pipeline de CI sem hardware cobre §3.3, §3.4 e §3.5 |
| **RNF-07 Fuso e relógio** | Horários persistidos em UTC, exibidos e agendados em `America/Sao_Paulo`; `timestamp` do gateway nunca usado como relógio | Cron 07:00 dispara às 07:00 de Brasília em host UTC |
| **RNF-08 Segurança** | UI e API autenticadas; segredos só em variáveis de ambiente ou arquivo com permissão restrita; broker com usuário/senha e TLS quando disponível; nenhuma credencial em repositório | Varredura do repo sem token; `/api/*` sem token → 401 |
| **RNF-09 Desempenho** | Carga atual: < 1 msg/s, ~1 M leituras/ano por instalação. Latência do caminho comando → publish < 100 ms; API de séries de 7 dias < 1 s | Medido no simulador com 5 nós |
| **RNF-10 Disponibilidade** | Reconexão automática ao broker com backoff; subscrição restabelecida; sessão MQTT persistente (`clean_session = false`) para não perder ecos durante reconexão | Derrubar o broker por 30 s não perde confirmação de comando em voo |
| **RNF-11 Convivência** | Pode rodar em paralelo com o Node-RED só consumindo, sem publicar, até a fase de corte | Flag `modo_somente_leitura` |
| **RNF-12 Retenção** | `message_log` 30 dias; `reading` sem limite inicial (particionada por mês); `rule_evaluation` 90 dias | Job diário de expurgo |

---

## 5. Arquitetura

### 5.1 Componentes

```
                 ┌─────────────┐   eventos de domínio   ┌──────────────────┐
 MQTT ──────────►│  Ingestor   │──────────────────────►│  Registro de nós │
 (aiomqtt/paho) │ decodifica, │                        └──────────────────┘
                 │ persiste    │──► reading / message_log / gateway_event
                 └──────┬──────┘
                        │ ecos, eventos
          ┌─────────────▼──────────────┐      ┌───────────────┐
          │  Despachante de comandos   │◄─────┤ Motor de      │ cron A, avaliador B,
          │ filas por nó, grupo MUX,   │      │ regras        │ soak time, Open-Meteo
          │ caixa postal, retry, eco   │      └───────┬───────┘
          └─────────────┬──────────────┘              │ runs, OFF agendado
                        │ publish {prefix}/in/...     ▼
                        │                    ┌─────────────────┐
                        │                    │ Agendador       │ scheduled_action,
                        │                    │ durável         │ reconciliador 30 s,
                        │                    └─────────────────┘ watchdogs, TTL, boletins
          ┌─────────────▼──────────────┐
          │  API REST + SSE + UI web   │──► Notifier (Telegram) · Weather (Open-Meteo) · LLM (Gemini)
          └────────────────────────────┘
                        │
                   PostgreSQL
```

Um processo único (asyncio) é suficiente para a carga; os componentes são
módulos com fronteiras claras, para poderem virar processos separados depois.
O **agendador** e o **despachante** são os únicos que publicam no broker.

### 5.2 Modelo de domínio (esquema relacional)

Chaves estrangeiras omitidas por brevidade. Todos os horários em `timestamptz`.

```sql
installation      (id PK, uf, car, mqtt_prefix UNIQUE, name, timezone, enabled,
                   state PENDENTE|COMISSIONANDO|ATIVA|SUSPENSA, lat, lon,
                   mqtt_user, radio jsonb, created_at, activated_at)
commissioning_check (installation_id, check_id, PK(...), state, evidence jsonb, ts)

gateway           (installation_id PK, last_seen, last_heartbeat, uptime_s, rssi, ip,
                   version, mqtt_connected, reconnect_count, last_error, status)

node              (installation_id, node_id, PK(installation_id,node_id),
                   first_seen, last_seen, last_heartbeat, last_transport_ack,
                   message_count, sketch_name, sketch_version, present_name,
                   power_profile, battery_level, battery_voltage, interval_min,
                   cycle_ms, timeout_s, status, category, prune_pending bool)

child             (installation_id, node_id, child_id, PK(...),
                   presentation_type, label, last_value, last_value_at,
                   is_actuator bool, mux_group text NULL, failsafe_s int NULL)

reading           (ts, installation_id, node_id, child_id, v_type, value_raw numeric,
                   value_si numeric, unit text)             -- particionada por mês
message_log       (ts, installation_id, direction IN|OUT, topic, payload text)
gateway_event     (ts, installation_id, event, node_id, details, rssi, raw jsonb)

command           (id PK, installation_id, created_at, origin, node_id, child_id,
                   command, v_type, payload, kind, state, attempts, next_attempt_at,
                   sent_at, transport_ack_at, confirmed_at, failed_at,
                   reject_reason, confirming_message_ts)
command_transition(command_id, ts, from_state, to_state, note)

actuator_state    (installation_id, node_id, child_id, PK(...), state 0|1,
                   since, source command|echo|preempt|failsafe|unknown, certain bool)

scheduled_action  (id PK, installation_id, run_at, kind, payload jsonb, state,
                   executed_at, result, dedupe_key UNIQUE)

irrigation_bed    (id PK, installation_id, name, node_id, child_id, flow_child_id,
                   area_m2, rule_kind fixed|agro, enabled, params jsonb)
irrigation_run    (id PK, bed_id, started_at, planned_s, deadline_at, mode,
                   rationale jsonb, state, ended_at, end_reason, volume_l,
                   on_command_id, off_command_id, off_action_id)
rule_evaluation   (ts, bed_id, inputs jsonb, decided_layer, decision, planned_s)

counter           (installation_id, node_id, child_id, kind, PK(...), value, updated_at,
                   last_sample_at)
weather_snapshot  (ts, installation_id, source, data jsonb)   -- Open-Meteo processado
daily_metrics     (installation_id, date, PK(...), data jsonb)  -- snapshot D-1 do relatório

telegram_subscriber (installation_id, chat_id, PK(...), name, origin, registered_at)
notification      (ts, installation_id, kind, target, text, ok bool, error)
user_account      (id PK, login, password_hash, role, created_at)
api_token         (id PK, user_id, token_hash, label, created_at, last_used_at)
```

`value_raw` e `value_si` convivem para permitir reinterpretar o histórico se uma
escala for corrigida. `child.mux_group` e `child.failsafe_s` são preenchidos pela
configuração da instalação (§9.1), não pela apresentação, porque o firmware não
os anuncia.

### 5.3 Máquina de estados do comando

```
PENDENTE ─validação falhou─► REJEITADO
PENDENTE ─nó LP dormindo──► AGUARDANDO_DESPERTAR ─msg do nó─► PENDENTE
PENDENTE ─publish ok──────► ENVIADO
ENVIADO  ─eco de aplicação (RF-CMD-04)──────────► CONFIRMADO
ENVIADO  ─command_rejected (events)─────────────► REJEITADO
ENVIADO  ─timeout ou command_send_failed, tentativas < 3─► PENDENTE (next_attempt_at)
ENVIADO  ─timeout, tentativas = 3───────────────► FALHOU
AGUARDANDO_DESPERTAR ─expiração (24 h)─────────► EXPIRADO
qualquer ─cancelamento pelo usuário/regra───────► CANCELADO
```

Administrativos (`kind ≠ atuar`) terminam em `ENVIADO`; `set_interval` pode ir a
`CONFIRMADO` ou `DIVERGENTE` pela leitura do child 254.

### 5.4 Agendador durável

- Loop com período de 1 s que seleciona `scheduled_action WHERE state='AGENDADA' AND run_at <= now()` com `FOR UPDATE SKIP LOCKED`, executa e marca `EXECUTADA` ou `FALHOU` na mesma transação lógica.
- Tipos: `irrigation_off`, `cron_fire(rule)`, `evaluate_bed(B)`, `reconcile`, `watchdog`, `ttl_sweep`, `weather_refresh`, `bulletin`, `daily_report`, `purge`.
- Ações recorrentes são reagendadas pelo próprio executor ao concluir (próxima ocorrência do cron no fuso da instalação). Na subida do processo, ações vencidas são executadas imediatamente (um OFF atrasado é melhor que nenhum) e registradas como `late`.
- `dedupe_key` (ex.: `irrigation_off:<run_id>`) impede duplicata por corrida entre agendador e reconciliador.

### 5.5 Eventos de domínio (internos)

`reading_received, node_presented, child_presented, node_status_changed,
gateway_status_changed, gateway_rebooted, command_state_changed,
actuator_state_changed, actuator_unsolicited_off, irrigation_started,
irrigation_ended, rule_evaluated, weather_updated, node_rejoin_started,
node_rejoined, gateway_factory_reset`. Consumidores: Notifier,
SSE, métricas, regras. Barramento em memória (asyncio) é suficiente; a
persistência de cada evento já está nas tabelas de origem.

### 5.6 Configuração

Arquivo YAML versionado **sem segredos** mais variáveis de ambiente para segredos.
Validado no startup (schema); erro de validação impede o start.

```yaml
installations:
  - id: horta-df-0000
    uf: DF
    car: "0000"
    timezone: America/Sao_Paulo
    location: { lat: -15.963944, lon: -47.804028 }
    actuators:                     # §9.1 — espelha o firmware do Nó 99
      - { node: 99, child: 31, label: Sol.CanteiroA, mux_group: mux99, failsafe_s: 600 }
      - { node: 99, child: 32, label: Sol.CanteiroB, mux_group: mux99, failsafe_s: 600 }
      - { node: 99, child: 33, label: Sol.CanteiroC, mux_group: mux99, failsafe_s: 600 }
      - { node: 99, child: 34, label: Perist.SuplA,  mux_group: mux99, failsafe_s: 120, confirm_ui: true }
      - { node: 99, child: 35, label: Perist.SuplB,  mux_group: mux99, failsafe_s: 120, confirm_ui: true }
      - { node: 99, child: 36, label: Perist.pH+,    mux_group: mux99, failsafe_s: 120, confirm_ui: true }
      - { node: 99, child: 37, label: Perist.pH-,    mux_group: mux99, failsafe_s: 120, confirm_ui: true }
      - { node: 99, child: 38, label: BombaNFT,      failsafe_s: null }
      - { node: 99, child: 39, label: BombaOxi,      failsafe_s: null }
    beds:
      - { id: A, node: 99, child: 31, flow_child: 21, rule: fixed, cron: ["0 7 * * *", "0 17 * * *"], duration_s: 300 }
      - { id: B, node: 99, child: 32, flow_child: 21, rule: agro, evaluate_every_s: 300,
          soil_source: { node: 2, children: [1,2,3,4,5,6] },
          climate_source: { node: 4, temp_child: 11, hum_child: 12, max_age_s: 3600, fallback: open_meteo },  # ≥ 2× o intervalo do Nó 4
          params: { ...§9.3... } }
    commands: { ack_timeout_s: 5, max_attempts: 3, backoff_factor: 2, mailbox_ttl_h: 24, confirmation_enabled: true }
    watchdog: { gateway_offline_s: 120, node_ttl_h: 48, heartbeat_stale_s: 900, ack_stale_s: 1800 }
    reconciler: { period_s: 30, backstop_s: 660, backstop_children: [31, 32, 33] }
    notifications: { bulletin_cron: "0 6,11,16,21 * * *", daily_report_cron: "0 8 * * *" }
mqtt: { host: mqtt.viridiotech.com.br, port: 1883, tls: false, client_id: m360-backend, qos: 1 }
retention: { message_log_days: 30, rule_evaluation_days: 90 }
read_only: false
```

Variáveis de ambiente obrigatórias: `M360_DB_URL, M360_MQTT_USER, M360_MQTT_PASSWORD,
TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, M360_UI_ADMIN_PASSWORD`. Opcional:
`GEMINI_API_KEY` (sem ela, o relatório diário sai sem narrativa).

---

## 6. Interfaces externas

### 6.1 MQTT

Consumo e produção conforme §2.2 e §2.3. Cliente com sessão persistente,
reconexão com backoff (1 s → 60 s), `client_id` fixo por instância. Durante o
modo `read_only`, nenhuma publicação.

### 6.2 API REST (mínimo)

| Método e rota | Função |
|---|---|
| `GET /api/health`, `GET /metrics` | Saúde e métricas |
| `GET /api/installations`, `POST /api/installations`, `PUT /api/installations/{id}` | Instalações (inclusive as `PENDENTE` descobertas pelo tráfego) |
| `GET /api/installations/{id}/commissioning`, `POST .../commissioning/{check}/run` | Checklist de comissionamento (§15.4) |
| `GET /api/nodes`, `GET /api/nodes/{id}` | Registro com status, children e últimos valores |
| `GET /api/readings?node&child&from&to&step` | Séries (agregação por `step` no servidor) |
| `GET /api/gateway` | Status, heartbeat, métricas MQTT |
| `GET /api/events?from&to&event` | Eventos do gateway |
| `POST /api/commands` | `{kind, node, child?, value?}` → `command` criado |
| `GET /api/commands?state&node`, `GET /api/commands/{id}` | Ciclo de vida |
| `POST /api/commands/{id}/cancel` | Cancela pendente ou na caixa postal |
| `GET /api/actuators` | Estado persistido, certeza e origem |
| `GET /api/irrigation/beds`, `PUT /api/irrigation/beds/{id}/params` | Regras e parâmetros |
| `POST /api/irrigation/beds/{id}/run` `{duration_s}` | Rega manual |
| `POST /api/irrigation/runs/{id}/stop` | Parada antecipada |
| `GET /api/irrigation/runs`, `GET /api/irrigation/evaluations` | Histórico |
| `GET /api/counters` | Volume acumulado por child |
| `GET /api/log/mqtt?from&to&node`, `GET /api/log/mqtt/export.csv` | Log e exportação |
| `GET /api/weather/latest` | Último snapshot agroclimático |
| `GET /api/telegram/subscribers` | Assinantes |
| `GET /api/stream` (SSE) | Eventos em tempo real |
| `POST /api/auth/login` | Sessão da UI |

Erros em JSON `{error, detail}`; validação de comando devolve `422` com o motivo
textual que também irá para `command.reject_reason`.

### 6.3 Telegram

Bot API (`getUpdates` por long polling ou webhook, configurável). Mensagens em
Markdown. Envio com retentativa simples (3×) e registro em `notification`.

### 6.4 Open-Meteo

`GET https://api.open-meteo.com/v1/forecast` com `latitude`, `longitude` da
instalação, `hourly=precipitation_probability,precipitation,temperature_2m,
relative_humidity_2m,dew_point_2m,et0_fao_evapotranspiration,vapour_pressure_deficit,
shortwave_radiation_instant,wind_speed_10m,soil_temperature_0cm,soil_moisture_0_to_1cm`,
`forecast_days=2`, `timezone=America/Sao_Paulo`. Atualização a cada 30 min. O
snapshot processado contém: hora corrente (índice mais próximo), `prob_chuva_max` e
`precip_total` nas próximas 4 h (motor) e 6 h (boletim), `et0_atual`,
`et0_acumulada_dia`, `vpd`, `radiacao`, `vento`, `ponto_orvalho`, `temp_ext`,
`hum_ext`, `solo_temp_modelo`, `solo_umid_modelo`, `updated_at`. Snapshot com mais
de 90 min é considerado **indisponível** pelo motor (não há default silencioso).

### 6.5 LLM (relatório diário)

Interface `ReportWriter.write(metrics_24h, metrics_d1, context) -> str`.
Implementação Gemini (`generateContent`, modelo configurável, padrão
`gemini-2.5-flash`). Timeout 60 s; falha → fallback numérico (RF-NOT-07).

---

## 7. Regras de negócio

### 7.1 Confirmação de comando de atuação

Ver RF-CMD-04. Pseudocódigo normativo:

```
confirma(msg, cmd) :=
    msg.installation == cmd.installation
and msg.nodeId == cmd.node and msg.childId == cmd.child
and msg.command == 1 and msg.type == 2 and msg.ack == 0
and str(msg.payload) == str(cmd.payload)
```

### 7.2 Canteiro A

Cron 07:00 e 17:00, 300 s, child 31. Sequência: criar run → `atuar(99,31,1)` →
agendar `irrigation_off` em `now + 300 s` → Telegram `INICIADA`. No OFF:
`atuar(99,31,0)` → run `FINALIZADA` → Telegram `FINALIZADA` com volume medido.

### 7.3 Canteiro B — pipeline de cinco camadas (a cada 300 s)

Entradas: leituras de solo do Nó 2 (children 1–6, `V_LEVEL` ADC bruto), clima da
`climate_source` (temp/umidade com idade ≤ `max_age_s`; fallback `temp_ext`/`hum_ext`
da Open-Meteo quando configurado; **sem clima válido, as camadas que dependem dele
são puladas e isso fica registrado**, em vez do default 20 °C/20 % do Node-RED),
snapshot agroclimático (§6.4), hora local.

1. **Soak time.** Se `now − last_irrigation_at < soak(temp)` → `STANDBY_SOAK`. `soak = 20 min` se `temp < 18 °C`, senão `15 min`.
2. **Sanitização.** Leituras de solo com idade ≤ 30 min e valor em 0..1023; `mediana` dos válidos. Sem leitura válida → `SEM_LEITURA` (alerta se persistir por 3 avaliações).
3. **Travas.**
   - `mediana < 350` → `STANDBY_UMIDO`.
   - `prob_chuva_4h > 70 %`: se `mediana < 700` → `ADIADA_CHUVA`; senão `emergencia = true`.
   - `radiacao > 700 W/m²` ou hora ∈ [12, 13], com `mediana < 650` e não emergência → `ADIADA_RADIACAO`.
   - Noite (hora ≥ 20 ou ≤ 5), `temp − ponto_orvalho < 1,5 °C`, `mediana < 650`, não emergência → `BLOQUEIO_FUNGICO`.
4. **Balanço hídrico.** `F_atmo = 1 + 0,15·[vpd > 1,8] + 0,10·[vento > 20] + 0,15·[temp > 25 e hum < 18]`;
   `lamina_mm = min(4,5, ((mediana − 350)/500) × 3,5 × F_atmo)`; `volume_l = lamina_mm × area_m2`;
   `duracao_s = round(volume_l / (vazao_nominal_l_min / 60))`, limitada por RF-IRR-02 (`failsafe_s − margem`, hoje 585 s; o Node-RED limita em 600 s, **igual** ao failsafe, o que é corrida).
5. **Corte mínimo.** `duracao_s ≤ 180` → `STANDBY_LAMINA`.

Aprovado → run com `mode = Emergência (chuva tardia)` ou `Balanço Hídrico
Agroclimático (FAO-56)`, `rationale` com todas as entradas, `last_irrigation_at =
now`, `atuar(99,32,1)`, OFF agendado, Telegram `INICIADA` com parâmetros.

### 7.4 Reconciliador

A cada 30 s: (1) runs ativas com `deadline_at < now` e OFF não executado → executar
OFF agora, `end_reason = late_by_reconciler`, Telegram `FINALIZADA (fechada pelo
Reconciliador)`; (2) backstop RF-IRR-07.

### 7.5 Volume

RF-VOL-01. O volume da run é `counter.value(fim) − counter.value(início)` do
`flow_child` do canteiro.

### 7.6 Watchdogs e TTL

RF-WDG-01, RF-REG-09 a RF-REG-11.

---

## 8. Interface do usuário

Aplicação web responsiva, uma página por função, com os blocos de RF-UI-01 a
RF-UI-08. Requisitos de comportamento herdados do dashboard atual:

- Feedback **imediato** ao clicar (âmbar) e cor final só pelo estado do comando; nunca
  pintar "ligado" por ACK de transporte.
- Botão "Ligar" verde permanece enquanto `actuator_state = ON`; ao receber eco de
  preempção, o botão do child preemptado volta ao neutro sem clique.
- Seletor de nó realinha a seleção se o nó sair por TTL.
- Gráficos recebem pontos novos por SSE sem recarregar; replay inicial vem do banco
  (não há mais `seed` embutido em código).
- Todos os horários exibidos em `America/Sao_Paulo` com fuso explícito no rodapé.

Gráficos de longo prazo podem ser complementados por Grafana apontado para o
mesmo banco (opcional, não substitui a UI operacional).

---

## 9. Parâmetros (valores iniciais, todos configuráveis)

### 9.1 Atuadores e failsafe do Nó 99 (espelho do firmware; `inventario.md` §7)

| Children | Carga | Grupo | `failsafe_s` | Limite de run (`−15 s`) |
|---|---|---|---:|---:|
| 31, 32, 33 | Solenóides de gotejamento | `mux99` | 600 | 585 |
| 34–37 | Peristálticas | `mux99` | 120 | 105 |
| 38, 39 | Bombas NFT | — | sem limite | não agendável como run |

### 9.2 Escalas (`inventario.md` §9)

| Grandeza | `V_*` | `value_raw` | `value_si` / unidade |
|---|---|---|---|
| Umidade de solo (S_MOISTURE) | `V_LEVEL` 37 | ADC 0–1023 (alto = seco) | mesma escala; percentual de umidade `100 − raw/10,23` só para exibição, marcado como derivado |
| Temperatura | `V_TEMP` 0 | °C | °C |
| Umidade do ar | `V_HUM` 1 | % | % |
| Vazão | `V_FLOW` 34 | L/min | L/min (UI também em L/s) |
| Bateria | `V_VOLTAGE` 38 / `I_BATTERY_LEVEL` | V / % | V / % |
| Atuação | `V_STATUS` 2 | `"0"`/`"1"` | bool |

### 9.3 Motor do Canteiro B

| Parâmetro | Valor |
|---|---|
| `evaluate_every_s` | 300 |
| `soak_min_cold` / `soak_min_warm` / `soak_temp_threshold_c` | 20 / 15 / 18 |
| `reading_max_age_s` | 1800 |
| `adc_min` / `adc_max` | 0 / 1023 |
| `wet_threshold_adc` | 350 |
| `rain_prob_threshold_pct` / `rain_window_h` | 70 / 4 |
| `critical_stress_adc` | 700 |
| `radiation_threshold_wm2` / `solar_peak_hours` / `radiation_override_adc` | 700 / [12, 13] / 650 |
| `night_hours` / `dew_margin_c` / `night_override_adc` | [20..5] / 1,5 / 650 |
| `vpd_threshold_kpa` / `vpd_factor` | 1,8 / 0,15 |
| `wind_threshold_kmh` / `wind_factor` | 20 / 0,10 |
| `greenhouse_temp_c` / `greenhouse_hum_pct` / `greenhouse_factor` | 25 / 18 / 0,15 |
| `lamina_scale_adc` / `lamina_scale_mm` / `lamina_cap_mm` | 500 / 3,5 / 4,5 |
| `area_m2` | 6,0 |
| `nominal_flow_l_min` | 2,5 |
| `min_duration_s` | 180 |
| `max_duration_s` | `failsafe_s(32) − 15` = 585 |
| `weather_max_age_min` | 90 |

### 9.4 Comandos, vigilância e notificações

| Parâmetro | Valor |
|---|---|
| `ack_timeout_s` / `backoff_factor` / `max_attempts` | 5 / 2 / 3 |
| `lp_awake_window_s` | 2 |
| `mailbox_ttl_h` | 24 |
| `gateway_offline_s` / watchdog period | 120 / 60 s |
| `heartbeat_stale_s` / `ack_stale_s` | 900 / 1800 |
| `node_ttl_h` / sweep | 48 / 300 s |
| `default_timeout_s` (nó sem intervalo) | 450 |
| `reconciler_period_s` / `backstop_s` | 30 / 660 |
| `volume_max_dt_s` | 300 |
| Cooldowns Telegram: lost / reconnected / sketch / cmd ok / cmd erro / gateway | 300 / 120 / 30 / 3 / 60 / 300 s |
| Boletim / relatório | 06,11,16,21 h / 08 h |
| `report_log_cap_chars` | 70 000 |

---

## 10. Plano de migração e desligamento do Node-RED

Pré-requisito de qualquer fase que publique comandos: Nó 99 e gateway gravados
com o failsafe (já é o caso em produção).

| Fase | Entrega | Node-RED | Critério de saída |
|---|---|---|---|
| **0** | Infra: banco, broker local de teste, simulador de nó, CI | ligado | Simulador reproduz eco, preempção, failsafe e sono; testes verdes |
| **1** | Ingestor + persistência + registro (`read_only: true`) | ligado | 7 dias em paralelo: contagem de mensagens igual ao `Central Logger`; status dos nós igual ao watchdog do Node-RED |
| **2** | API, SSE, UI de leitura (mapa, gráficos, log, vigilância) | ligado | Dashboard do backend usado no dia a dia; gráficos conferem |
| **3** | Despachante + botões manuais + painel admin | **botões do Node-RED desativados** (aba Solenóides/Bombas) | 20 comandos manuais confirmados pelo eco; falha simulada vai a `FALHOU` em 35 s com alerta |
| **4** | Agendador durável, Canteiro A, reconciliador | **aba Irrigação desligada antes de ligar o A** | 3 dias de regas A às 07:00/17:00 com OFF confirmado; teste de kill no meio da rega |
| **5** | Motor do Canteiro B, Open-Meteo, soak time | idem | 7 dias de avaliações registradas; decisões conferidas contra o `node.status` histórico |
| **6** | Telegram (alertas, bot, boletim, relatório) | abas Telegram/IA desativadas | Mensagens equivalentes às atuais; assinantes migrados |
| **7** | Corte: Node-RED parado; backup final do `flows.json` e do contexto; `nr_deploy.py` e `DEPLOY.md` marcados como legado | **desligado** | 14 dias sem regressão; rollback documentado (religar o Node-RED e colocar o backend em `read_only`) |

**Migração de dados do Node-RED** (uma vez, script dedicado):

| Origem (contexto do Node-RED) | Destino |
|---|---|
| `mys_nodes` (store `file`) | `node`, `child`, `actuator_state` (com `certain = false`) |
| `flow_vol_node_99_21` | `counter(99, 21, volume_l)` valor inicial |
| `telegram_subscribers`, `TELEGRAM_CHAT_ID` | `telegram_subscriber` |
| `m360_daily_metrics_prev` | `daily_metrics` (D-1) |
| `hist_*` (7 dias) | opcional, `reading` com `source = nodered_import` |
| `mqtt_logs` (export CSV) | arquivo em `docs/csv/`, não importado |

Regra de ouro durante a convivência: **nunca duas fontes comandando os children
31–37 ao mesmo tempo** — a preempção do MUX corta a rega de uma pela outra.

---

## 11. Implantação e operação

- Containers (`docker compose`): `backend`, `postgres` (volume persistente, backup diário `pg_dump` para fora do host), `mosquitto` opcional para teste local. Em produção o broker existente continua sendo usado.
- Reverse proxy com TLS para UI/API (`horta.viridiotech.com.br` ou equivalente).
- Logs em stdout JSON; rotação pelo runtime de containers.
- Migrações de esquema versionadas (Alembic ou equivalente) e aplicadas no start.
- Backup: banco diário; configuração versionada no repositório (`backend/config/`), segredos em arquivo `.env` com permissão restrita, fora do repositório, como hoje `%USERPROFILE%\.m360\nodered.env`.
- Local no monorepo: `backend/` na raiz (código, testes, `compose.yaml`, `README.md`), ao lado de `src/`, `lib/` e `hardware/`. Não entra no `build_src_filter` do PlatformIO.

---

## 12. Testes e critérios de aceite

### 12.1 Simulador de nó (obrigatório, Fase 0)

Processo que se conecta ao broker e emula o gateway + Nó 99 + um nó LP:

- Responde a `in/99/{31..39}/1/0/2` com o eco `out/99/{child}/1/0/2` e payload igual, após latência configurável; publica `transport_ack` (`ack=1`) antes do eco.
- Grupo MUX: ao ligar 31–37 com outro canal ligado, publica `0` do preemptado antes do eco do comandado.
- Failsafe: solenóide ligada por 600 s publica `0` sozinha; peristáltica em 120 s.
- Rejeita payload fora de `"0"/"1"` com `command_rejected` em `out/events`.
- Nó LP: só responde a comandos recebidos dentro de 2 s após uma transmissão própria; transmite a cada N s com `I_PRE/POST_SLEEP_NOTIFICATION`.
- Heartbeat do gateway a cada 60 s; opção de "cair" para testar watchdog e reboot (uptime zera).
- Perfis de falha: perder o eco (para testar retry), atrasar o eco além de 5 s, publicar envelope JSON em vez de nativo.

### 12.2 Cenários de aceite (Dado / Quando / Então)

1. **Payload tipado.** Dado qualquer comando de atuação da API, quando publicado, então o payload é exatamente `"0"` ou `"1"` (teste de propriedade sobre todos os children configurados).
2. **Confirmação real.** Dado comando `atuar(99,31,1)`, quando chega só `ack=1`, então o estado segue `ENVIADO`; quando chega o eco `out/99/31/1/0/2 = 1`, então `CONFIRMADO` e `actuator_state = ON`.
3. **Retry e falha.** Dado o simulador sem eco, então 3 publicações em 5 s, 10 s e 20 s e estado `FALHOU` em ~35 s, com alerta Telegram único.
4. **Preempção.** Dado 31 `ON` e comando 32 `ON`, quando chegam `31=0` e `32=1`, então ambos os estados refletem isso e a run de A (se houver) termina com `end_reason = failsafe_or_preempt` e alerta.
5. **Failsafe não solicitado.** Dado run B em curso e `out/99/32/1/0/2 = 0` sem comando, então run encerrada, OFF agendado cancelado, estado `OFF`, evento `actuator_unsolicited_off`.
6. **Durabilidade.** Dado run de 60 s, quando o processo morre em 20 s e volta em 70 s, então um único OFF é publicado e a run fica `FINALIZADA (late_by_reconciler)`.
7. **Caixa postal.** Dado comando a nó LP dormindo, então estado `AGUARDANDO_DESPERTAR`; quando o nó transmite, então o comando é publicado em < 500 ms.
8. **Soak time.** Dado rega B iniciada às 10:00 com `temp = 16 °C`, quando o avaliador roda às 10:15, então `STANDBY_SOAK`; às 10:20, avalia normalmente.
9. **Clima indisponível.** Dado `climate_source` = Nó 4 sem leitura há mais de `max_age_s` e `fallback = open_meteo`, então o motor usa `temp_ext`/`hum_ext` e registra a origem em `rationale`; com leitura recente do Nó 4, a origem registrada é `node:4`.
10. **Watchdog.** Dado gateway silencioso por 121 s, então `gateway_offline` e alerta; quando volta, então alerta `online` e cooldown respeitado.
11. **Apresentação.** Dado `REPRESENT` para um nó com children antigos, então a tabela é zerada e reconstruída só com os apresentados; estados de atuadores preservados.
12. **Gráficos limpos.** Dado um `REPRESENT`, então nenhuma leitura `NaN` entra em `reading`.
13. **Volume.** Dado vazão 2,5 L/min por 120 s em amostras de 60 s, então o contador cresce 5,0 L; amostra após lacuna de 400 s não acrescenta nada.
14. **Multi-instalação.** Dado tráfego em `m360/DF/0000` e `m360/GO/0001`, então nós, comandos e alertas não se misturam.
15. **Segredo ausente.** Dado `TELEGRAM_BOT_TOKEN` vazio, então o processo não sobe e o log diz qual variável falta.
15a. **Repareamento.** Dado `out/99/253/1/0/47 = REJOIN` e, 20 s depois, apresentações dos children e `REJOIN:OK`, então existem os eventos `node_rejoin_started` e `node_rejoined`, a tabela de children do Nó 99 **não** é zerada, o estado dos atuadores não muda e um alerta é enviado.
15b. **Reed da LP.** Dado o Nó 4 com `interval_min = 30` e uma transmissão 10 min após a anterior, então `cycle_ms` não muda e a caixa postal do nó é liberada.

### 12.3 Checklist de paridade com o Node-RED

Cada function node do `flows.json` mapeado para o requisito que o substitui:

| Node-RED (aba · nó) | Requisito |
|---|---|
| ACK Handling · Decodificador Nativo MySensors | RF-ING-01..03 |
| ACK Handling · Mapeia nós, Limpeza TTL, Watchdog da rede, Processador do Mapa | RF-REG-01..13, RF-UI-04/05 |
| ACK Handling · Sincronizador ACK / Timeout (fila, mailbox, bypass) | RF-CMD-01..13 |
| ACK Handling · Filtros Solo A/B, Clima, SolarMini, Bateria, Vazão e Volume; Salvar Histórico; Replay | RF-ING-05/06, RF-VOL-01/02, RF-UI-03 |
| ACK Handling · Gateway Status + Watchdog, Verifica Gateway Offline | RF-ING-08, RF-WDG-01 |
| ACK Handling · Central Logger, Esvaziar, Formata CSV, `/api/mqtt-log/export` | RF-ING-04, RF-UI-06 |
| ACK Handling · Monitor de Falhas & Alertas Telegram | RF-NOT-03/04, RF-ING-07 |
| Irrigação · Controlador Canteiro A | RF-IRR-01..03, §7.2 |
| Irrigação · Coletor Telemetria, Motor de Regras Canteiro B, Processar Variáveis Agroclimáticas | RF-IRR-04/05, §6.4, §7.3 |
| Irrigação · Reconciliador | RF-IRR-07, §7.4 |
| Irrigação · Formatador Mensagem Telegram | RF-NOT-05 |
| Solenóides/Bombas · Despachante, Feedback Visual, Processador do Comando UI, Dropdown | RF-UI-01/02, RF-CMD-02 |
| MQTT · injects de teste, Tradutor Comando Backend, Pub MQTT | RF-API-01 (`POST /api/commands`) |
| Telegram Bot · Tratar Comandos e Convites, Boletim 4x dia | RF-NOT-01/02/06 |
| IA Repórter · Radar M360, Gemini, Snapshot D-1 | RF-NOT-07, §6.5 |

---

## 13. Decisões em aberto (confirmar antes da Fase 0)

| ID | Decisão | Recomendação | Alternativa |
|---|---|---|---|
| **D-01 Linguagem e runtime** | Python 3.12+ com `asyncio`, `aiomqtt`, FastAPI, SQLAlchemy 2 + Alembic | Já é a linguagem das ferramentas do repo (`nr_deploy.py`) e do dono do projeto; simulador e testes ficam no mesmo stack | Node.js/TypeScript (NestJS), coerente com o entregável ENT-DOC-01 do Manejo360, que previa Node.js/Express. Os documentos ENT-DOC não estão neste repositório e a taxonomia de tópicos que citam (`m360/out/{gateway_id}/...`) não é a do firmware |
| **D-02 Banco** | PostgreSQL 16, `reading` particionada por mês | Relacional cobre tudo; TimescaleDB pode ser ligado depois sem mudar o modelo | SQLite para instalação única (perde concorrência e particionamento) |
| **D-03 UI** | Páginas servidas pelo backend (templates + HTMX ou Vue leve), gráficos com uPlot/ECharts, SSE | Um só deploy, sem build pesado | SPA separada (React/Vue) ou Grafana para tudo (perde controle manual) |
| **D-04 Fonte de clima do Canteiro B** | **Decidida (2026-10-17):** `climate_source = Nó 4 (11/12)`, `max_age_s = 3600`, `fallback = open_meteo` | Já em produção no Node-RED; o Nó 99 na ACT não tem DHT. O clima é o de fora da estufa e muda a cada 30 min | Mover um DHT para dentro da estufa (LP com `LP_HAS_DHT`) e apontar a `climate_source` para ele |
| **D-05 Hospedagem** | Mesmo servidor do broker e do Node-RED (`viridiotech`), em containers | Latência mínima com o broker; infra já existe | Servidor dedicado |
| **D-06 Autenticação** | Usuário/senha local com sessão e tokens de API; um papel `admin` | Suficiente para a equipe atual | OIDC quando houver mais usuários |
| **D-07 Peristálticas na UI** | Botões 34–37 atrás de confirmação explícita | Dosagem de pH/suplemento não deve ser clique único | Omitir da UI até existir regra de dosagem |
| **D-08 Kit Hélio** | Segunda instalação na mesma instância, só leitura | Reaproveita tudo | Instância separada |
| **D-09 Separação de rádio por propriedade** | **Grade de 64 canais de 200 kHz em 915–928 MHz** (§15.6): índice 0..63, `f = 915,2 MHz + 0,2 MHz × k`; o portal do gateway grava o índice na EEPROM, os nós o recebem por comando de provisionamento, e o transporte lê a frequência no `before()`. Reuso do mesmo canal só entre propriedades fora do alcance mútuo, conferido pelo cadastro (RF-PRV-12) | Medido em 2026-09-10: frequência em variável custa 4 B de RAM e funciona; 200 kHz é a grade do LoRaWAN AU915 e absorve o desvio do cristal (até 18 kHz em 915 MHz); o canal atual, 916,8 MHz, é o índice 8 | Grade de 125 kHz (104 canais, adjacentes se degradam); manter em compilação (`[common]`), um binário por propriedade; sync word por propriedade (o driver da MySensors não o programa e poucos valores são confiáveis); assinatura MySensors, que **não cabe** no Nó 99 (103 B de pilha) |

---

## 14. Rastreabilidade com o restante do repositório

- Alterações de child ID, label ou failsafe no firmware exigem atualizar **na mesma entrega** o `inventario.md` e a configuração `actuators`/`beds` do backend (§5.6). O teste de aceite 1 e um teste de contrato que lê o `inventario.md` detectam divergência.
- O `inventario.md` §8 ("Contrato com o Node-RED") passa a chamar-se "Contrato com o backend" na Fase 7, apontando para a configuração do backend em vez de nós do `flows.json`.
- `CLAUDE.md`: a seção "Deploy no Node-RED" vira histórico na Fase 7; a seção "Armadilhas Críticas" permanece válida e passa a citar os requisitos RF-CMD-04 e RF-CMD-08.
- `hardware/ARQUITETURA_FAMILIA.md` §9 e §6.1: o factory reset do gateway foi implementado em 2026-10-17 conforme RF-PRV-09 (exceto o botão do portal).

---

## 15. Provisionamento de nova propriedade

Uma propriedade é uma instalação: um gateway, um prefixo `m360/{UF}/{CAR}`, seus
nós, suas regras e seus operadores. Esta seção fixa o fluxo do primeiro dia e o
que o backend tem de oferecer para que ele não dependa de editar arquivo nem de
recompilar firmware.

### 15.1 Fluxo

```
 escritório                    campo                              backend
 ──────────                    ─────                              ───────
 1 reservar UF/CAR          4 gateway com A0 em GND (botão      7 tráfego em prefixo novo →
 2 usuário MQTT + ACL         ou ímã no reed) → AP M360-Config    instalação PENDENTE + alerta
   restrita ao prefixo      5 portal 192.168.4.1: WiFi, MQTT,   8 operador confirma na UI:
 3 gravar firmware dos nós    UF, CAR, [frequência/canal]         nome, fuso, coordenadas,
   (IDs 1..254 livres;      6 LED verde fixo; heartbeat em        atuadores, canteiros,
   um gateway por             m360/UF/CAR/out                     Telegram → COMISSIONANDO
   propriedade)             6b ligar nós: apresentação           9 checklist (§15.4) completo
                              automática                           → ATIVA; regras habilitadas
```

### 15.2 Lado do firmware (estado atual e requisitos)

| Etapa | Hoje | Requisito |
|---|---|---|
| Identidade da instalação | Portal grava `ssid, password, mqttServer, mqttPort, mqttUser, mqttPassword, uf, carNumber` na EEPROM (521+, CRC) | Mantido. O portal passa a validar `UF` (2 letras) e `CAR` (1–7 dígitos) e a mostrar o prefixo resultante |
| Separação de rádio | Frequência RFM95W fixa no `[common]` do `platformio.ini` (vale para as placas M360; protótipos e Kit Hélio são fixos em nRF24); todo gateway é o nó 0 e o vizinho na mesma frequência confirma pacotes alheios | **D-09 / §15.6**: campo `canal` (0..63) no portal, gravado na EEPROM e convertido em frequência no `before()`; nos nós, o índice entra por comando de provisionamento `V_CUSTOM` `SET_RADIO:<k>` enquanto o nó ainda está no canal de fábrica, com confirmação pelo eco antes de o nó trocar |
| Nós | `MY_NODE_ID` por env; IDs 1–254 recomeçam em cada propriedade; apresentação automática | Mantido. As faixas por função de `M360Config.h` (`isClimaNode` 1–50 etc.) **não** são usadas pelos nós reais e não entram no fluxo |
| Factory reset do gateway | **Por hardware: implementado** (2026-10-17, `M360::FactoryReset`). Pelo portal: não existe | **RF-PRV-09** — falta o botão do portal |

### 15.3 Requisitos (PRV)

- **RF-PRV-01** Cadastro de instalação pela API/UI (`POST /api/installations`) com `uf, car, name, timezone, lat, lon, mqtt_user`, lista de atuadores (child, rótulo, grupo MUX, failsafe) e canteiros. O YAML da §5.6 continua aceito como semente inicial e para ambientes de teste; em produção a fonte é o banco.
- **RF-PRV-02** Unicidade de `mqtt_prefix`. Tentativa de cadastrar um prefixo já existente é recusada com `409` e o nome da instalação que o detém.
- **RF-PRV-03** **Descoberta:** mensagem válida em prefixo desconhecido cria `installation` em `PENDENTE` com o que dá para inferir (uf, car, primeiro contato, versão do gateway pelo heartbeat) e envia alerta ao chat administrativo. Enquanto `PENDENTE`: ingestão e registro **ligados**, comandos, regras e notificações da instalação **desligados**.
- **RF-PRV-04** Confirmação pelo operador leva a `COMISSIONANDO`; nesse estado comandos manuais são permitidos e regras continuam desligadas.
- **RF-PRV-05** `ATIVA` só com o checklist da §15.4 completo. Regras de irrigação só podem ser habilitadas em `ATIVA`.
- **RF-PRV-06** `SUSPENSA`: instalação desativada pelo operador (propriedade desligada, gateway em manutenção). Ingestão continua, alertas de offline são silenciados, comandos recusados.
- **RF-PRV-07** Credencial MQTT por instalação, com ACL do broker restrita a `m360/{UF}/{CAR}/#`. O backend conecta com uma credencial de serviço que lê `m360/+/+/#` e publica em todos os prefixos. A ACL é requisito de implantação (§11), verificada no checklist.
- **RF-PRV-08** Coordenadas por instalação alimentam a Open-Meteo (§6.4) e o boletim; fuso por instalação governa cron, soak time e exibição.
- **RF-PRV-09** Factory reset do gateway, requisito de firmware rastreado aqui porque é pré-condição para reaproveitar um gateway em outra propriedade.
  - **Por hardware (implementado):** nos primeiros **30 s do `loop()`** (não do boot: a conexão WiFi no `before()` pode passar disso), o ímã/botão é **afastado** e depois mantido **3 s** no A0. Segurá-lo desde o boot só abre o modo AP — é preciso soltar e aproximar de novo, para quem só quer o AP não apagar nada.
  - **Efeito:** publica `factory_reset` em `out/events` com o prefixo atual, **apaga** a `M360DeviceConfig` (EEPROM 521+) e reinicia direto no modo AP. Nada da instalação anterior sobrevive. Ao entrar no AP o firmware grava os padrões de compilação; se ninguém configurar pelo portal, o boot seguinte os usa, como um gateway novo.
  - **Pelo portal (pendente):** botão "restaurar padrões" com o mesmo efeito.
  - **Backend:** ao receber `factory_reset`, marcar o gateway como `RESETADO`, alertar (RF-NOT-03) e não tratar o silêncio seguinte como falha.
- **RF-PRV-10** Operadores: cada instalação tem seu chat principal e seus assinantes do Telegram; o convite `/start convite` fica vinculado à instalação pelo link gerado na UI (`?start=convite_<installation_id>`).
- **RF-PRV-11** Toda mudança de estado da instalação e toda alteração de atuadores ou canteiros é registrada com usuário e horário.
- **RF-PRV-12** Canal de rádio por instalação (`installation.radio = {banda, canal}`, §15.6). No cadastro e na edição, o backend lista as instalações no mesmo canal a menos de `raio_reuso_km` (padrão 10 km, configurável) das coordenadas informadas e **recusa** o cadastro com `409` se houver alguma; o administrador pode forçar com justificativa registrada. O checklist C5 confere na prática o que esta regra confere no papel.

### 15.4 Checklist de comissionamento

Executado na UI, um item por vez, com evidência gravada em `commissioning_check`.
Itens automáticos são verificados pelo backend; os manuais exigem confirmação do
operador no local.

| # | Verificação | Como o backend confere |
|---|---|---|
| C1 | Gateway online | Heartbeat há menos de 120 s, `mqtt_connected = true` |
| C2 | Credencial e ACL do broker | Publicação de teste fora do prefixo com a credencial da instalação é **recusada** |
| C3 | Nós esperados apresentados | Lista de nós informada no cadastro; todos com `sketch_name` e children apresentados |
| C4 | Children conferem com o cadastro | Cada atuador cadastrado existe como child `S_BINARY` no nó; divergência é listada |
| C5 | Rádio isolado | Canal cadastrado igual ao gravado no gateway (o heartbeat passa a informar `radioChannel`); nenhuma outra instalação `ATIVA` no mesmo canal dentro de `raio_reuso_km`; nenhum nó de **outra** instalação visto neste prefixo nas últimas 24 h e vice-versa (gateway vizinho confirmando pacotes aparece como nós fantasmas) |
| C6 | Comando manual por atuador | Um `atuar(…,1)` e um `atuar(…,0)` `CONFIRMADO` pelo eco em cada atuador de solenóide e bomba; peristálticas opcionais |
| C7 | Preempção do MUX | Ligar 31 e depois 32 produz eco `31 = 0` antes de `32 = 1` |
| C8 | Vazão | Com a solenóide aberta, `V_FLOW > 0` no child de vazão do canteiro; contador cresce |
| C9 | Failsafe | Solenóide ligada e deixada por 600 s publica `0` sozinha (uma vez, na instalação; evidência gravada) |
| C10 | Telegram | Mensagem de teste recebida pelo chat principal; pelo menos um operador registrado |
| C11 | Open-Meteo | Snapshot válido para as coordenadas da instalação |
| C12 | Nós LP | Pelo menos um ciclo de sono observado (`type 32/33`) e um comando entregue pela caixa postal |

C9 é manual e demorado; pode ser dispensado pelo administrador com justificativa
registrada, mas a instalação fica marcada `failsafe_nao_verificado` até ser feito.

### 15.5 Cenários de aceite

16. **Descoberta.** Dado tráfego em `m360/GO/0001` sem cadastro, então existe `installation` `PENDENTE` com `uf = GO, car = 0001`, alerta enviado uma vez, leituras persistidas e nenhum comando aceito para o prefixo (`409`).
17. **Prefixo duplicado.** Dado cadastro de `DF/0000` já existente, então `POST /api/installations` devolve `409` com o nome da instalação existente.
18. **Regra bloqueada.** Dado instalação `COMISSIONANDO`, quando o operador tenta habilitar a regra do Canteiro A, então a API recusa com a lista dos itens do checklist pendentes.
19. **ACL.** Dado a credencial da instalação `GO/0001`, quando se publica em `m360/DF/0000/in/99/31/1/0/2`, então o broker recusa e C2 passa; se aceitar, C2 falha e a instalação não pode ir a `ATIVA`.
20. **Nó fantasma.** Dado um nó com `sketch_name` de outra instalação aparecendo no prefixo, então C5 falha e o alerta cita o nó e o prefixo de origem provável.
21. **Suspensão.** Dado instalação `SUSPENSA` e gateway silencioso por 10 min, então nenhum alerta de `gateway_offline` é enviado; ao reativar, o watchdog volta a valer.
22. **Reaproveitamento de gateway.** Dado gateway com configuração de `DF/0000` já no `loop()`, quando o ímã é afastado e depois mantido 3 s no A0 dentro dos 30 s iniciais, então `factory_reset` chega em `m360/DF/0000/out/events`, o gateway reinicia em AP (`M360-Config`) sem a configuração anterior e o backend não alerta `gateway_offline` para `DF/0000` (firmware verificado em bancada; backend no simulador).
23. **Canal em conflito.** Dado instalação `ATIVA` em `DF/0000` no canal 8 em (−15,964; −47,804), quando se cadastra `DF/0001` no canal 8 a 3 km, então `POST /api/installations` devolve `409` citando `DF/0000`; a 25 km, o cadastro é aceito.

### 15.6 Grade de canais de rádio (RFM95W, 915 MHz)

Base legal: radiação restrita em **915–928 MHz** (13 MHz) e 902–907,5 MHz no Brasil,
sem limite de ciclo de trabalho e com teto de potência bem acima dos 17 dBm usados.
Modem do `[common]`: BW 125 kHz, CR 4/5, SF7 (`RFM95_BW125CR45SF128`). O RFM95W
sintoniza com passo de 61 Hz, então a grade é escolha de projeto.

| Parâmetro | Valor |
|---|---|
| Banda `A` (padrão) | 915–928 MHz |
| Espaçamento | 200 kHz (mesma grade dos canais de subida do LoRaWAN AU915) |
| Canais | `k = 0..63`, `f(k) = 915,2 MHz + 0,2 MHz × k` (915,2 … 927,8 MHz) |
| Canal de fábrica | `k = 8` → 916,8 MHz, o valor atual de `MY_RFM95_FREQUENCY` |
| Banda `B` (reserva) | 902–907,5 MHz, `k = 64..90`, `f(k) = 902,2 + 0,2 × (k − 64)` MHz; só se a banda A esgotar numa região |
| Reuso geográfico | Mesmo canal permitido entre instalações a mais de `raio_reuso_km` (padrão 10 km) |
| Fora da grade | Não aceito pelo portal nem pelo cadastro |

Por que 200 kHz e não 125 kHz: o sinal ocupa 125 kHz, mas o cristal do módulo
desvia até ±18 kHz em 915 MHz (20 ppm) em cada ponta do enlace; com 125 kHz de
espaçamento canais adjacentes se sobrepõem na prática e a rejeição de canal
adjacente do SX1276 deixa de valer. A grade de 125 kHz daria 104 canais, mas
nenhum deles seria limpo com vizinho ocupado.

Por que não sync word: o SX1276 filtra por sync word em hardware, mas o driver
RFM95 da MySensors não programa o registrador (fica o padrão 0x12), poucos valores
são confiáveis e isso exigiria patch na biblioteca. Frequência resolve sozinha.

Limite prático: 64 gateways **dentro de um mesmo alcance de rádio**; com reuso
geográfico o número de instalações é indefinido. O que o backend garante é que
duas instalações ao alcance uma da outra nunca compartilhem canal (RF-PRV-12).

Requisitos de firmware derivados (fora do backend, rastreados aqui): campo `canal`
na `M360DeviceConfig` e no portal; `radioChannel` no heartbeat do gateway;
comando `SET_RADIO:<k>` nos nós com persistência em EEPROM e eco de confirmação;
canal de fábrica `k = 8` em todos os binários.
