# Extrato de fontes — Backend M360 (substituto do Node-RED)

**Gerado em:** 2026-10-17 · **Para:** redação do PRD (BMad)
**Fontes lidas:**
- `docs/especificacao-backend-m360.md` v1.3 (2026-10-17) — **normativa**, citada como "spec §x"
- `docs/Backend Definitivo M360.md` — lições do backend temporário, citado como "guia §x"
- `src/DRY/horta/nodered/DEPLOY.md` — citado como "DEPLOY §x"
- `src/DRY/horta/nodered/flows.json` — só a lista de abas e function nodes (7 abas, 51 function nodes, conferido por script)
- `CLAUDE.md` — "Armadilhas Críticas…" e "Fonte de clima do Node-RED", citado como "CLAUDE §nome"

> Status da spec: **"proposta para aprovação do dono do projeto"** (spec cabeçalho). Onde guia e spec divergem em número, vale a spec §9; em child ID, vale `inventario.md` (spec cabeçalho).

---

## 1. O que o Node-RED faz hoje (capacidades em produção a preservar)

Uma linha por aba. Function nodes entre parênteses (nomes reais do `flows.json`). Mapeamento para RF em spec §12.3.

| Aba (ID em DEPLOY §4) | O que faz hoje |
|---|---|
| **M360 Horta - ACK Handling** (`50fbcdf0053fa19f`, 30 functions) | Núcleo: decodifica o MQTT nativo, mantém o registro de nós (`mys_nodes`), watchdog do gateway e TTL de nós, mapa da rede, filtros e históricos dos gráficos (solo A/B, clima, SolarMini, bateria, vazão, volume) com replay, log MQTT central com exportação CSV, sincronizador de comandos com timeout/retry e caixa postal LP, e alertas Telegram de falha de comunicação (spec §12.3). |
| **Irrigação** (`tab_irrigacao_m360`, 6 functions) | Canteiro A por timer fixo 07:00/17:00 de 5 min; Canteiro B pelo motor de regras Manejo360 com clima do Nó 4 e Open-Meteo; formata as mensagens de rega para o Telegram; reconciliador que fecha o OFF perdido (spec §12.3; CLAUDE §Fonte de clima). |
| **Fluxo 2 - Solenóides** (`a757ff0be26f944a`, 5 functions) | Botões manuais das solenóides/peristálticas com despachante, feedback visual imediato e dropdown de seleção de nó/ação administrativa (spec §12.3, RF-UI-01/02). |
| **Fluxo 2 - Bombas** (`1e714cfec06339e2`, 3 functions) | Botões manuais das bombas NFT/oxigenação com despachante e feedback visual (spec §12.3). |
| **MQTT** (`6e38860673f8fc3a`, 3 functions, 17 injects) | Injects de teste, tradutor de comando do "backend", publicação em tópico dinâmico e uma tabela de watchdog da rede (spec §12.3 → RF-API-01). |
| **Telegram Bot Interativo** (`tab_telegram_bot`, 2 functions) | Bot com `/start`, convites, `/status`, `/tempo` e boletim meteorológico 4×/dia (spec §12.3 → RF-NOT-01/02/06). |
| **IA Repórter (Diário)** (`5f670c411a47a898`, 2 functions) | Relatório diário 08:00: contexto 24 h + comparativo D-1 enviado ao Gemini, resposta no Telegram, snapshot D-1 salvo (spec §12.3 → RF-NOT-07). |

Fatos operacionais do Node-RED que justificam a troca (guia §8, §11):
- OFF da rega é `setTimeout` em function node: redeploy/restart perde o OFF (guia §8.1; DEPLOY §5).
- Só `mys_nodes` vai a disco; volume acumulado, soak time, estado de relés e históricos zeram a cada restart (guia §8.2).
- Sincronizador já aceitou ACK de transporte como confirmação: retries nunca disparavam (guia §8.3; CLAUDE §Sincronizador ACK).
- Segredos com fallback para string vazia: falha silenciosa (guia §8.4).
- `out/events` é descartado inteiro (spec RF-ING-07).
- Caixa postal guarda 1 comando por nó e sobrescreve sem aviso (spec RF-CMD-09).
- Watchdog e mapa usam critérios de status diferentes (spec RF-REG-10).
- Sem clima, motor B cai em default silencioso 20 °C / 20 % (CLAUDE §Fonte de clima; spec §7.3).
- Limite de rega B em 600 s = failsafe do firmware, corrida (spec §7.3 camada 4).
- Deploy é manual por aba via `nr_deploy.py`, servidor frequentemente à frente do repo (DEPLOY §3, §6).

