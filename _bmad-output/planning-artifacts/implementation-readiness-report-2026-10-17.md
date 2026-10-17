---
stepsCompleted: [step-01-document-discovery, step-02-prd-analysis, step-03-epic-coverage-validation, step-04-ux-alignment, step-05-epic-quality-review, step-06-final-assessment]
mode: autonomo (rascunho; nenhum menu apresentado ao usuário; todo "C" tratado como selecionado)
filesIncluded:
  prd:
    - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/prd.md
    - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/addendum.md
  architecture:
    - _bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-10-17/ARCHITECTURE-SPINE.md
    - docs/especificacao-backend-m360.md (v1.3, companheira normativa)
  epics:
    - _bmad-output/planning-artifacts/epics.md
  ux: []
  contexto_brownfield:
    - project-context.md
    - src/DRY/horta/inventario.md
    - hardware/ARQUITETURA_FAMILIA.md
---

# Implementation Readiness Assessment Report

**Date:** 2026-10-17
**Project:** m360_horta
**Avaliador:** agente PM (workflow `bmad-check-implementation-readiness`, modo autônomo)
**Escopo julgado:** épico 3 (histórias 3.5 a 3.11), épico 4 e, com menos profundidade, épicos 5 a 11. Épicos 1 e 2 e as histórias 3.1 a 3.4 estão entregues e não foram reavaliados.

> **Modo autônomo.** O usuário escolheu um rascunho autônomo e não estava disponível. Toda pergunta que o workflow faria virou item aberto na seção "Questões abertas para o Marcelo", no fim do relatório. Nenhum documento de entrada foi editado.

---

## 1. Descoberta de documentos

### PRD

**Documentos inteiros:** nenhum `planning-artifacts/*prd*.md` na raiz. Os PRDs de junho (`planning-artifacts/prd.md`, `docs/prd.md`) estão apagados no Git (status `D`) e o PRD novo os substitui (`supersedes`).

**Pasta de execução (usada):** `prds/prd-m360_horta-2026-10-17/`
- `prd.md` (39,8 KB, 17/10 12:35) — **documento avaliado**
- `addendum.md` (7,5 KB) — **documento avaliado**
- `extract-backend.md`, `extract-firmware-hardware.md`, `extract-produto-campo.md`, `reconcile-fontes.md`, `review-rubric.md`, `.memlog.md` — insumos e revisões do processo de PRD; não normativos, consultados só pontualmente.

### Arquitetura

**Pasta de execução (usada):** `architecture/architecture-m360_horta-2026-10-17/`
- `ARCHITECTURE-SPINE.md` (27,6 KB) — **documento avaliado** (AD-1 a AD-22)
- `review-adversarial.md`, `review-rubric.md`, `.memlog.md` — revisões; consultadas pontualmente.

**Companheira normativa:** `docs/especificacao-backend-m360.md` v1.3 (77,6 KB) — dona dos RF-*/RNF-*, do esquema, da máquina de estados e do plano de migração. O spine declara que ela precisa de ajustes antes da fase 0.

### Épicos e histórias

- `planning-artifacts/epics.md` (40,5 KB, 17/10 12:37) — **documento avaliado**, 11 épicos.
- Os arquivos antigos `2-1-…md` e `2-2-…md` estão apagados no Git e substituídos.

### UX

- **Nenhum documento de UX.** Os requisitos de interface estão na spec do backend §3.8 (RF-UI-01..08, RF-API-01..03) e §8.

### Problemas da descoberta

- **Duplicatas:** nenhuma. Os documentos antigos já estão apagados e marcados como substituídos.
- **Ausente:** documento de UX (avaliado na §4 deste relatório).
- **Observação:** `_bmad-output/implementation-artifacts/` não tem `sprint-status.yaml` para os épicos novos (só o `final-sprint-status.md` legado). O sprint planning ainda não rodou sobre este `epics.md`.

---

## 2. Análise do PRD

### 2.1 Requisitos funcionais extraídos

Texto do PRD §4, sem a linha de consequência (que foi usada na avaliação dos critérios de aceite).

| FR | Requisito | Estado declarado |
|---|---|---|
| FR-1 | Cada canteiro tem um nó com seis sondas (três distâncias, duas profundidades); lê a cada ciclo e publica o bruto quando muda e no mínimo a cada 10 ciclos | Produção / LP sem bancada |
| FR-2 | Sondas só recebem energia durante a leitura | idem |
| FR-3 | A plataforma exibe a umidade de forma legível sem perder o bruto; conversão derivada de `S_MOISTURE` | Especificado |
| FR-4 | Nó solar fora da estufa mede T/UR do ar e T do solo a cada 30 min; é a fonte de clima do motor B, do relatório e do `/status` | Produção |
| FR-5 | Sem clima recente (> 1 h), o motor usa a previsão como reserva e registra | Novo |
| FR-6 | O nó atuador mede a vazão sem perder pulsos, em L/min | Produção |
| FR-7 | Volume acumulado persistente, sem inventar consumo em lacunas, atribuído por rega | Especificado |
| FR-8 | Atuador comanda até 9 cargas (3 solenóides, 4 peristálticas, 2 bombas) | Produção / ACT sem bancada |
| FR-9 (P0) | Só um canal do MUX ligado por vez; o nó avisa o desligamento antes de confirmar a nova | idem |
| FR-10 (P0) | O nó desliga solenóides e peristálticas no tempo máximo, sem gateway; bombas NFT sem limite | idem (defeito aberto: rearme) |
| FR-11 (P0) | Nenhuma carga liga sozinha em reset/queda/falha | idem |
| FR-12 | Quatro perfis de energia, escolhidos na configuração do nó | Produção / bateria LP sem bancada |
| FR-13 | Nós a bateria reportam carga no 255; na LP, corte de TX abaixo de 3,0 V com bandgap | LP sem bancada |
| FR-14 | Placas M360 aceitam LoRa 915 MHz ou nRF24, um rádio por placa, na compilação | LoRa sem bancada |
| FR-15 | Nó atuador pode operar como repetidor | Produção (proto) |
| FR-16 | Isolamento entre vizinhos por canal de frequência, um por propriedade (versão seguinte) | Especificado |
| FR-17 | Gateway valida cada comando e publica o motivo de cada recusa | Produção |
| FR-18 | Sem config válida ou com ímã no boot, AP `M360-Config`; ímã desde o boot nunca apaga | Produção |
| FR-19 | Heartbeat periódico e eventos de nó descoberto/perdido/reconectado | Produção |
| FR-20 | LEDs indicam WiFi e MQTT | Produção |
| FR-21 | Ímã no nó sensor força leitura e acorda o nó | Implementado sem bancada |
| FR-22 | Ímã 3 s na ACT refaz a ligação sem reiniciar nem mexer nos relés | idem |
| FR-23 | Factory reset do gateway por ímã (30 s / 3 s) ou botão do portal; publica `factory_reset` com MQTT | idem |
| FR-24 | Todo comando é registro auditável com estados, origem e horário de cada transição | Especificado |
| FR-25 (P0) | Atuação só confirmada pelo eco do próprio nó com o valor pedido | Especificado |
| FR-26 | Sem eco, três tentativas, falha e alerta | Especificado |
| FR-27 | Fila por nó para nós que dormem, entregue no despertar, expira em 24 h | Novo |
| FR-28 | Mudar intervalo, forçar leitura, reapresentar e diagnosticar qualquer nó à distância | Firmware pronto / backend especificado |
| FR-29 | Canteiro A por horário fixo (07:00 e 17:00, 300 s) | Produção Node-RED |
| FR-30 | Motor do B a cada 5 min, cinco camadas, sobreposições ≥ 650/≥ 700, cada avaliação registrada | idem |
| FR-31 (P0) | Desligamento de toda rega agendado de forma durável | Especificado |
| FR-32 (P0) | Duração abaixo do failsafe e reconciliador desliga o que o agendador perder | Especificado |
| FR-33 | Rega manual pelo mesmo caminho (registro, limite, OFF durável, aviso); falha de abertura avisa no Telegram | Especificado |
| FR-34 (P0) | Regras ligáveis sem reimplantar; um canteiro nunca comandado por dois sistemas | Especificado |
| FR-35 | Ingestão de todo o tráfego num log auditável (30 dias) | Especificado |
| FR-36 | Lista de nós montada pela apresentação, sem lista fixa | Especificado |
| FR-37 | Critério único de online para painel, API e alertas | Especificado |
| FR-38 | Leituras sem limite, consulta de 7 dias < 1 s | Especificado |
| FR-39 | Alertas de rede, de comando e de rega uma vez por transição, com cooldown | Especificado |
| FR-40 | Bot de status/previsão com assinantes por convite | Especificado |
| FR-41 | Boletim 4×/dia e relatório às 08:00 (com reserva numérica sem LLM) | Especificado |
| FR-42 | Toda notificação registrada; falha de envio não interrompe a origem | Especificado |
| FR-43 | Botões por atuador com cor pelo estado real do comando; confirmação nas peristálticas | Especificado |
| FR-44 | Gráficos 7 dias, mapa, vigilância, log com CSV, regas em andamento e histórico, lista de comandos (paridade RF-UI-02..08) | Especificado |
| FR-45 | Parâmetros editáveis no painel com trilha | Especificado |
| FR-46 | API autenticada e documentada (leituras, comandos, regas, estado, tempo real) | Especificado |
| FR-47 | Painel e API exigem autenticação (401 sem token) | Especificado |
| FR-48 | Litros por canteiro por dia e por ciclo | Novo |
| FR-49 | Registro de ciclo, perdas, colheita; cálculo de kg/L, Índice e economia | Novo |
| FR-50 | Exportação CSV por tabela, filtrada por ciclo, em UTC | Novo |
| FR-51 | Várias propriedades com isolamento total | E3 |
| FR-52 | Gateway desconhecido cria instalação pendente que não comanda nem alerta | E3 |
| FR-53 | Estados pendente/comissionando/ativa/suspensa e resetado; regras só em ativa após checklist | E3 |
| FR-54 | Credencial MQTT, coordenadas, fuso, assinantes e canal por instalação | E3 |
| FR-55 | Canal no portal e no heartbeat; nós trocam por comando, confirmando antes | E3 |
| FR-56 | Portal do gateway autenticado | E3 |
| FR-57 | Papéis por instalação | E3 |
| FR-58 | Pareamento guiado com ID e perfil do backend | Futuro |
| FR-59 | Perfil do nó na memória, alterável à distância | Futuro |
| FR-60 | Realocação por comando remoto em child administrativo próprio | Futuro |
| FR-61 | Nunca dois comandos em voo no MUX do Nó 99; eco de OFF não pedido encerra a rega ativa | Especificado |

**Total de FRs: 61** (FR-1 a FR-61, contínuos; FR-61 fica em F9).

### 2.2 Requisitos não funcionais extraídos