---

## 2. Features e requisitos funcionais

83 RFs mapeados (ING 10, REG 13, CMD 13, IRR 10, VOL 3, WDG 3, NOT 8, UI 8, API 3, PRV 12), cada um atribuído a **uma** feature primária. Linhas resumem a capacidade testável — não são o texto da spec.

### F1 — Ingestão de telemetria (spec §3.1)
Recebe toda mensagem que o gateway publica, entende os dois formatos do contrato (nativo e envelope JSON), guarda tudo num log auditável e transforma só o que é medição em série temporal. Eventos de transporte e de validação do gateway, hoje descartados, passam a ser registrados.

- **RF-ING-01** Assina saída, status e entrada de cada instalação; classifica a mensagem pelo nº de segmentos do tópico, não pelo conteúdo.
- **RF-ING-02** Decodifica formato nativo e envelope JSON; tópico com segmento não numérico é rejeitado e contado em métrica.
- **RF-ING-03** Toda mensagem vira um registro normalizado com direção `sensor` ou `transport_ack` (ack=1).
- **RF-ING-04** Todo tráfego (inclusive comandos do próprio backend e de terceiros) fica no log com retenção configurável (30 d).
- **RF-ING-05** Só medições numéricas (`cmd=1, ack=0`) viram leitura; apresentação e internas nunca entram em gráfico (fim do `NaN`).
- **RF-ING-06** Cada leitura guarda valor bruto e valor convertido com unidade; percentual de solo derivado do tipo `S_MOISTURE`, não de lista de nós.
- **RF-ING-07** Eventos do gateway (`out/events`) são persistidos.
- **RF-ING-08** Heartbeat e métricas do gateway atualizam seu estado; reboot do gateway é detectado (uptime menor).
- **RF-ING-10** Texto de debug remoto (child 253) vira evento de nó; `REJOIN`/`REJOIN:OK` geram eventos com duração; resposta de `DEBUG_NET` é ligada ao comando.

### F2 — Registro de nós e children (spec §3.2)
Mantém automaticamente a lista de nós e sensores a partir do que os próprios nós anunciam, sem lista fixa. Deriva perfil de energia, cadência, timeout e um status único (o mesmo para API, UI e alerta).

- **RF-REG-01** Nó criado na primeira mensagem, com contadores de primeiro/último contato e mensagens.
- **RF-REG-02** Apresentação do nó zera a tabela de children (exceto no `type 30`) e agenda poda de órfãos.
- **RF-REG-03** Apresentação de child grava tipo e rótulo; child sem apresentação recebe placeholder.
- **RF-REG-04** `REPRESENT` observado em qualquer direção também zera a tabela.
- **RF-REG-05** Poda remove valores de children que saíram, preservando 253/254/255 e **nunca** o estado de atuadores.
- **RF-REG-06** Mensagens internas alimentam bateria, sketch, versão, heartbeat, marca de sono, tensão e intervalo.
- **RF-REG-07** Perfil de energia (`LP/ON/PAS/REP`) vem do sufixo do sketch, com fallback pela cadência.
- **RF-REG-08** Cadência por média móvel; acordadas fora do ciclo (reed, `FORCE_UPDATE`) não encurtam a média.
- **RF-REG-09** Timeout de inatividade calculado num único ponto, espelhando o gateway (padrão 450 s).
- **RF-REG-10** Status único: `SEM DADOS / OFFLINE / SEM HEARTBEAT / SEM ACK / ONLINE`.
- **RF-REG-11** Nó sem contato > 48 h sai da lista ativa (`EXPIRED`) sem perder histórico.
- **RF-REG-12** Categoria do nó (solo, clima, atuação, água, gateway) derivada dos children.
- **RF-REG-13** Nós Horta e Kit Hélio aparecem separados por instalação.

### F3 — Comandos e atuação (spec §3.3, §5.3, §7.1)
Todo comando a um nó é um registro com ciclo de vida auditável. Atuação só é considerada feita quando o próprio nó ecoa o novo estado; há retentativa, fila por nó, exclusão mútua do MUX do Nó 99 e caixa postal para nós que dormem.

- **RF-CMD-01** Todo comando nasce `PENDENTE` e percorre a máquina de estados; nada publica fora do despachante.
- **RF-CMD-02** API aceita só `atuar`, `ler`, `represent`, `force_update`, `debug_net`, `set_interval(1..1440)`.
- **RF-CMD-03** Validação antes de publicar; `atuar` só em child atuador apresentado ou configurado; desconhecido é rejeitado com motivo.
- **RF-CMD-04** `atuar` só é `CONFIRMADO` pelo eco de aplicação com payload igual; ACK de transporte só é anotado.
- **RF-CMD-05** Sem eco: 3 tentativas (5/10/20 s) e `FALHOU`; parâmetros por instalação.
- **RF-CMD-06** `command_rejected` → `REJEITADO` com motivo; `command_send_failed` conta como tentativa.
- **RF-CMD-07** Administrativos terminam em `ENVIADO`; `set_interval` vira `CONFIRMADO` ou `DIVERGENTE` pela leitura do child 254.
- **RF-CMD-08** Fila FIFO por nó; children 31–37 do Nó 99 nunca têm dois comandos em voo; eco de preempção desliga os demais.
- **RF-CMD-09** Caixa postal FIFO para nó LP; libera na próxima transmissão; expira em 24 h; reed força a janela.
- **RF-CMD-10** Todo comando registra origem (`user/rule/reconciler`) e todas as transições com horário.
- **RF-CMD-11** Eco não solicitado atualiza o atuador; `0` sem comando gera evento e encerra a rega ativa (`failsafe_or_preempt`).
- **RF-CMD-12** No startup, estado dos atuadores é recarregado e marcado incerto se gateway/Nó 99 rebootaram; OFF geral opcional.
- **RF-CMD-13** Chave de diagnóstico desliga a espera de eco; seu estado é visível na UI e no health.

### F4 — Irrigação A e B (spec §3.4, §7.2, §7.3, §7.4, §9.3)
Executa as duas parcelas do experimento: A (controle) por horário fixo e B (Manejo360) por motor agroclimático de cinco camadas a cada 5 min. Toda rega é um registro com OFF agendado de forma durável, limitada abaixo do failsafe do firmware, e um reconciliador fecha o que o agendador perder.

- **RF-IRR-01** Toda rega é um registro; o OFF é ação persistida no agendador, nunca timer em memória.
- **RF-IRR-02** Duração limitada a `failsafe − 15 s` (585 s nas solenóides); excesso truncado e registrado.
- **RF-IRR-03** Canteiro A: 07:00 e 17:00 (Brasília), 300 s, child 31, sem sensores — não acrescentar inteligência.
- **RF-IRR-04** Canteiro B: avaliação a cada 300 s pelo pipeline de 5 camadas; cada avaliação persistida com entradas e camada decisora.
- **RF-IRR-05** Soak time 15/20 min (limiar 18 °C) entre regas do B, persistido por canteiro.
- **RF-IRR-06** Se o comando de abertura falha, a rega termina `command_failed`, OFF é cancelado e o Telegram avisa.
- **RF-IRR-07** Reconciliador a cada 30 s fecha runs vencidas e desliga solenóide `ON` > 660 s sem run (bombas fora).
- **RF-IRR-08** Rega manual por API/UI segue o mesmo caminho (run, OFF, soak, Telegram, limite).
- **RF-IRR-09** Regras A e B habilitáveis/desabilitáveis sem redeploy; nunca o mesmo canteiro comandado por dois sistemas.
- **RF-IRR-10** Volume de cada rega atribuído pela integração da vazão do child associado ao canteiro.

### F5 — Volume e contadores (spec §3.5, §7.5)
Converte vazão instantânea em volume acumulado persistente, sem inventar consumo em lacunas, para qualquer nó com hidrômetro.

- **RF-VOL-01** Integra vazão em volume só com `0 < dt ≤ 300 s`; contador persistido a cada atualização.
- **RF-VOL-02** Séries de vazão (L/s) e volume acumulado disponíveis para qualquer nó com hidrômetro.
- **RF-VOL-03** Totais diários e por rega calculados do banco, nunca de acumulador em RAM.

### F6 — Vigilância (spec §3.6, §7.6)
Detecta gateway e nós fora do ar por um critério único e evita que a interface mostre dado velho como atual.

- **RF-WDG-01** Gateway `OFFLINE` após 120 s sem contato (avaliação a cada 60 s), com eventos de transição.
- **RF-WDG-02** Status dos nós recalculado a cada 60 s; eventos do gateway (`node_lost`/`reconnected`) também alimentam alertas, um por transição.
- **RF-WDG-03** Com gateway `OFFLINE`, UI e API marcam todos os dados como "sem contato".

### F7 — Notificações, bot Telegram, boletim e relatório LLM (spec §3.7, §6.3–§6.5)
Avisa operadores sobre rede, comandos e regas com cooldown; mantém um bot de consulta; envia boletim meteorológico 4×/dia e um relatório diário narrado por LLM, com fallback numérico.