| NFR | Requisito | Fonte |
|---|---|---|
| NFR-1 | Durabilidade: nenhuma ação futura só em memória | PRD §8.1 / RNF-01 |
| NFR-2 | Persistência: reinício não altera contadores nem estado | RNF-02 |
| NFR-3 | Idempotência e reconexão: reexecutar não publica de novo; queda do broker não perde confirmação em voo | RNF-03, RNF-10 |
| NFR-4 | Segredo obrigatório ausente impede o start | RNF-04 |
| NFR-5 | Comando falho rastreável em ≤ 3 consultas; log estruturado e métricas | RNF-05 |
| NFR-6 | Simulador + CI sem hardware (critério de saída da fase 0) | RNF-06, §12.1 |
| NFR-7 | UTC persistido; agenda e exibição em Brasília | RNF-07 |
| NFR-8 | Painel, API e broker autenticados; segredos fora do repo | RNF-08 |
| NFR-9 | Comando→publish < 100 ms; séries de 7 dias < 1 s | RNF-09 |
| NFR-10 | Convivência só-leitura com o Node-RED até o corte | RNF-11 |
| NFR-11 | Retenção: log 30 d, leituras sem limite, avaliações 90 d | RNF-12 |
| NFR-12 | Backup diário do banco | PRD §8.1, spec §11 |
| NFR-13 | Degradação sem internet: recarregar estado de atuadores e marcar incerto após reboot do gateway/Nó 99 | PRD §8.1, RF-CMD-12 |
| NFR-14 | Firmware: ≥ 512 B de RAM livre; interrupção de vazão nunca bloqueada | PRD §8.2, AD-20 |
| NFR-15 | Fail-safe elétrico, proteção de campo, bateria só via módulo protegido, 915 MHz | PRD §8.2 |
| NFR-16 | Instalação: 5,0 V travados, USB e borne nunca juntos, FTDI sem VCC | PRD §8.2 |
| NFR-17 | Ambiente (IP, temperatura, autonomia, alcance) **não definido** (Q-5, Q-6) | PRD §8.2 |

**Total de NFRs: 17.** O `epics.md` lista 13 (NFR-1 a NFR-13 com outra numeração) e **omite** o backup diário (NFR-12 aqui), a degradação/reconciliação de startup (NFR-13 aqui) e as regras elétricas e de proteção (NFR-15).

### 2.3 Requisitos adicionais e restrições

- **Contrato MQTT** = `inventario.md`; payload de relé só `"0"`/`"1"` (PRD §8.3).
- **Congelamento do ciclo medido** sobre E1 + E2 (PRD §6, P-9).
- **Prioridade P0:** FR-9, 10, 11, 25, 31, 32, 34; nenhuma entrega pode regredir neles.
- **Bloqueio declarado:** Q-10 (D-01, 02, 03, 05, 06, 07, 08) **bloqueia a fase 0 da E2**. Q-7 bloqueia o pedido de RFM95W em volume.
- **Spine:** 22 ADs; sete ajustes pendentes na spec v1.3 antes da fase 0.

### 2.4 Avaliação de completude do PRD

O PRD é sólido: FRs numerados com consequência verificável, estado por funcionalidade, P0 explícito, não objetivos, métricas com contra-métricas, premissas e questões com dono. Pontos fracos para a implementação:

1. **Q-10 é bloqueante e está aberta.** Toda a E2 depende dela.
2. **Q-13, Q-14, Q-15** (área do canteiro, A em dia de chuva, calibração de vazão) afetam FR-30, o Índice e FR-48. Não bloqueiam o código, mas bloqueiam o ciclo congelado.
3. **Q-2** (limiar de conclusão do Índice) deixa SM-1 sem alvo numérico.
4. O PRD não diz **qual rádio** a estufa usa no corte da E1, e o Nó 4 é não objetivo de migração (ver C2 abaixo).

---

## 3. Validação de cobertura dos épicos

### 3.1 Matriz de cobertura

Legenda: ✓ coberto com critério de aceite verificável; ◐ mapeado, mas o critério de aceite não cobre parte do FR; ✗ ausente.

| FR | Épico / história | Status |
|---|---|---|
| FR-1 | 1.2; placas: 3.7 | ✓ |
| FR-2 | 1.2; 3.7 (VSW) | ✓ |
| FR-3 | 4.2 (bruto + convertido) | ✓ (a derivação por `S_MOISTURE` não está no AC; menor) |
| FR-4 | 1.3, 2.3; backend 6.4 | ✓ (ameaçado pelo corte da 3.10, ver C2) |
| FR-5 | 6.4 | ✓ |
| FR-6 | 1.4; 3.8 | ✓ |
| FR-7 | 4.5 (acumulado), 6.7 (por rega) | ✓ |
| FR-8 | 1.4; 3.8 | ✓ |
| FR-9 | 1.4; 3.8 (preempção); 5.3 | ✓ |
| FR-10 | 1.4; 3.8; 3.11 | ✓ |
| FR-11 | 3.8 | ✓ |
| FR-12 | 1.x; 3.7 (sono) | ◐ perfis bateria e repetidor nas placas sem AC de bancada |
| FR-13 | 3.7 | ✓ |
| FR-14 | 1.x (nRF24); 3.6, 3.8, 3.9 | ✓ |
| FR-15 | 1.x (proto); "3" no mapa | ◐ nenhuma história do épico 3 testa `act_99_rep` |
| FR-16 | 9.5, 10.2 | ✓ |
| FR-17 | 1.5 | ✓ (sem AC de bancada na GW; mesmo fonte) |
| FR-18 | 1.5; 3.9 | ✓ |
| FR-19 | 1.5; 3.10 implícito | ✓ |
| FR-20 | 3.9 | ✓ |
| FR-21 | 3.4; 3.7 | ✓ |
| FR-22 | 3.4; 3.8 | ✓ |
| FR-23 | 3.4; 3.9 | ✓ |
| FR-24 | 5.1, 5.4 | ✓ |
| FR-25 | 5.1 | ✓ |
| FR-26 | 5.2, 5.6 | ✓ (o "alerta" depende do épico 7, ver M5) |
| FR-27 | 5.4 | ✓ |
| FR-28 | 5.5 | ✓ |
| FR-29 | 6.3 | ✓ |
| FR-30 | 6.4 | ✓ |
| FR-31 | 6.1, 6.5 | ✓ |
| FR-32 | 6.1, 6.2 | ✓ |
| FR-33 | 6.2 | ◐ o aviso no Telegram da falha de abertura não está em nenhum AC |
| FR-34 | 6.5, 6.6 | ✓ |
| FR-35 | 4.2 | ✓ |
| FR-36 | 4.3 | ✓ |
| FR-37 | 4.4 | ✓ |
| FR-38 | 4.6 | ✓ |
| FR-39 | 7.1 | ◐ alertas de **comando** (`FALHOU`, caixa postal; RF-NOT-04) ausentes do AC |
| FR-40 | 7.2 | ✓ |
| FR-41 | 7.3 | ✓ |
| FR-42 | 7.3 | ✓ |
| FR-43 | 5.6 (bombas), 6.2 (MUX) | ✓ |
| FR-44 | 4.6, 6.2 | ◐ lista de comandos (RF-UI-08), "reapresentar toda a rede" (RF-UI-02), gráficos de bateria/vazão e o checklist de paridade §12.3 não estão em nenhum AC |
| FR-45 | 6.6 | ✓ |
| FR-46 | 4.6 (leitura) | ◐ API de comandos e documentação OpenAPI sem AC |
| FR-47 | 4.6 | ✓ |
| FR-48 | 6.7, 8.1 | ✓ |
| FR-49 | 8.1 | ✓ |
| FR-50 | 8.2 | ✓ |
| FR-51 | 9.1 | ✓ |
| FR-52 | 9.2 | ✓ |
| FR-53 | 9.2, 9.3 | ✓ |
| FR-54 | 9.4, 9.5 | ✓ |
| FR-55 | 10.2, 10.3 | ✓ |
| FR-56 | 10.4 | ✓ |
| FR-57 | 9.6 | ✓ |
| FR-58 | 11.1 | ✓ (futuro) |
| FR-59 | 11.2 | ✓ (futuro) |
| FR-60 | 11.3 | ✓ (futuro) |
| FR-61 | 5.3 | ◐ a segunda metade (eco de OFF não pedido **encerra a rega ativa**, spec §12.2 cenário 5) não tem AC no épico 6 |