- **RF-NOT-01** Assinantes persistidos; chat principal sempre assinante.
- **RF-NOT-02** Bot responde `/start`, `/start convite`, `/id`, `/status`, `/tempo`/`/previsao`, `/ajuda` e desconhecido.
- **RF-NOT-03** Alertas de rede com cooldown e por transição; `factory_reset` sempre e suprime o offline seguinte; repareamento sem conclusão em 5 min alerta.
- **RF-NOT-04** Alertas de comando (caixa postal, confirmado, administrativo, `set_interval`, falha) com cooldowns.
- **RF-NOT-05** Mensagens de rega `INICIADA/FINALIZADA/RECONCILIADA` com canteiro, duração, modo e parâmetros do B.
- **RF-NOT-06** Boletim 06/11/16/21 h com dados Open-Meteo e recomendação por regra fixa.
- **RF-NOT-07** Relatório 08:00 com métricas 24 h do banco + D-1, narrado por LLM; sem LLM, sai só o numérico; teto de 70 000 caracteres.
- **RF-NOT-08** Toda notificação registrada com resultado; falha de envio não derruba o fluxo de origem.

### F8 — Interface web e API (spec §3.8, §6.2, §8)
Substitui o dashboard: controle manual com feedback por estado real, painel administrativo, gráficos de longo prazo, mapa e vigilância da rede, log MQTT, irrigação e comandos, tudo autenticado e em tempo real. API REST documentada como contrato externo.

- **RF-UI-01** Botões ligar/desligar por atuador; peristálticas com confirmação; cor pelo estado do comando, inicial pelo estado persistido.
- **RF-UI-02** Painel admin por nó (`REPRESENT`, `FORCE_UPDATE`, `DEBUG_NET`, `SET_INTERVAL`) e "reapresentar toda a rede" como um comando por nó.
- **RF-UI-03** Gráficos de 7 dias com zoom: solo A e B, clima Nó 4 e Nó 99, bateria, vazão e volume, séries pelo rótulo do child.
- **RF-UI-04** Mapa da rede: gateway e cards por nó com children e último valor formatado.
- **RF-UI-05** Tabela de vigilância com idades de contato/heartbeat/ACK e timeout aplicado.
- **RF-UI-06** Log MQTT com filtro e exportação CSV; "limpar" só limpa a visão.
- **RF-UI-07** Irrigação: runs com contagem regressiva e parada, histórico com volume, última avaliação do B, parâmetros editáveis com trilha.
- **RF-UI-08** Lista de comandos com estado, tentativas, origem e o eco que confirmou.
- **RF-API-01** API REST autenticada por token e documentada (endpoints mínimos em spec §6.2).
- **RF-API-02** Fluxo em tempo real de leituras, comandos, atuadores e status.
- **RF-API-03** Health com broker, última mensagem, agendador, banco, chave de confirmação e versão.

### F9 — Provisionamento de propriedade (spec §15)
Leva uma propriedade nova do "gateway ligado" a "regras habilitadas" sem editar arquivo nem recompilar: descoberta automática, cadastro, estados da instalação e checklist de comissionamento com evidência. Inclui o canal de rádio por propriedade e o factory reset do gateway.

- **RF-PRV-01** Cadastro de instalação por API/UI com atuadores e canteiros; YAML só como semente/teste.
- **RF-PRV-02** Prefixo duplicado recusado (`409`) com o nome da dona.
- **RF-PRV-03** Tráfego em prefixo desconhecido cria instalação `PENDENTE` e alerta; ingestão ligada, comandos/regras/notificações desligados.
- **RF-PRV-04** Confirmação do operador → `COMISSIONANDO`: comandos manuais sim, regras não.
- **RF-PRV-05** `ATIVA` só com checklist completo; regras só em `ATIVA`.
- **RF-PRV-06** `SUSPENSA`: ingere, silencia offline, recusa comandos.
- **RF-PRV-07** Credencial MQTT por instalação com ACL restrita ao prefixo; backend usa credencial de serviço.
- **RF-PRV-08** Coordenadas e fuso por instalação governam Open-Meteo, boletim, cron, soak e exibição.
- **RF-PRV-09** Factory reset do gateway (firmware já implementado); backend marca `RESETADO`, alerta e não trata o silêncio como falha.
- **RF-PRV-10** Chat principal e assinantes por instalação; convite vinculado à instalação.
- **RF-PRV-11** Mudança de estado da instalação e de atuadores/canteiros auditada com usuário e horário.
- **RF-PRV-12** Canal de rádio por instalação; cadastro recusa mesmo canal a < 10 km, com força administrativa justificada.

Checklist C1–C12 (spec §15.4): gateway online, ACL, nós apresentados, children conferem, rádio isolado, comando por atuador, preempção MUX, vazão, failsafe (dispensável com marca `failsafe_nao_verificado`), Telegram, Open-Meteo, caixa postal LP.

### F10 — Multi-instalação (spec §2.1, §3.1, §15)
Uma instância atende várias propriedades (prefixos `m360/{UF}/{CAR}`), isolando registro, comandos, regras e alertas. Nenhum prefixo hardcoded.

- **RF-ING-09** Várias instalações na mesma instância com isolamento total; prefixo desconhecido vira `PENDENTE`.
- (cruzados: RF-REG-13, RF-PRV-02/03/07/10/12; cenário de aceite 14)

### Fora do escopo do backend (spec §1)
Firmware de nós e gateway, placas, Node-RED do Kit Hélio. Requisitos de firmware derivados (canal no portal, `radioChannel` no heartbeat, `SET_RADIO:<k>`) são rastreados na spec §15.6, não implementados pelo backend.

---

## 3. Requisitos não funcionais (spec §4)

| ID | Resumo | Critério verificável |
|---|---|---|
| RNF-01 Durabilidade | Ação futura é linha persistida; matar o processo não perde nem duplica ação | Rega 60 s, kill em 20 s, volta em 70 s → 1 OFF, run `late_by_reconciler` |
| RNF-02 Persistência | Leituras, registro, comandos, runs, contadores, soak, assinantes, D-1 em banco | Reinício não altera volume acumulado nem `last_irrigation_at` |
| RNF-03 Idempotência | Reexecutar ação já executada não publica de novo | Chave única + checagem de `EXECUTADA` |
| RNF-04 Configuração | Tudo configurável fora do código; segredo obrigatório ausente impede o start | Start sem `TELEGRAM_BOT_TOKEN` falha com mensagem clara |
| RNF-05 Observabilidade | Log estruturado correlacionado por comando; métricas expostas | Comando falho rastreável em ≤ 3 consultas |
| RNF-06 Testabilidade | Testes contra broker local e simulador de nó | CI sem hardware cobre comandos, irrigação e volume |
| RNF-07 Fuso e relógio | UTC persistido, Brasília na exibição e agenda; relógio do gateway nunca usado | Cron 07:00 dispara 07:00 BRT em host UTC |
| RNF-08 Segurança | UI/API autenticadas; segredos fora do repo; broker autenticado | Varredura do repo sem token; `/api/*` sem token → 401 |
| RNF-09 Desempenho | < 1 msg/s, ~1 M leituras/ano/instalação | Comando→publish < 100 ms; séries 7 d < 1 s (simulador, 5 nós) |
| RNF-10 Disponibilidade | Reconexão com backoff e sessão persistente | Broker fora 30 s não perde confirmação em voo |
| RNF-11 Convivência | Roda em paralelo ao Node-RED só consumindo | Flag somente-leitura |
| RNF-12 Retenção | Log 30 d; leituras sem limite; avaliações 90 d | Job diário de expurgo |

---

## 4. Atores e jornadas

### Atores (inferidos; a spec só define um papel `admin` — D-06)
| Ator | Papel | Onde aparece |
|---|---|---|
| **Operador** da propriedade | Acompanha gráficos, mapa, vigilância; liga/desliga atuadores; rega manual; confirma instalação descoberta | spec RF-UI-01..08, RF-PRV-04 |
| **Administrador** | Cadastra instalações, edita parâmetros de regra, força canal em conflito, dispensa C9, gerencia tokens e chave de confirmação | spec RF-PRV-01/12, §15.4, RF-CMD-13, D-06 |
| **Assinante do Telegram** | Recebe alertas, mensagens de rega, boletim e relatório; consulta `/status` e `/tempo` | spec RF-NOT-01..07, RF-PRV-10 |
| **Instalador no comissionamento** | No campo: configura gateway pelo portal AP, liga nós, usa o reed para forçar janela LP/repareamento, executa itens manuais do checklist | spec §15.1, §15.4, RF-CMD-09 |
| **Sistemas externos** | Gateway/broker MQTT, Open-Meteo, LLM (Gemini), Telegram Bot API | spec §6 |
| (implícito) **Pesquisador/agrônomo** | Compara parcela A (controle) com B (experimental) | spec RF-IRR-03/04 — não nomeado como ator |