### 3.2 Requisitos sem cobertura

**Nenhum FR está ausente do mapa.** A afirmação do `epics.md` ("todos os FRs de FR-1 a FR-61 estão no mapa e em pelo menos uma história") é verdadeira para o mapa, mas **sete FRs têm cobertura parcial** nos critérios de aceite: FR-12, FR-15, FR-33, FR-39, FR-44, FR-46 e FR-61.

**NFRs sem história:**

- **Backup diário do banco** (PRD §8.1, spec §11): nenhuma história. Deveria estar no épico 4, porque os dados começam a acumular na fase 1.
- **Reconciliação de estado no startup / atuador incerto** (PRD §8.1, RF-CMD-12): nenhuma história. A 5.3 recusa ON para child "incerto", mas nenhuma história cria o estado incerto.
- **Reconexão ao broker sem perder confirmação em voo** (RNF-10): nenhum AC.
- **Comando→publish < 100 ms** (RNF-09), `/metrics` e log estruturado (RNF-05), expurgo de `rule_evaluation` em 90 dias (RNF-12), chave `confirmacao_habilitada` (RF-CMD-13): nenhum AC.

### 3.3 Estatísticas

- FRs no PRD: **61**
- FRs mapeados para épicos: **61 (100 %)**
- FRs com AC que cobre o requisito inteiro: **54 (88,5 %)**
- FRs com cobertura parcial: **7**
- NFRs do PRD sem história: **2 com peso de produto** (backup, reconciliação de startup) e 4 secundários

---

## 4. Alinhamento de UX

### Estado do documento de UX

**Não encontrado.** O `epics.md` declara: "Não há documento de UX: a interface do backend segue a spec §8 e os RF-UI."

### A interface está implícita?

Sim. O PRD tem um painel web (F13, UJ-3) com botões de cor por estado, contagem regressiva, confirmação de peristálticas, gráficos, mapa, vigilância, log, lista de comandos, painel admin e edição de parâmetros. Há ainda o canal Telegram (F12) e o portal do gateway (F7, já implementado).

### A ausência é aceitável?

**Aceitável para o piloto, com ressalvas.** A spec §3.8 e §8 descrevem o **comportamento** da UI com precisão incomum: cores e transições dos botões, preempção visível sem clique, replay por SSE, fuso no rodapé, o que o botão "limpar" do log faz. O público é uma pessoa (o técnico, papel único `admin`) e o PRD trata o piloto como lançamento interno (P-1). O dashboard do Node-RED serve de referência de paridade.

### Avisos

1. **Layout e navegação não estão definidos** ("uma página por função"). Com D-03 em aberto (HTMX ou Vue), a primeira história de UI (4.6) vai decidir layout sem referência.
2. **"Responsiva" sem critério.** O PRD diz que no celular o canal é o Telegram, mas UJ-3 (rega manual com contagem regressiva) é um caso natural de celular. Não há AC de largura mínima.
3. **Paridade não verificada.** FR-44 promete cobrir a lista fechada RF-UI-02..08, mas nenhuma história usa o checklist da spec §12.3 como critério (ver M8).
4. **Telegram e mensagens** têm requisitos de texto na spec (RF-NOT-03..05), sem modelos de mensagem; o AC da 7.1 aceita "mensagens equivalentes".

**Recomendação:** não criar um documento de UX completo. Basta, antes da 4.6, uma página curta com o mapa de telas, a navegação e um critério de celular para a rega manual, ou capturas do dashboard do Node-RED anotadas como referência de paridade.

---

## 5. Revisão de qualidade dos épicos

### 5.1 Valor ao usuário e independência dos épicos

| Épico | Valor ao usuário | Independência | Observação |
|---|---|---|---|
| 3 | ✓ placas próprias, manutenção sem abrir a caixa | ✓ do backend | 3.10 tem um problema de escopo de rádio (C2) |
| 4 | ✓ histórico e vigilância confiáveis | ✗ precisa do agendador do épico 6 (C3) e de decisões externas (C1) | 4.1 é técnica e grande demais (M2) |
| 5 | ✓ comandos confirmados | ✗ retries e expiração precisam do agendador durável (6.1) e o alerta precisa do notificador (7) | M5, C3 |
| 6 | ✓ irrigação durável | ◐ alertas de rega dependem do épico 7; janela sem avisos de rega após a 6.5 | M6 |
| 7 | ✓ | ✓ | — |
| 8 | ✓ | ✓ (depende de 4–7, para trás) | — |
| 9 | ✓ | ✗ 9.2 e 9.5 dependem de 10.1 e 10.2 | M11 |
| 10 | ✓ | ✓ | — |
| 11 | ✓ (futuro) | depende de 9 e 10, declarado | — |