### Jornadas implicadas
1. **Comissionar propriedade** (spec §15.1, §15.4, cenários 16–23): escritório reserva UF/CAR e credencial com ACL → instalador põe gateway em AP, grava WiFi/MQTT/UF/CAR/canal → tráfego aparece → backend cria `PENDENTE` e alerta → operador confirma nome, fuso, coordenadas, atuadores, canteiros, Telegram → `COMISSIONANDO` → checklist C1–C12 com evidência → `ATIVA` → regras habilitadas.
2. **Rega manual** (spec RF-IRR-08, RF-UI-01/07, §5.3, cenários 2–4): operador escolhe canteiro e duração → limite aplicado → comando `atuar(…,1)` (âmbar) → eco confirma (verde) → OFF agendado → contagem regressiva com botão parar → OFF confirmado → Telegram `FINALIZADA` com volume.
3. **Receber alerta** (spec RF-WDG-01/02, RF-NOT-03/04, cenários 3, 10, 21): gateway/nó cai ou comando falha → transição detectada → alerta com cooldown e motivo (timeout aplicado, RSSI, horário) → volta → alerta de reconexão; em `SUSPENSA` silencia; `factory_reset` suprime o offline.
4. **Relatório diário** (spec RF-NOT-07, §6.5): 08:00 → métricas 24 h do banco + snapshot D-1 → LLM escreve narrativa → Telegram a assinantes; LLM falha → numérico puro.
5. (adicionais) **Boletim 4×/dia** (RF-NOT-06), **reparear nó ACT pelo reed** (RF-ING-10, cenário 15a), **reaproveitar gateway** via factory reset (RF-PRV-09, cenário 22), **comando a nó LP dormindo** (RF-CMD-09, cenário 7).

---

## 5. Plano de migração (spec §10)

Pré-requisito: Nó 99 e gateway com failsafe gravados (já em produção). Regra de ouro: nunca dois sistemas comandando 31–37 ao mesmo tempo.

| Fase | Entrega | Node-RED | Critério de saída |
|---|---|---|---|
| 0 | Infra, banco, broker de teste, simulador, CI | ligado | Simulador reproduz eco, preempção, failsafe e sono; testes verdes |
| 1 | Ingestor + persistência + registro, somente leitura | ligado | 7 dias em paralelo: contagem = Central Logger; status = watchdog do Node-RED |
| 2 | API, SSE, UI de leitura | ligado | Dashboard do backend usado no dia a dia; gráficos conferem |
| 3 | Despachante, botões manuais, painel admin | botões desativados | 20 comandos manuais confirmados por eco; falha simulada → `FALHOU` em 35 s com alerta |
| 4 | Agendador durável, Canteiro A, reconciliador | aba Irrigação desligada antes | 3 dias de regas A com OFF confirmado; teste de kill no meio da rega |
| 5 | Motor B, Open-Meteo, soak | idem | 7 dias de avaliações registradas, conferidas com o histórico do Node-RED |
| 6 | Telegram (alertas, bot, boletim, relatório) | abas Telegram/IA desativadas | Mensagens equivalentes; assinantes migrados |
| 7 | Corte; backup final; `nr_deploy.py`/DEPLOY.md viram legado | desligado | 14 dias sem regressão; rollback documentado (religar NR + backend somente leitura) |

Migração única de dados (spec §10): `mys_nodes`, contador de volume `flow_vol_node_99_21`, assinantes, snapshot D-1, opcionalmente 7 dias de `hist_*`; log MQTT só como CSV.

---

## 6. Decisões (spec §13)

| ID | Tema | Situação |
|---|---|---|
| D-01 | Linguagem/runtime (rec.: Python 3.12 asyncio/FastAPI; alt.: Node/TS) | **Aberta** (recomendação) |
| D-02 | Banco (rec.: PostgreSQL 16 particionado; alt.: SQLite) | **Aberta** |
| D-03 | UI (rec.: servida pelo backend + HTMX/Vue leve; alt.: SPA ou Grafana) | **Aberta** |
| D-04 | Fonte de clima do Canteiro B = Nó 4 (11/12), `max_age 3600 s`, fallback Open-Meteo | **Decidida** 2026-10-17 |
| D-05 | Hospedagem (rec.: mesmo servidor viridiotech, containers) | **Aberta** |
| D-06 | Autenticação (rec.: usuário/senha local + tokens, um papel `admin`; alt.: OIDC) | **Aberta** |
| D-07 | Peristálticas na UI (rec.: atrás de confirmação; alt.: omitir) | **Aberta** (já refletida em RF-UI-01 como requisito) |
| D-08 | Kit Hélio (rec.: 2ª instalação, só leitura; alt.: instância separada) | **Aberta** |
| D-09 | Separação de rádio: grade de 64 canais de 200 kHz em 915–928 MHz | **Decidida** (texto em negrito como decisão; base medida 2026-09-10) |

Spec diz "confirmar antes da Fase 0" — 7 de 9 ainda pendentes.

---

## 7. Detalhe técnico para addendum/arquitetura (NÃO para o PRD)

- Topologia e broker atual (host, porta, sem TLS) — spec §2.1
- Tabela de tópicos MQTT consumidos e campos do heartbeat/status/eventos — spec §2.2
- Formato do tópico de comando e limite de 25 bytes; validação espelhada do gateway — spec §2.3
- Nota técnica do broadcast de reapresentação que não funciona (MySensors 2.3.2) — spec §2.3
- Tabela de restrições de firmware (atoi, MUX, failsafe, LP, reed, uptime) — spec §2.4 (consequências podem virar premissas no PRD; a mecânica vai para arquitetura)
- Diagrama de componentes e "processo único asyncio" — spec §5.1
- Esquema relacional (tabelas e colunas) — spec §5.2; esboço antigo em guia §9.2
- Máquina de estados do comando (transições exatas) — spec §5.3; guia §9.1
- Agendador (loop 1 s, `FOR UPDATE SKIP LOCKED`, tipos de ação, `dedupe_key`) — spec §5.4
- Eventos de domínio internos — spec §5.5
- YAML de configuração e variáveis de ambiente — spec §5.6
- Lista de endpoints REST — spec §6.2 (no PRD, só "API para X")
- Parâmetros da chamada Open-Meteo e campos do snapshot — spec §6.4
- Interface `ReportWriter` e modelo Gemini — spec §6.5
- Pseudocódigo da confirmação — spec §7.1; CLAUDE §Sincronizador ACK
- Fórmulas do motor B (soak, travas, F_atmo, lâmina, duração) — spec §7.3; tabela de parâmetros spec §9.3
- Tabelas de failsafe, escalas e parâmetros de comandos/vigilância/cooldowns — spec §9.1, §9.2, §9.4
- Docker compose, proxy TLS, Alembic, backup `pg_dump`, local `backend/` no monorepo — spec §11
- Especificação do simulador de nó — spec §12.1
- Grade de canais RFM95W e fórmulas de frequência; requisitos de firmware derivados — spec §15.6
- Mapeamento de migração de contexto Node-RED → tabelas — spec §10
- Procedimento de deploy do Node-RED (legado) — DEPLOY §1–§5
- Armadilhas de integração (ACK de transporte, `getSender()`, `V_STATUS`, MAX_RT, canal 76, `V_LEVEL`) — CLAUDE §Armadilhas Críticas

---

## 8. Métricas de sucesso e contra-métricas

### Métricas implícitas (derivadas dos cenários de aceite e critérios de fase)
| Métrica | Alvo | Origem |
|---|---|---|
| Comandos de atuação com payload fora de `"0"/"1"` | 0 | spec §12.2 cen. 1 |
| Taxa de comandos manuais confirmados pelo eco na fase de corte | 20/20 | spec §10 fase 3 |
| Tempo até `FALHOU` em falha de última milha | ≈ 35 s, alerta único | spec cen. 3; fase 3 |
| Regas com OFF perdido ou duplicado após restart | 0 | spec RNF-01, cen. 6 |
| Regas A executadas no horário com OFF confirmado | 100 % por 3 dias | spec fase 4 |
| Avaliações do motor B registradas (1 a cada 5 min) | 288/dia por 7 dias | spec RF-IRR-04, fase 5 |
| Paridade de contagem de mensagens com o Central Logger | igual em 7 dias | spec fase 1 |
| Entrega da caixa postal após despertar do nó LP | < 500 ms | spec cen. 7 |
| Latência comando→publish / consulta 7 dias | < 100 ms / < 1 s | spec RNF-09 |
| Volume acumulado preservado após reinício | 100 % | spec RNF-02, cen. 13 |
| Leituras `NaN` no banco após `REPRESENT` | 0 | spec cen. 12 |
| Vazamento entre instalações (nós/comandos/alertas) | 0 | spec cen. 14 |
| Dias sem regressão após desligar o Node-RED | 14 | spec fase 7 |
| Rastreabilidade de comando falho | ≤ 3 consultas | spec RNF-05 |
| Tempo para comissionar propriedade sem editar arquivo/recompilar | não quantificado | spec §15 (lacuna) |