Nenhum épico é "técnico" no título. O épico 4 se sustenta como valor ("o técnico vê a rede inteira num painel novo"), mas a história 4.1 é infraestrutura pura.

### 5.2 Achados por severidade

#### 🔴 Críticos

**C1 — O épico 4 não pode começar: Q-10 está aberta e o ajuste da spec não tem dono.**
A 4.1 traz como **pré-condição** "D-01, D-02, D-03 e D-05 decididos (Q-10) e a spec do backend ajustada conforme o spine". O próprio `epics.md` diz, em "Requisitos adicionais", que o ajuste da spec "faz parte da história 4.1". As duas leituras se contradizem: ou é pré-condição (e então nenhuma história a entrega), ou é entregável (e então não pode estar no "Dado"). O PRD marca Q-10 como **bloqueante da fase 0**.
*Remediação:* criar uma história 4.0, dona do Marcelo, "Decisões D-01..D-08 e spec v1.4", com AC: cada decisão registrada na spec §13 com data; os sete ajustes do spine aplicados (mais os da M10); a spec sai de "proposta" para "vigente". A 4.1 passa a depender dela.

**C2 — O corte da 3.10 para LoRa deixa o Nó 4, fonte de clima, fora da rede.**
A 3.10 troca "gateway e nós da estufa no mesmo dia" "para cumprir a regra de um rádio por rede", e o épico 3 fala em "rádio LoRa". Mas:
- o Nó 4 é **não objetivo** de migração para as placas M360 (PRD §5) e seu env `ProMini_04noodeSolarMini` é fixo em nRF24 (`${proto_rf24.flags}`, `src/DRY/horta/platformio.ini`);
- o `ARQUITETURA_FAMILIA.md` §4.1 registra que o Nó 4 roda o MCU direto na bateria e que **com RFM95W isso não pode** (SPI a 4,2 V acima do máximo de 3,9 V do SX1276);
- o AD-15 diz que migrar uma rede para LoRa é um corte de **todos** os nós da propriedade.
Com o gateway em LoRa, o Nó 4 para de chegar, e o motor do B, o relatório e o `/status` perdem a fonte de clima (FR-4, D-04). O motor cai no Open-Meteo de forma permanente, o que é exatamente a contra-métrica SM-C4.
*Remediação:* decidir antes da 3.10 uma de três saídas e registrar no PRD/spine: (a) corte da E1 com as placas M360 em **nRF24** pelo header 1×8, adiando o LoRa; (b) o Nó 4 ganha um novo hardware (LP a bateria com DHT/DS18B20) e a fonte de clima muda (D-04 prevê a alternativa); (c) um segundo gateway nRF24 só para o Nó 4. Incluir a escolha no AC da 3.10.

**C3 — O agendador durável nasce no épico 6, mas os épicos 4 e 5 já precisam dele.**
O AD-7 manda que "todo desligamento agendado, **retry** e notificação adiada" seja linha persistida no agendador durável. A spec §5.4 põe no agendador também `watchdog`, `ttl_sweep` e `purge`. O `epics.md` cria o agendador na 6.1 ("as regas na 6.1"). Consequências:
- 4.4 (vigilância a cada 60 s), 4.2 (retenção de 30 dias) e 4.3 (TTL) precisam de tarefas recorrentes antes do agendador;
- 5.2 (retry em 5/10/20 s) e 5.4 (expiração em 24 h) só podem ser implementados em memória, **violando o AD-7**, ou puxando parte da 6.1 para o épico 5.
É dependência para a frente entre épicos e quebra de uma regra de arquitetura.
*Remediação:* mover o núcleo do agendador durável (tabela `scheduled_action`, laço, `dedupe_key`, ON vencido descartado) para o épico 4 (história própria ou dentro da divisão da 4.1). A 6.1 fica com o OFF de rega e o reconciliador.

#### 🟠 Maiores

**M1 — Dependência para a frente dentro do épico 3: a 3.8 exige a 3.11.**
"Dado uma ACT montada com a correção da 3.11". A 3.11 é P0, não depende de nada e deveria vir antes. *Remediação:* renumerar a 3.11 para antes da 3.8, ou declarar a ordem de execução 3.11 → 3.8 no `epics.md` e no sprint status.

**M2 — A 4.1 tem tamanho de épico.**
Ela cria o projeto (sem starter), o simulador (eco, preempção, failsafe, sono), o teste de contrato, os vetores de validação compartilhados com a lib, a recusa de start sem segredo, o health e a publicação retida da posse. São pelo menos quatro entregas verificáveis independentemente. *Remediação:* dividir em 4.1a esqueleto + CI + configuração e segredos + health; 4.1b simulador; 4.1c teste de contrato e vetores; 4.1d agendador durável (C3) e tópico de posse.

**M3 — O teste de contrato precisa de um inventário legível por máquina, e o inventário não declara o que o AD-1 diz que declara.**
O AD-1 diz que o `inventario.md` define failsafe, grupo de exclusão (`mux99` = 31–37) e janela de despertar. O inventário é Markdown em tabelas; o failsafe está no §7.3, mas **não há "grupo de exclusão" nomeado nem janela de despertar** (consulta: `grep -niE "failsafe|mux99|grupo de exclus|janela|despertar"` no `inventario.md` só achou o §7.3 e uma "janela maior" sem relação; busca vazia não é prova, mas a estrutura de seções §1–§9 confirma). A 5.4 cita "a janela de despertar declarada no inventário", que não existe. *Remediação:* antes da 4.1c, decidir o formato do contrato (bloco YAML no `inventario.md` ou arquivo `inventario.yaml` gerado/conferido) e acrescentar grupo de exclusão e janela de despertar, atualizando o inventário na mesma entrega (regra do `CLAUDE.md`).