### Contra-métricas plausíveis (a vigiar)
- **Falsos alarmes / fadiga de alerta** no Telegram (alertas por dia por instalação), dado o número de tipos de alerta e cooldowns curtos (3 s para `CONFIRMADO`).
- **Regas bloqueadas indevidamente** pelo motor B (soak, travas, clima indisponível) → stress hídrico; medir umidade de solo mediana após dias com `STANDBY_*`.
- **Comandos `EXPIRADO`/`FALHOU`** crescendo com a caixa postal FIFO (24 h de fila pode aplicar comando velho).
- **Backstop do reconciliador disparando** (`RECONCILIADA`): indica failsafe do firmware que não atuou — deveria ser ~0.
- **Custo e falha do LLM** (relatórios só numéricos por indisponibilidade).
- **Fricção do comissionamento**: itens do checklist dispensados (C9) ou instalações paradas em `COMISSIONANDO`.
- **Divergência inventário ↔ configuração do backend** (comandos rejeitados por child desconhecido).
- **Crescimento do banco** (`reading` sem limite de retenção).

---

## 9. Lacunas e perguntas para o PM

1. **Ainda é proposta**: spec não aprovada; 7 decisões (D-01..03, D-05..08) abertas e "a confirmar antes da Fase 0". Quem aprova e quando? (spec cabeçalho, §13)
2. **Papéis e permissões**: D-06 prevê um só papel `admin`, mas RF-PRV-10 fala em "operadores" por instalação e RF-UI-07/RF-PRV-11 exigem trilha de "quem alterou". Há operador sem privilégio de admin? Operador de uma propriedade vê as outras?
3. **Telegram por instalação vs. segredo único**: RF-PRV-10 quer chat principal por instalação, mas §5.6 exige um único `TELEGRAM_CHAT_ID` em env. Qual é o "chat administrativo" que recebe o alerta de `PENDENTE` (RF-PRV-03)? `/status` responde sobre qual instalação para um assinante de várias?
4. **Fonte da configuração**: RF-PRV-01 diz que em produção a fonte é o banco; §5.6 e RNF-04 tratam o YAML como fonte validada no startup. Qual vence em conflito? Edição de parâmetros pela UI persiste onde?
5. **Objetivo do experimento A × B**: Canteiro A é "controle", B é "experimental", mas não há requisito de comparação (relatório A vs B, volume por canteiro, produtividade). O sucesso do produto inclui essa comparação?
6. **Canteiro C (child 33) e peristálticas (34–37)**: configurados como atuadores, mas sem regra; dosagem de pH/suplemento não tem regra nem limite de volume — fica só manual para sempre?
7. **Vazão compartilhada**: A e B usam o mesmo `flow_child: 21` (spec §5.6). Atribuição de volume por canteiro depende da exclusão do MUX; rega manual de C simultânea a bomba NFT? Confirmar que o hidrômetro mede só o ramo das solenóides.
8. **Kit Hélio**: D-08 diz "só leitura" na mesma instância; RF-REG-13 já o inclui. Ele entra no escopo do MVP ou depois? Fase de migração dele não está no plano §10.
9. **Disponibilidade/recuperação**: não há SLO, RPO/RTO, nem o que fazer se o host cair durante uma rega (só o failsafe do firmware cobre). Backup diário é suficiente?
10. **Retenção de leituras** "sem limite inicial" (RNF-12): até quando? Custo de armazenamento com N instalações?
11. **Limites de escala**: RNF-09 mede 5 nós e < 1 msg/s; quantas instalações a instância única deve suportar? (D-09 fala em reuso geográfico "indefinido")
12. **Dados pessoais**: chat_id, nomes de assinantes, coordenadas e CAR das propriedades — política de privacidade/LGPD não é tratada.
13. **Custo/escolha do LLM**: provedor, custo mensal aceitável e conteúdo enviado (logs até 70 000 caracteres) não têm critério de aceite além do fallback.
14. **Mobile/UX**: §8 diz "responsiva", sem requisitos de uso em campo (conectividade ruim, PWA, idioma).
15. **Rollback de fase**: só a Fase 7 tem rollback documentado; o que fazer se a Fase 4/5 falhar em produção (religar aba Irrigação?) não é especificado.
16. **Guia desatualizado**: guia §7.2 ainda descreve o motor B antigo (limiares 15/45 s, clima do Nó 99) — diverge da spec §7.3 e de D-04. Precisa ser marcado como obsoleto para não contaminar o PRD.
17. **Métrica de comissionamento**: §15 quer "não depender de editar arquivo nem recompilar", mas não fixa tempo-alvo nem quem executa cada passo (escritório vs. campo); credencial MQTT por instalação depende de ACL no broker — quem a administra?
18. **Notificação de alertas de motor B**: `SEM_LEITURA` "alerta se persistir por 3 avaliações" (spec §7.3) não tem RF-NOT correspondente nem cooldown.