**M4 — O AC da 4.3 "a variante de placa sai do nome do sketch" não é atingível com o firmware atual.**
O Nó 99 apresenta `node.begin(F("NodeReles"), F("2.0.0"))` igual no Nano e na ACT (`src/DRY/horta/nos/99nodeReles/99nodeReles.cpp:320`). A spec RF-REG-07 só deriva o **perfil** pelo sufixo (`[LP]`, `[ON]`, `[PAS]`, `[REP]`, gerado em `M360Node.cpp:198–201`). *Remediação:* ou tirar a variante do AC, ou criar história de firmware (nome por flag de placa, com versão subida e inventário atualizado) antes da 4.3.

**M5 — "Alerta" aparece nos épicos 4 a 6, mas o notificador só nasce no 7.**
4.2 ("mensagem retida em `in/#` gera alerta"), 5.6 ("um único alerta registrado"), 6.1 (reconciliador, RF-IRR-07 alerta `RECONCILIADA`), 6.2/FR-33 ("o Telegram avisa"). A tabela `notification` também não tem história de criação na lista "Banco" do `epics.md`. Não está definido se "alerta" nesses épicos é Telegram, registro em tabela ou log. *Remediação:* criar no épico 4 a porta `Notifier` com um adaptador "registro apenas" (tabela `notification`) e definir que alertas dos épicos 4–6 são registrados; o épico 7 só troca o adaptador para Telegram.

**M6 — Janela sem avisos de rega entre a 6.5 e o épico 7.**
Na 6.5 a aba Irrigação do Node-RED é desligada. É ela que envia "IRRIGAÇÃO INICIADA/FINALIZADA" (UJ-1) e o aviso de falha. O backend só envia Telegram no épico 7. Entre as duas entregas a estufa rega sem nenhum aviso, o que regride UJ-1 e UJ-2 em produção. *Remediação:* puxar a mensagem de rega (RF-NOT-05) e o alerta de falha para a 6.5, ou exigir no "Dado" da 6.5 que a 7.1 esteja pronta.

**M7 — Reconciliação de startup (RF-CMD-12) e reconexão ao broker (RNF-10) sem história.**
O PRD §8.1 lista os dois como NFRs que definem o produto. Sem eles, depois de um reboot do Nó 99 o painel mostra um estado de relé que ninguém confirmou, e uma queda de 30 s do broker pode perder um eco e fazer um comando aplicado terminar em `FALHOU`. *Remediação:* acrescentar à 5.1 (ou nova 5.7) AC com o cenário RNF-10 ("derrubar o broker por 30 s não perde confirmação") e o RF-CMD-12 (estado incerto após reboot detectado por uptime).

**M8 — Paridade do painel (FR-44) e API de comandos (FR-46) sem verificação.**
Nenhum AC cobre a lista de comandos (RF-UI-08), o botão "reapresentar toda a rede" (RF-UI-02), os gráficos de bateria e de vazão (RF-UI-03) nem a documentação OpenAPI (RF-API-01). O checklist de paridade da spec §12.3 é o critério natural para o corte (8.3) e não é citado. *Remediação:* AC na 4.6 (gráficos completos), na 5.5 (reapresentar a rede), nova AC na 5.1 ou 5.6 (lista de comandos e `POST /api/commands` documentado), e o §12.3 como pré-condição da 8.3.

**M9 — FR-61 e FR-39 parciais.**
O eco de OFF não pedido que encerra a rega ativa (`end_reason = failsafe_or_preempt`, spec §12.2 cenários 4 e 5) não tem AC no épico 6; a 5.3 só atualiza o atuador. A 7.1 cobre alertas de rede e de rega, mas não os de comando (`FALHOU`, caixa postal; RF-NOT-04). *Remediação:* AC na 6.1 ou 6.2 com o cenário 5; AC na 7.1 com RF-NOT-04.

**M10 — A lista de ajustes da spec no spine está incompleta.**
O spine lista sete divergências. Há outras entre ADs e a spec v1.3 que não estão na lista:
- AD-2: a plataforma **recusa ON** em child ligado ou incerto e conta o limite a partir do primeiro publish (a spec não diz);
- AD-3: comando externo visto em `in/#` entra na correlação (RF-ING-04 só registra);
- AD-6: tópico retido de posse por grupo e troca com o grupo quieto (a spec só fala da regra de ouro);
- AD-13: código do prefixo com até 7 caracteres atribuído pela plataforma (a spec usa o CAR);
- AD-17: leituras de fila marcadas como atrasadas e fora da integração (RF-VOL-01 não prevê);
- AD-19: YAML só semeia banco vazio (a spec §5.6 trata o YAML como configuração versionada validada no start).
Quem implementar pela spec vai divergir do spine nesses pontos. *Remediação:* incluir na 4.0 (C1).

**M11 — Referências cruzadas erradas e dependência do épico 9 sobre o 10.**
O spine (AD-13) aponta a correção do prefixo de fábrica para a "história 10.4"; no `epics.md` ela é a **10.1** (a 10.4 é a autenticação do portal). Além disso, a 9.2 (descoberta de gateway novo como `PENDENTE`) só funciona se o gateway novo não publicar no prefixo do piloto (10.1), e a 9.5 recusa o canal k = 8 enquanto o piloto ainda está nele até a 10.2. O épico 9 depende do 10. *Remediação:* corrigir a referência no spine; executar a 10.1 (e a 10.2 no que toca o piloto) antes do épico 9, ou fundir a ordem dos dois épicos.

**M12 — Backup diário e script de migração de dados sem história.**
O backup é NFR do PRD §8.1 e da spec §11. A migração (spec §10: `mys_nodes` → atuadores com `certain = false`, assinantes, snapshot D-1, contador de volume) está espalhada em AC parciais (4.5, 7.2, 7.3) sem uma história que a entregue e teste. *Remediação:* história no épico 4 para backup e restauração testada; história de migração única, chamada pela 4.5, 6.5 e 7.2.

**M13 — D-06 (autenticação) é necessária na 4.6, mas não está nas pré-condições.**
A 4.6 exige login e 401 sem token; a 4.1 só exige D-01, 02, 03 e 05. O spine adia D-06 para "antes da fase 2" sem dono. *Remediação:* incluir D-06 na 4.0.

#### 🟡 Menores

- **m1** — A 3.6 põe no AC a consulta à ANATEL (Q-7), que é externa e sem dono definido ("Quem consulta o laboratório?").
- **m2** — 3.9: "o 3,3 V não cai abaixo do mínimo dos módulos" sem número (ESP8266 e RFM95W têm mínimos diferentes). 3.10: "o canteiro B regou pelo menos uma vez" depende do solo secar; trocar por uma rega manual comandada e confirmada pelo eco.
- **m3** — FR-15 (`act_99_rep`) e o perfil bateria da LP não têm AC de bancada; o alcance da ACT dentro do quadro metálico (risco do PRD §8.4) não é medido em nenhuma história.
- **m4** — 3.11 só cita a solenóide (600 s); falta o caso da peristáltica (120 s).
- **m5** — O PRD §8.4 e o AD-2 dizem que a 3.11 precede a "fase 3 da E2"; o `epics.md` diz que precede a 6.5 (fase 4). A spec §10 exige o failsafe gravado antes de qualquer fase que publique comandos. As leituras são compatíveis com o ajuste do spine (a fase 3 não toca o MUX), mas o texto diverge.
- **m6** — AD-4: o arquivo único de vetores deve rodar "nos testes da lib", e nenhuma história cria o lado do firmware; o campo `schema` versionado no heartbeat e nos eventos "antes da E3" não tem história.
- **m7** — 4.7: "ou a diferença está explicada" deixa o AC sem limite.
- **m8** — RNF-09 (< 100 ms), RNF-05 (`/metrics`), RNF-12 (expurgo de avaliações em 90 dias), RF-CMD-13 e RF-ING-10 (eventos de repareamento, cenário 15a) sem AC.
- **m9** — 9.6: "o tratamento LGPD está decidido (Q-8)" é decisão, não comportamento testável.
- **m10** — 10.2 diz que o campo novo do gateway em 521+ está "documentado no spine", mas o AD-12 só documenta o byte 516 do nó. 10.3 não tem AC de orçamento de RAM (AD-20) para o `SET_RADIO` no Nó 99.
- **m11** — Sem documento de UX (ver §4).
- **m12** — Não há história de pedido, recebimento e montagem da PCBA entre a 3.5 e a 3.7; não há sprint status para o `epics.md` novo.
- **m13** — AD-7: toda ligação de atuador com failsafe nasce com o seu OFF. A 6.2 não diz que o ON de peristáltica agenda OFF.

### 5.3 Dependências

**Dentro dos épicos**

| De | Para | Tipo | Situação |
|---|---|---|---|
| 3.8 | 3.11 | para a frente | ✗ (M1) |
| 3.10 | 3.7–3.9 | para trás | ✓ (falta também 3.5 e 3.6 explícitas) |
| 4.1 | Q-10, spec ajustada | externa | ✗ sem dono (C1) |
| 4.4, 4.2, 4.3 | agendador (6.1) | entre épicos, para a frente | ✗ (C3) |
| 5.2, 5.4 | agendador (6.1) | entre épicos, para a frente | ✗ (C3) |
| 5.6, 6.1, 6.2 | notificador (7.x) | entre épicos, para a frente | ✗ (M5) |
| 6.5 | 3.11, 6.1–6.4 | para trás | ✓ |
| 8.3 | 4–7 | para trás | ✓ |
| 9.2, 9.5 | 10.1, 10.2 | entre épicos, para a frente | ✗ (M11) |

**Criação de tabelas.** O plano "cada tabela nasce na história que a usa" está correto em princípio, mas faltam `scheduled_action` (precisa nascer antes de 6.1; C3), `notification` (sem história; M5), `actuator_state` (implícita na 4.3/5.1) e `rule_evaluation` (6.4, não listada).

### 5.4 Brownfield

O plano trata bem a convivência: backend só-lendo (épico 4), troca de posse por grupo (5.6, 6.5), rollback (6.5, 8.3), envs de protótipo preservados. Faltam: o lado do Node-RED que **lê** o tópico de posse (AD-6 diz "lido também pelo Node-RED"; hoje as trocas dependem de desligar abas à mão) e a migração de dados como história (M12).

### 5.5 Checklist por épico

| Item | Ép. 3 | Ép. 4 | Ép. 5 | Ép. 6 | Ép. 7 | Ép. 8 | Ép. 9 | Ép. 10 |
|---|---|---|---|---|---|---|---|---|
| Entrega valor ao usuário | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Funciona com os épicos anteriores | ✓ | ✗ C1/C3 | ✗ C3/M5 | ◐ M5/M6 | ✓ | ✓ | ✗ M11 | ✓ |
| Histórias bem dimensionadas | ✓ | ✗ 4.1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Sem dependência para a frente | ✗ 3.8 | ✗ | ✗ | ◐ | ✓ | ✓ | ✗ | ✓ |
| Tabelas criadas quando usadas | n/a | ◐ | ✗ | ◐ | ◐ | ✓ | ✓ | n/a |
| AC claros e testáveis | ◐ m2 | ◐ M4/m7 | ✓ | ✓ | ✓ | ✓ | ◐ m9 | ◐ m10 |
| Rastreável aos FRs | ✓ | ✓ | ✓ | ◐ | ◐ | ✓ | ✓ | ✓ |

---

## 6. Resumo e recomendações

### Estado geral de prontidão

**NEEDS WORK** no conjunto, com veredito por escopo:

| Escopo | Veredito | Por quê |
|---|---|---|
| **Épico 3 (3.5–3.11)** | **NEEDS WORK** | 3.11, 3.5, 3.6, 3.7 e 3.9 podem começar já. A 3.8 só depois da 3.11 (M1). A **3.10 está bloqueada** até decidir o rádio do corte e o destino do Nó 4 (C2) |
| **Épico 4** | **NOT READY** | Q-10 aberta e ajuste da spec sem dono (C1); agendador no lugar errado (C3); 4.1 grande demais (M2); contrato sem formato (M3); AC impossível na 4.3 (M4); "alerta" indefinido (M5); backup e migração sem história (M12); D-06 fora das pré-condições (M13) |
| **Épicos 5–8** | **NEEDS WORK** | Bem formados e rastreáveis, mas herdam C3 e M5, têm a janela sem avisos de rega (M6) e lacunas em RF-CMD-12/RNF-10 (M7), paridade (M8) e FR-61/FR-39 (M9) |
| **Épicos 9–10** | **NEEDS WORK** | Dependência do 9 sobre o 10 (M11); AC de decisão na 9.6; RAM do `SET_RADIO` sem AC |
| **Épico 11** | Futuro, não avaliado para execução | Depende da E3; pré-condições (faixa de EEPROM acima de 520) declaradas |

### Problemas críticos que pedem ação imediata

1. **C1** — Decidir D-01, D-02, D-03, D-05, D-06 (e D-07, D-08) e publicar a spec v1.4 com os ajustes do spine e os da M10, numa história 4.0 com dono.
2. **C2** — Decidir o rádio do corte da E1 e o destino do Nó 4 antes de qualquer trabalho de preparação da 3.10.
3. **C3** — Mover o núcleo do agendador durável para o épico 4.

### Próximos passos recomendados

1. **Agora, sem esperar nada:** executar a **3.11** (P0) e, em paralelo, 3.5, 3.6, 3.7 e 3.9. A 3.8 entra depois da 3.11.
2. **Sessão de decisões (Marcelo):** Q-10 inteira, a saída para o Nó 4 (C2), o formato do contrato do inventário (M3) e o significado de "alerta" antes do épico 7 (M5).
3. **Revisar o `epics.md`** (via `bmad-correct-course` ou edição do épico): criar a 4.0; dividir a 4.1; mover o agendador; criar as histórias de backup e migração; acrescentar os AC de M7, M8 e M9; puxar o aviso de rega para a 6.5; reordenar 10.1/10.2 antes do 9; corrigir a referência "10.4" no spine.
4. **Atualizar o `inventario.md`** com grupo de exclusão e janela de despertar, num formato que a CI leia.
5. **Rodar `bmad-sprint-planning`** sobre o `epics.md` revisto, com a ordem 3.11 → 3.8 explícita.
6. Antes da 4.6: uma página curta de mapa de telas e critério de celular para a rega manual.

### Questões abertas para o Marcelo

O workflow pararia para perguntar isto; no modo autônomo, ficam registradas:

1. Qual rádio a estufa usa no corte da 3.10, LoRa ou nRF24 nas placas M360? Se LoRa, como o Nó 4 continua na rede?
2. O ajuste da spec do backend é pré-condição da 4.1 ou entregável dela? Quem faz?
3. O contrato do AD-1 fica no `inventario.md` (bloco legível por máquina) ou num arquivo à parte?
4. Nos épicos 4 a 6, "alerta" significa Telegram, registro em tabela ou outra coisa?
5. Aceita-se que a estufa regue sem aviso no Telegram entre a 6.5 e o épico 7, ou o aviso de rega sobe para a 6.5?
6. A variante de placa precisa mesmo sair do nome do sketch (4.3)? Se sim, entra uma mudança de firmware no Nó 99.
7. O agendador durável pode ir para o épico 4?
8. Quem consulta a ANATEL (Q-7) e isso bloqueia só o pedido em volume ou também o lote do piloto?
9. A ordem dos épicos 9 e 10 pode ser invertida (ou a 10.1 antecipada)?
10. O PRD considera aceitável não ter documento de UX, com o mapa de telas sugerido na §4?

### Nota final

Esta avaliação encontrou **29 problemas** em seis categorias (descoberta, cobertura de FR, NFR, UX, estrutura de épicos e consistência entre documentos): **3 críticos, 13 maiores e 13 menores**. O PRD e o spine são de boa qualidade e a cobertura de FRs é completa no mapa (61/61) e de 88,5 % nos critérios de aceite. O que impede a execução é sequenciamento e decisões, não lacuna de produto. Resolva os críticos antes de iniciar o épico 4 e antes de preparar a 3.10. O resto do épico 3 pode seguir. Os achados podem ser usados para melhorar os artefatos, ou o usuário pode decidir seguir como está.

---

## 7. Resolução (2026-10-17)

Os 29 achados foram tratados na revisão 2 do `epics.md`, com ajustes no PRD, no spine e no `project-context.md`. A tabela "Rastreio da revisão 2" no fim do `epics.md` mostra onde cada um foi resolvido.

Três escolhas foram feitas sem o dono e estão marcadas para revisão:

- **C2:** o corte da estufa usa as placas M360 em nRF24 (história 3.12). O LoRa virou a história 3.13, bloqueada pela nova PRD Q-16 (como o Nó 4 entra numa rede LoRa).
- **M5:** nos épicos 4 a 6, "alerta" é linha registrada pelo adaptador de registro. O Telegram entra para as regas na 6.5 e para o resto no épico 7.
- **M3:** o contrato do AD-1 é um bloco legível por máquina dentro do `inventario.md`, criado na 4.3.

Continuam abertas com o dono:

- as decisões D-01 a D-08 (história 4.0);
- a Q-16;
- a Q-7 (ANATEL);
- a Q-8 (LGPD).

**Prontidão depois da revisão:**

- A 3.5 só precisa ser gravada.
- As histórias 3.6 a 3.11 podem seguir.
- O épico 4 está pronto para começar pela 4.0.
