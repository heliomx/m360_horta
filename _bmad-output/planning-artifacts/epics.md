---
stepsCompleted: [step-01-validate-prerequisites, step-02-design-epics, step-03-create-stories, step-04-final-validation]
inputDocuments:
  - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/prd.md
  - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/addendum.md
  - _bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-10-17/ARCHITECTURE-SPINE.md
  - docs/especificacao-backend-m360.md
  - hardware/ARQUITETURA_FAMILIA.md
  - src/DRY/horta/inventario.md
  - _bmad-output/planning-artifacts/implementation-readiness-report-2026-10-17.md
created: 2026-10-17
updated: 2026-10-17
revision: 2 — achados do relatório de prontidão de 2026-10-17 aplicados
supersedes:
  - _bmad-output/planning-artifacts/epics.md (2026-06)
  - docs/epics.md (2026-06)
  - _bmad-output/planning-artifacts/2-1-develop-node-umidade-temperatura-core.md
  - _bmad-output/planning-artifacts/2-2-gestao-de-energia-e-bateria.md
---

# M360 Horta — Épicos e histórias

## Visão geral

Este documento decompõe o PRD de 2026-10-17 e o spine de arquitetura em épicos e histórias. Os épicos 1 e 2 registram o que já está em produção. O épico 3 está em andamento: projeto e firmware estão prontos, e a correção do failsafe está implementada; faltam fabricação, bancada e o corte. Os épicos 4 a 8 são o backend (entrega E2), na ordem das fases da spec §10 com os ajustes do spine. O épico 9 prepara o rádio e o gateway para várias propriedades, e o 10 é a multi-propriedade na plataforma (E3). O 11 é o firmware genérico, futuro.

**Revisão 2.** O relatório de prontidão apontou falhas de sequência. Esta revisão muda o plano assim:

- **Épico 3:** o corte da estufa usa as placas M360 com nRF24, e o LoRa passa a ser um corte separado (C2).
- **Épico 4:** a história 4.0 reúne as decisões do backend e a spec v1.4 (C1). O agendador durável e o notificador de registro passam para este épico (C3, M5), e a antiga 4.1 foi dividida em quatro (M2).
- **Épico 6:** o aviso de rega no Telegram chega antes da troca de posse do MUX (M6).
- **Épicos 9 e 10:** foram trocados de ordem (M11).

A tabela de rastreio está no fim do documento.

Não há documento de UX. A interface do backend segue a spec §8 e os RF-UI, e a história 4.9 só começa com um mapa de telas curto.

**Regras para todas as histórias de backend:**

- Os FRs de F9 a F14 obrigam o backend, e não o Node-RED.
- Só o despachante publica (AD-5).
- Um atuador só é comandado pelo dono do grupo (AD-6).
- Nenhum estado bloqueia OFF (AD-22).
- Os enums vêm da spec §5.
- Toda tarefa recorrente ou adiada é linha do agendador durável (AD-7).

## Inventário de requisitos

### Requisitos funcionais

Os FRs estão no PRD §4, com texto e consequência.

| Faixa | Funcionalidade |
|---|---|
| FR-1 a FR-3 | F1 Umidade de solo |
| FR-4, FR-5 | F2 Clima |
| FR-6, FR-7 | F3 Vazão e volume |
| FR-8 a FR-11 | F4 Atuação segura (P0: FR-9, FR-10, FR-11) |
| FR-12, FR-13 | F5 Energia |
| FR-14 a FR-16 | F6 Rádio |
| FR-17 a FR-20 | F7 Gateway e portal |
| FR-21 a FR-23 | F8 Manutenção por ímã |
| FR-24 a FR-28, FR-61 | F9 Comandos (P0: FR-25) |
| FR-29 a FR-34 | F10 Irrigação A × B (P0: FR-31, FR-32, FR-34) |
| FR-35 a FR-38 | F11 Registro e vigilância |
| FR-39 a FR-42 | F12 Telegram |
| FR-43 a FR-47 | F13 Painel e API |
| FR-48 a FR-50 | F14 Indicadores do experimento |
| FR-51 a FR-57 | F15 Multi-propriedade |
| FR-58 a FR-60 | F16 Pareamento e firmware genérico |

### Requisitos não funcionais

- **NFR-1** Durabilidade: nenhuma ação futura só em memória (RNF-01).
- **NFR-2** Persistência de contadores e estado (RNF-02).
- **NFR-3** Idempotência e reconexão sem perder confirmação (RNF-03, RNF-10).
- **NFR-4** Segredo obrigatório ausente impede o start (RNF-04).
- **NFR-5** Comando falho rastreável em até três consultas; log estruturado e métricas (RNF-05).
- **NFR-6** CI com simulador, teste de contrato e vetores de validação, sem hardware (RNF-06, AD-1, AD-4).
- **NFR-7** UTC persistido; agenda e exibição no fuso da instalação (RNF-07).
- **NFR-8** Painel, API e broker autenticados; `retain` proibido em `in/#` (RNF-08, AD-4).
- **NFR-9** Comando→publish < 100 ms; séries de 7 dias < 1 s (RNF-09).
- **NFR-10** Backend em paralelo ao Node-RED, só lendo, até assumir cada grupo (RNF-11, AD-6).
- **NFR-11** Retenção: log 30 dias, leituras sem limite, avaliações 90 dias (RNF-12).
- **NFR-12** Nós com ≥ 512 B de RAM livre em `act_99` e `nano_99reles`; interrupção de vazão nunca bloqueada (AD-20, PRD §8.2).
- **NFR-13** Regras de instalação: 5,0 V travados, USB e borne nunca juntos, FTDI sem VCC (PRD §8.2).
- **NFR-14** Backup diário do banco com restauração testada (PRD §8.1, spec §11).
- **NFR-15** Estado de atuador recarregado no start e marcado incerto após reboot do gateway ou do Nó 99 (RF-CMD-12).

### Requisitos adicionais da arquitetura

- **AD-1:** mudança de child, failsafe, grupo ou janela de despertar atualiza na mesma história o firmware, o `inventario.md` (incluindo o bloco legível por máquina criado na 4.3) e a configuração do backend.
- **AD-2:** o failsafe conta do primeiro ON. Está corrigido no firmware (3.5) e precisa estar gravado no Nó 99 antes de qualquer fase do backend que publique comandos (5.6).
- **AD-3:** só o eco de aplicação confirma. Comando externo visto em `in/#` entra na correlação.
- **AD-6:** posse por grupo físico, publicada num tópico retido e trocada com o grupo quieto, levando o estado.
- **AD-7:** o agendador durável nasce no épico 4. Ação ON vencida é descartada no restart.
- **AD-12 e AD-13:** valem para o firmware do épico 9.
- **Spec do backend:** a v1.4, com os ajustes do spine, sai na história 4.0.
- **Starter template:** não há. O backend é criado do zero na 4.1.

### Mapa de cobertura dos FRs

| FR | Épico |
|---|---|
| FR-1, FR-2, FR-4, FR-6, FR-8 a FR-12, FR-14 (nRF24), FR-15, FR-17 a FR-20, FR-28 (firmware) | 1 (concluído) |
| Comportamento atual de F9–F12 no Node-RED | 2 (concluído; não obriga os FRs) |
| FR-1, FR-2, FR-8 a FR-15, FR-17 a FR-23 nas placas M360 | 3 |
| FR-3, FR-7 (acumulado), FR-35 a FR-38, FR-44 (leitura), FR-46 (leitura), FR-47 | 4 |
| FR-24 a FR-28, FR-61, FR-43 (bombas), FR-44 (comandos), FR-46 (comandos) | 5 |
| FR-5, FR-7 (por rega), FR-29 a FR-34, FR-39 (rega), FR-43 (MUX), FR-45, FR-61 (rega) | 6 |
| FR-39 (rede e comando), FR-40 a FR-42 | 7 |
| FR-48 a FR-50 | 8 |
| FR-16 (firmware), FR-55, FR-56 | 9 |
| FR-16 (plataforma), FR-51 a FR-54, FR-57 | 10 |
| FR-58 a FR-60 | 11 |

## Lista de épicos

| # | Épico | Entrega | Estado |
|---|---|---|---|
| 1 | Rede de campo com motor comum | E0 | Concluído |
| 2 | Automação do piloto no Node-RED | E0 | Concluído |
| 3 | Placas M360 em campo | E1 | Em andamento |
| 4 | Histórico e vigilância confiáveis | E2, fases 0–2 | Próximo (começa pela 4.0) |
| 5 | Comandos confirmados pelo eco | E2, fase 3 | Backlog |
| 6 | Irrigação durável A × B | E2, fases 4–5 | Backlog |
| 7 | Avisos e relatórios no Telegram | E2, fase 6 | Backlog |
| 8 | Indicadores do experimento e corte do Node-RED | E2, fase 7 | Backlog |
| 9 | Rádio por propriedade e gateway seguro | E3 | Backlog |
| 10 | Várias propriedades numa plataforma | E3 | Backlog |
| 11 | Pareamento e firmware genérico | Futuro | Backlog |

O épico 3 anda em paralelo aos épicos 4 a 8. Entre eles há duas ligações: o failsafe corrigido (3.5) precisa estar gravado antes da 5.6, e o corte da estufa (3.12) não pode acontecer durante um ciclo congelado. O épico 9 depende do 3 (LoRa em campo, 3.13). O 10 depende do 9 e dos épicos 4 a 8. O 11 depende do 10.

---

## Épico 1: Rede de campo com motor comum *(concluído)*

O técnico tem uma rede de nós que mede solo, clima e vazão. A irrigação é acionada com segurança no próprio nó. A rede conversa com o broker por um gateway configurável em campo, e todo nó usa a lib M360-DRY.

### História 1.1: Lib M360-DRY como motor único *(concluída)*

Como desenvolvedor de firmware, quero um motor comum de ciclo de vida, para criar um nó declarando só seus sensores e drivers.

**Critérios de aceite:**

**Dado** um nó com `NODE_ITEMS[]` e callbacks `onRead`/`onWrite`
**Quando** ele compila com a lib M360-DRY 2.2.0
**Então** o nó apresenta seus children, lê, envia por mudança ou a cada 10 ciclos e trata intervalo, `FORCE_UPDATE`, `REPRESENT` e `DEBUG_NET`
**E** `pio check -e check_m360_dry` sai sem defeitos.

### História 1.2: Nós de solo 1 e 2 *(concluída)*

Como técnico, quero seis sondas por canteiro, para ver a umidade em três distâncias e duas profundidades.

**Critérios de aceite:**

**Dado** os Nós 1 e 2 em fonte fixa
**Quando** o ciclo de leitura roda
**Então** cada sonda publica ADC bruto 0–1023 no child correspondente do inventário
**E** as sondas só recebem energia durante a varredura.

### História 1.3: Nó 4 de clima solar *(concluída)*

Como técnico, quero um nó solar de clima, para alimentar o motor do B, o relatório e o `/status`.

**Critérios de aceite:**

**Dado** o Nó 4 em `LOW_POWER`
**Quando** ele acorda a cada 30 minutos
**Então** publica temperatura e umidade do ar (11/12), temperatura do solo e bateria (255).

### História 1.4: Nó 99 atuador com failsafe *(concluída; rearme do failsafe corrigido na 3.5)*

Como técnico, quero um atuador que nunca deixe uma carga ligada esquecida, para regar sem medo de alagar.

**Critérios de aceite:**

**Dado** o Nó 99 com 7 canais no MUX e 2 bombas
**Quando** um canal é ligado com outro já ativo
**Então** o anterior é desligado e ecoado antes do eco do novo
**E** um único ON desliga sozinho em 600 s nas solenóides e em 120 s nas peristálticas, sem gateway
**E** a vazão é lida por interrupção e publicada em L/min.

### História 1.5: Gateway nativo com validação e portal *(concluída)*

Como instalador, quero um gateway que valide comandos e se configure pelo celular, para instalar sem regravar firmware.

**Critérios de aceite:**

**Dado** o gateway Horta em modo nativo
**Quando** ele recebe um comando com segmento inválido ou payload de `V_STATUS` diferente de `"0"`/`"1"`
**Então** publica `command_rejected` com motivo e não transmite
**E** sem configuração válida, ou com A0 em GND no boot, sobe o AP `M360-Config`
**E** publica heartbeat a cada 60 s e eventos de nó descoberto, perdido e reconectado.

### História 1.6: Diagnóstico do enlace de rádio *(concluída)*

Como técnico, quero distinguir falha de transmissor de falha de receptor, para não caçar defeito no firmware errado.

**Critérios de aceite:**

**Dado** o env `nano_99reles_diag`
**Quando** ele roda com o gateway
**Então** o nó registra uptime, RAM livre e sonda de TX
**E** o canal 76 entrega 38/38 com o rádio do gateway em bom estado.

---

## Épico 2: Automação do piloto no Node-RED *(concluído)*

O técnico e o produtor operam o experimento A × B pelo dashboard e pelo Telegram. Este épico registra o estado da E0; o backend o substitui nos épicos 4 a 8.

### História 2.1: Irrigação A por timer e B pelo motor *(concluída)*

Como pesquisador, quero o Canteiro A por horário e o B pelo motor agroclimático, para comparar os dois manejos.

**Critérios de aceite:**

**Dado** a aba Irrigação em produção
**Quando** dá 07:00 ou 17:00
**Então** o A rega 300 s no child 31
**E** o B é avaliado a cada 5 minutos com o clima do Nó 4 e a previsão do Open-Meteo.

### História 2.2: Confirmação pelo eco no Sincronizador *(concluída)*

Como técnico, quero que um comando só conte como confirmado com o eco do nó, para não ver verde com o relé desligado.

**Critérios de aceite:**

**Dado** um comando de atuação pendente
**Quando** chega só o ACK de transporte
**Então** o comando não é confirmado e o retry dispara
**E** só o eco com `ack=0`, `type=2` e payload igual confirma.

### História 2.3: Fonte de clima no Nó 4 *(concluída)*

Como técnico, quero o clima vindo do Nó 4, para que o motor continue funcionando quando o Nó 99 trocar para a ACT sem DHT.

**Critérios de aceite:**

**Dado** as abas Irrigação, IA Repórter e Telegram
**Quando** precisam de temperatura e umidade
**Então** leem os children 11/12 do Nó 4
**E** o `diff` do servidor com o repositório sai idêntico depois do deploy.

### História 2.4: Telegram, boletim e relatório diário *(concluída)*

Como produtor, quero avisos e um relatório diário no Telegram, para acompanhar a estufa sem painel.

**Critérios de aceite:**

**Dado** o bot em produção
**Quando** um assinante envia `/status` ou `/tempo`
**Então** recebe o estado e a previsão
**E** o boletim sai 4 vezes por dia e o relatório às 08:00.

---

## Épico 3: Placas M360 em campo *(em andamento)*

O técnico troca os protótipos por placas próprias e protegidas, que se mantêm com um ímã sem abrir a caixa. O critério de pronto é a SM-7: todos os itens de bancada aprovados antes de aposentar os protótipos. O primeiro corte usa o nRF24 pelo header legado das placas, e o LoRa vem num corte separado, que depende do destino do Nó 4 (PRD Q-16).

### História 3.1: Projeto das quatro placas *(concluída)*

Como técnico, quero esquemático, placa e arquivos de fabricação de RF, LP, ACT e GW, para encomendar a montagem.

**Critérios de aceite:**

**Dado** os projetos em `hardware/`
**Quando** rodam ERC e DRC com o `kicad-cli`
**Então** saem com zero violações
**E** BOM e posição JLCPCB estão gerados.

### História 3.2: Firmware comum da M360-LP *(concluída)*

Como desenvolvedor de firmware, quero um fonte único para a LP com configuração por nó, para criar um nó sem escrever C++.

**Critérios de aceite:**

**Dado** um `config/<nó>/nodeConfig.h` e um env
**Quando** compila `lp_01` ou `lp_02`
**Então** o nó tem os mesmos children do protótipo no inventário
**E** os drivers não usados ficam fora do binário.

### História 3.3: Firmware da ACT e da GW por flag de placa *(concluída)*

Como desenvolvedor de firmware, quero ACT e GW servidas pelo mesmo fonte do protótipo, para não manter dois firmwares.

**Critérios de aceite:**

**Dado** `NODE99_BOARD_ACT` e `M360_BOARD_GW`
**Quando** compilam `act_99` e `d1_mini_gw`
**Então** a pinagem confere com a netlist exportada
**E** `nano_99reles` e `d1_mini_gateway` compilam idênticos ao que estava antes.

### História 3.4: Manutenção por ímã nas três placas *(concluída)*

Como instalador, quero ímã para leitura forçada, repareamento e factory reset, para manter sem abrir a caixa.

**Critérios de aceite:**

**Dado** o firmware de 2026-10-17
**Quando** o ímã é aplicado na LP, mantido 3 s na ACT, ou afastado e mantido 3 s no GW nos primeiros 30 s
**Então** a LP envia tudo, a ACT refaz a ligação sem mexer nos relés, e o GW publica `factory_reset`, apaga a configuração e sobe em AP
**E** o botão do portal tem o mesmo efeito no GW.

### História 3.5: Failsafe sobre o tempo contínuo ligado (P0) *(implementada em 2026-10-17; falta gravar e testar)*

Como técnico, quero que ONs repetidos não estendam o tempo máximo ligado, para que o failsafe seja de fato a última barreira.

**Critérios de aceite:**

**Dado** uma solenóide ligada há 500 s, ou uma peristáltica ligada há 100 s
**Quando** chega outro ON para o mesmo child
**Então** o nó ecoa o estado sem rearmar o prazo, e a carga desliga aos 600 s (solenóide) ou 120 s (peristáltica) contados do primeiro ON
**E** um OFF seguido de ON começa um prazo novo
**E** a mudança vale para `nano_99reles` e `act_99`, está no inventário §7.3 e compila dentro do orçamento de RAM
**E** o firmware está gravado no Nó 99 em campo antes da história 5.6.

### História 3.6: Conferências antes do pedido PCBA

Como técnico, quero conferir o que só se vê na peça física, para não fabricar um lote com erro.

**Critérios de aceite:**

**Dado** a lista do `ARQUITETURA_FAMILIA.md` §9
**Quando** confiro o pino 1 do RJ45, os furos A4–A7 do Pro Mini com paquímetro, as rotações no visualizador da JLCPCB e o estoque do C25804
**Então** cada item tem resultado registrado no §9.

### História 3.7: Validar a banda do RFM95W

Como técnico, quero confirmar que o módulo transmite em 915 MHz, para não comprar em volume a banda errada.

**Critérios de aceite:**

**Dado** um RFM95W no mezanino
**Quando** ele transmite em 916,8 MHz e um RTL-SDR observa
**Então** o sinal aparece na frequência certa e o resultado fica no §9.

A consulta à ANATEL (PRD Q-7) não faz parte desta história. Ela bloqueia só o pedido de RFM95W em volume.

### História 3.8: Pedido e montagem do lote piloto

Como técnico, quero o lote piloto encomendado e recebido, para ter placas para a bancada.

**Critérios de aceite:**

**Dado** as histórias 3.6 aprovada e 3.7 concluída
**Quando** o pedido PCBA sai na JLCPCB com as quantidades do piloto
**Então** as placas chegam e os módulos e conectores são soldados à mão
**E** cada placa recebe identificação e fica registrada no §9.

### História 3.9: Bancada da M360-LP

Como técnico, quero a LP testada em bancada, para confiar na leitura de solo e na bateria.

**Critérios de aceite:**

**Dado** uma LP montada
**Quando** testo VSW, MUX, as 6 sondas, bateria pelo A6 com bandgap, corte em 3,0 V, ímã pela parede da caixa, corrente de sono no perfil bateria e fuses por ISP
**Então** cada item passa e o resultado vai para o §9
**E** com a fonte descendo até o corte, a LP para de transmitir e não reinicia em 1 h.

### História 3.10: Bancada da M360-ACT

Como técnico, quero a ACT testada em bancada, para instalar sem risco de relé batendo no boot.

**Critérios de aceite:**

**Dado** uma ACT montada com o firmware da 3.5
**Quando** ligo, desligo e reinicio a placa
**Então** nenhum relé fecha durante o boot
**E** passam o failsafe (inclusive o ON repetido), a preempção do MUX, a vazão com as duas variantes do YF-S201, o CE do nRF24 no D2, o repareamento por ímã e o `act_99_rep` repetindo para um nó fora do alcance direto
**E** a RAM livre medida respeita o NFR-12.

### História 3.11: Bancada da M360-GW

Como técnico, quero a GW testada em bancada, para instalar um gateway que não reinicia sob carga.

**Critérios de aceite:**

**Dado** uma GW montada
**Quando** WiFi e o rádio transmitem juntos, medidos no osciloscópio
**Então** o 3,3 V não cai abaixo de 3,0 V nem reinicia o ESP8266
**E** passam o LED verde por Q1, o CE do nRF24 no D1, o modo AP e o factory reset por ímã através da parede.

### História 3.12: Corte da estufa para as placas M360 em nRF24

Como técnico, quero trocar a rede da estufa para as placas M360 sem perder o Nó 4, para aposentar os protótipos sem esperar o LoRa.

**Critérios de aceite:**

**Dado** as histórias 3.6 a 3.11 aprovadas e nenhum ciclo congelado em curso
**Quando** gateway M360-GW, LPs dos Nós 1 e 2 e ACT do Nó 99 entram no mesmo dia com o nRF24 pelo header 1×8 (`radio_flags` em nRF24), e o Nó 4 protótipo permanece
**Então** todos os nós aparecem com os children do inventário, inclusive o clima do Nó 4
**E** uma rega manual no canteiro B é confirmada pelo eco
**E** o 5 V do quadro está travado em 5,0 V e o alcance da ACT dentro do quadro está medido
**E** o env gravado em cada nó fica registrado no inventário
**E** existe roteiro de volta para os protótipos.

### História 3.13: Corte da estufa para LoRa *(bloqueada pela PRD Q-16)*

Como técnico, quero a estufa em LoRa, para ter o alcance e o isolamento por canal da E3.

**Critérios de aceite:**

**Dado** a Q-16 decidida (como o Nó 4 entra na rede LoRa) e a 3.7 aprovada
**Quando** gateway e todos os nós da estufa trocam para RFM95W no mesmo dia
**Então** todos os nós, inclusive a fonte de clima, aparecem com os children do inventário
**E** o motor do B segue recebendo clima de nó, e não só do Open-Meteo.

---

## Épico 4: Histórico e vigilância confiáveis

O técnico vê a rede inteira num painel novo, com histórico que sobrevive a reinícios, um critério único de online e offline, e backup. O backend roda ao lado do Node-RED, só lendo.

### História 4.0: Decisões do backend e spec v1.4

Como dono do projeto, quero as decisões de stack tomadas e a spec alinhada ao spine, para que o backend comece sobre um contrato estável.

Dono: Marcelo. Apoio do agente para redigir a spec.

**Critérios de aceite:**

**Dado** as recomendações do addendum do PRD §6
**Quando** D-01, D-02, D-03, D-05, D-06, D-07 e D-08 são decididas
**Então** cada decisão está registrada na spec §13 com data
**E** os ajustes listados no spine ("Ajustes pendentes na spec") estão aplicados
**E** a spec passa de "proposta" a v1.4 vigente.

### História 4.1: Esqueleto do backend, configuração e health

Como técnico, quero um backend que só sobe com configuração válida e mostra a própria saúde, para saber a qualquer momento se ele está de pé.

**Critérios de aceite:**

**Dado** a história 4.0 concluída
**Quando** o backend sobe na CI e no servidor
**Então** ele recusa iniciar sem um segredo obrigatório, com mensagem clara
**E** o health mostra broker, banco, versão e a chave de confirmação (RF-CMD-13)
**E** o log é estruturado e as métricas estão expostas (RNF-05).

### História 4.2: Simulador de nó

Como técnico, quero um simulador de nó, para testar comandos e regas sem tocar a estufa.

**Critérios de aceite:**

**Dado** o backend e um broker de teste na CI
**Quando** o simulador recebe comandos
**Então** ele reproduz eco, preempção do MUX, failsafe sobre tempo contínuo ligado, sono de nó LP com janela de despertar e reboot com uptime zerado.

### História 4.3: Contrato do inventário e vetores de validação

Como técnico, quero que a CI quebre quando firmware, inventário e backend divergirem, para nunca ter comando ignorado em silêncio.

**Critérios de aceite:**

**Dado** um bloco legível por máquina no `inventario.md` com children, failsafe, grupo de exclusão (`mux99` = 31–37) e janela de despertar
**Quando** a CI roda
**Então** o teste de contrato compara o bloco com a configuração do backend e falha na divergência
**E** o arquivo `test/vectors/` de validação de comando roda na CI do backend e num teste nativo da lib
**E** o bloco do inventário entra na mesma entrega que o firmware que ele descreve.

### História 4.4: Agendador durável, notificador de registro e tópico de posse

Como técnico, quero tarefas recorrentes e adiadas que sobrevivem a reinícios, para que nada dependa de timer em memória.

**Critérios de aceite:**

**Dado** o backend com banco
**Quando** uma ação é agendada e o processo é morto e reiniciado
**Então** a ação executa uma vez, com chave de idempotência
**E** uma ação ON vencida é descartada e registrada, nunca executada
**E** a porta de notificação existe com um adaptador de registro (tabela de notificações), e "alerta" nos épicos 4 a 6 significa linha registrada
**E** o backend publica, num tópico retido de plataforma, a posse de `mux99`, 38 e 39, que aponta o Node-RED.

### História 4.5: Ingestão e log auditável

Como técnico, quero todo o tráfego do gateway guardado, para investigar qualquer problema depois.

**Critérios de aceite:**

**Dado** o backend assinando o prefixo da estufa
**Quando** chegam medições, ACKs de transporte, heartbeat, `out/events` e comandos em `in/#`
**Então** todos entram no log com direção correta
**E** só medições numéricas viram leitura, com o valor bruto e o convertido
**E** um tópico com segmento não numérico é rejeitado e contado
**E** uma mensagem retida em `in/#` gera alerta registrado
**E** leituras entregues de fila depois de uma reconexão são marcadas como atrasadas
**E** eventos de repareamento (`REJOIN`, `REJOIN:OK`) viram eventos de nó (RF-ING-10)
**E** o expurgo de 30 dias do log roda pelo agendador.

### História 4.6: Registro automático de nós e children

Como técnico, quero a lista de nós montada pelo que os nós anunciam, para não editar lista nenhuma ao incluir um nó.

**Critérios de aceite:**

**Dado** um nó que se apresenta
**Quando** o backend recebe a apresentação
**Então** o nó e seus children aparecem com tipo e rótulo
**E** um `REPRESENT` zera a tabela de children sem apagar o estado de atuadores
**E** o perfil de energia sai do sufixo do sketch
**E** a variante de placa é atributo do nó, preenchido pelo instalador no cadastro, e não deduzido do sketch.

### História 4.7: Status único e vigilância

Como técnico, quero um só critério de online, para que painel, API e alertas nunca discordem.

**Critérios de aceite:**

**Dado** o registro preenchido
**Quando** o gateway passa 120 s sem contato
**Então** ele fica `OFFLINE` e todos os dados aparecem como "sem contato"
**E** cada nó tem o status da spec §5, calculado só no backend por tarefa do agendador
**E** eventos de nó do gateway ficam registrados como evidência, sem disparar alerta por si
**E** nó sem contato há mais de 48 h sai da lista ativa sem perder histórico.

### História 4.8: Volume acumulado persistente

Como pesquisador, quero o volume de água que não zera em reinício, para confiar nos litros do experimento.

**Critérios de aceite:**

**Dado** leituras de vazão do Nó 99
**Quando** o backend integra com `0 < dt ≤ 300 s`, sem leituras atrasadas
**Então** o acumulado é persistido a cada atualização
**E** reiniciar o backend não altera o acumulado.

### História 4.9: Painel de leitura e API

Como técnico, quero gráficos, mapa da rede, vigilância e log no painel novo, para deixar de abrir o dashboard antigo.

**Critérios de aceite:**

**Dado** um mapa de telas aprovado (navegação, uma página por função, critério de uso no celular para a rega manual)
**Quando** abro o painel com login
**Então** há gráficos de 7 dias de solo A e B, clima, bateria, vazão e volume, mapa, vigilância e log com exportação CSV
**E** cada consulta de 7 dias responde em menos de 1 s
**E** a API de leitura está documentada, e ela e o fluxo em tempo real exigem token; sem token, a resposta é 401.

### História 4.10: Backup e restauração

Como técnico, quero backup diário com restauração testada, para não perder o histórico do experimento.

**Critérios de aceite:**

**Dado** o banco em produção
**Quando** o backup diário roda
**Então** um banco restaurado a partir dele, num ambiente separado, sobe e mostra as mesmas leituras e o mesmo volume acumulado.

### História 4.11: Migração única de dados do Node-RED

Como técnico, quero os dados úteis do Node-RED trazidos uma vez, para o backend começar com o histórico que importa.

**Critérios de aceite:**

**Dado** um backup do contexto do Node-RED
**Quando** o script de migração roda
**Então** `mys_nodes` vira registro com atuadores marcados como incertos, o contador `flow_vol_node_99_21` entra como deslocamento inicial datado, os assinantes e o snapshot D-1 são importados
**E** rodar o script de novo não duplica nada.

### História 4.12: Sete dias de paridade com o Node-RED

Como técnico, quero comparar o backend com o Node-RED por uma semana, para saber que ele vê a mesma rede.

**Critérios de aceite:**

**Dado** backend e Node-RED ligados juntos
**Quando** passam 7 dias
**Então** a contagem de mensagens difere da do Central Logger em no máximo 0,1 %
**E** o status de cada nó é igual ao do watchdog do Node-RED, salvo divergência explicada por um evento registrado.

---

## Épico 5: Comandos confirmados pelo eco

O técnico comanda nós e bombas pelo painel novo e sabe, sem ambiguidade, se o nó aplicou o comando. Os canais do MUX ficam com o Node-RED até o épico 6.

### História 5.1: Despachante e ciclo de vida do comando

Como técnico, quero cada comando registrado com estado e origem, para rastrear qualquer falha.

**Critérios de aceite:**

**Dado** um pedido de `atuar` a um atuador apresentado, de um grupo cujo dono é o backend
**Quando** o despachante publica, sem `retain`
**Então** o comando percorre os estados da spec §5.3 com horário e origem, e o publish sai em menos de 100 ms
**E** só o eco do nó com o payload pedido o confirma; o ACK de transporte é só anotado
**E** um child desconhecido, ou um grupo de outro dono, é recusado com motivo e sem publicar
**E** um comando em `in/#` que o backend não emitiu entra como externo e casa com o seu eco
**E** a lista de comandos no painel e a API de comandos documentada mostram estado, tentativas, origem e o eco que confirmou.

### História 5.2: Retry, falha e rejeição do gateway

Como técnico, quero saber em segundos que um comando falhou, para agir antes que o canteiro sofra.

**Critérios de aceite:**

**Dado** o nó desligado no simulador
**Quando** um comando de atuação é enviado
**Então** há 3 tentativas (5, 10 e 20 s), agendadas no agendador durável, e o comando termina `FALHOU` em cerca de 35 s com alerta registrado
**E** `command_rejected` do gateway leva a `REJEITADO`, e `command_send_failed` conta como tentativa.

### História 5.3: Serialização do MUX e eco não pedido

Como técnico, quero que a plataforma respeite o MUX do Nó 99, para o estado na tela seguir o nó.

**Critérios de aceite:**

**Dado** dois comandos para canais diferentes do MUX no simulador
**Quando** chegam juntos
**Então** só um fica em voo por vez, em fila FIFO por nó
**E** o eco de preempção desliga o canal anterior na plataforma
**E** um `0` não pedido (failsafe) atualiza o atuador e gera evento
**E** um ON para um child já ligado ou incerto é recusado.

### História 5.4: Caixa postal para nós que dormem

Como técnico, quero mandar comando a um nó dormindo, para que ele aplique quando acordar.

**Critérios de aceite:**

**Dado** um nó LP em `LOW_POWER` no simulador
**Quando** envio dois comandos seguidos
**Então** os dois ficam em fila, sem sobrescrita
**E** são entregues dentro da janela de despertar do bloco do inventário
**E** expiram em 24 h por tarefa do agendador.

### História 5.5: Comandos administrativos

Como técnico, quero ajustar intervalo, forçar leitura, reapresentar e diagnosticar qualquer nó, para manter a rede sem ir ao campo.

**Critérios de aceite:**

**Dado** o painel admin de um nó
**Quando** envio `set_interval` de 1 a 1440
**Então** o comando é confirmado ou marcado como divergente pela leitura do child 254
**E** `force_update`, `represent` e `debug_net` saem como `V_CUSTOM` no child 0, e a resposta do `DEBUG_NET` fica ligada ao comando
**E** "reapresentar toda a rede" gera um `represent` por nó.

### História 5.6: Bombas no painel e corte dos botões de bomba do Node-RED

Como técnico, quero ligar e desligar as bombas pelo painel novo com o estado real, para nunca acreditar num verde falso.

**Critérios de aceite:**

**Dado** o firmware da 3.5 gravado no Nó 99, os grupos 38 e 39 quietos e passados ao backend, e os botões de bomba do Node-RED desativados
**Quando** aperto ligar numa bomba
**Então** o botão fica âmbar até o eco e verde depois
**E** 20 comandos seguidos terminam confirmados pelo eco
**E** uma falha simulada termina `FALHOU` em 35 s com um único alerta registrado.

### História 5.7: Estado no start e reconexão ao broker

Como técnico, quero que o backend nunca mostre estado de relé que ninguém confirmou, para confiar no painel depois de quedas.

**Critérios de aceite:**

**Dado** o backend com atuadores no estado persistido
**Quando** o backend reinicia, ou o gateway ou o Nó 99 reiniciam (uptime menor)
**Então** os atuadores afetados ficam incertos até o próximo eco
**E** com o broker fora por 30 s, nenhuma confirmação em voo se perde.

---

## Épico 6: Irrigação durável A × B

O pesquisador tem as duas parcelas regando sozinhas, com cada rega registrada e avisada, e com desligamento garantido mesmo com o servidor caindo. Cada decisão do motor fica explicada. Tudo é construído no simulador e em modo sombra, e o `mux99` muda de dono numa história só, a 6.6.

### História 6.1: Regas duráveis e reconciliador

Como técnico, quero que todo desligamento de rega sobreviva a um reinício, para nunca alagar o canteiro.

**Critérios de aceite:**

**Dado** uma rega de 60 s em curso no simulador
**Quando** o backend é morto aos 20 s e volta aos 70 s
**Então** sai exatamente um OFF e a rega fica marcada como fechada com atraso
**E** o reconciliador, a cada 30 s, desliga solenóide ligada há mais de 660 s sem rega ativa
**E** um eco `0` não pedido encerra a rega ativa daquele canal como `failsafe_or_preempt` (spec §12.2, cenário 5).

### História 6.2: Rega manual e botões do MUX

Como técnico, quero regar um canteiro e acionar peristálticas pelo painel, com contagem regressiva, para intervir sem atalho perigoso.

**Critérios de aceite:**

**Dado** o painel de irrigação no simulador
**Quando** peço 900 s numa solenóide
**Então** a duração é truncada para `failsafe − 15 s` contados do primeiro publish, e o truncamento é registrado
**E** a solenóide só liga como rega, com registro, OFF agendado, contagem regressiva, botão parar e volume ao fim
**E** o ON de peristáltica pede confirmação e nasce com o seu OFF agendado
**E** se a abertura falha, a rega termina como falha e o OFF é cancelado.

### História 6.3: Canteiro A no backend

Como pesquisador, quero o controle por timer pronto no backend, para tirar a irrigação do Node-RED.

**Critérios de aceite:**

**Dado** o simulador com relógio controlado
**Quando** dá 07:00 e 17:00 no fuso da instalação
**Então** o A rega 300 s no child 31 com OFF confirmado.

### História 6.4: Motor do B em modo sombra

Como pesquisador, quero o motor agroclimático rodando no backend sem comandar, para conferir suas decisões antes de entregar a estufa.

**Critérios de aceite:**

**Dado** o Node-RED ainda dono do `mux99`
**Quando** o motor do backend avalia a cada 300 s com dados reais
**Então** cada avaliação guarda entradas e a camada que decidiu, 288 por dia, sem publicar nada
**E** sem clima do Nó 4 há mais de 1 h, usa o Open-Meteo e registra isso
**E** a validade do solo se mede pela idade do contato do nó
**E** soak, travas e sobreposições de solo crítico (≥ 650 e ≥ 700) seguem a spec §7.3
**E** o expurgo de avaliações com mais de 90 dias roda pelo agendador
**E** 7 dias de avaliações conferem com as decisões do Node-RED.

### História 6.5: Avisos de rega no Telegram

Como técnico, quero continuar recebendo o início, o fim e a falha de cada rega no Telegram, para não perder o aviso quando a irrigação sair do Node-RED.

**Critérios de aceite:**

**Dado** o adaptador Telegram da porta de notificação, no lugar do adaptador de registro, só para mensagens de rega
**Quando** uma rega começa, termina, é reconciliada ou falha
**Então** sai `INICIADA`, `FINALIZADA`, `RECONCILIADA` ou o aviso de falha, com canteiro, duração e modo (RF-NOT-05)
**E** uma falha de envio não interrompe a rega.

### História 6.6: Troca de posse do `mux99` para o backend

Como técnico, quero passar a irrigação inteira para o backend de uma vez, para que só um sistema comande o MUX.

**Critérios de aceite:**

**Dado** as histórias 6.1 a 6.5 concluídas, o `mux99` quieto e nenhum ciclo congelado em curso
**Quando** a aba Irrigação e os botões do MUX do Node-RED são desligados e a posse é publicada para o backend
**Então** `last_irrigation_at`, soak e o estado dos atuadores passam junto
**E** A, B e rega manual rodam no backend por 3 dias com OFF confirmado em toda rega e aviso no Telegram
**E** o teste de kill no meio de uma rega real resulta em um único OFF
**E** o rollback devolve a posse ao Node-RED pela mesma regra.

### História 6.7: Regras ligáveis e parâmetros auditados

Como técnico, quero ligar e desligar regras e ajustar parâmetros pelo painel, para mudar o manejo sem reimplantar.

**Critérios de aceite:**

**Dado** o painel de irrigação
**Quando** desligo a regra do B ou altero um parâmetro do motor
**Então** a mudança vale sem reimplantar e fica na trilha com usuário e horário
**E** a configuração alterada fica no banco, e o YAML não a sobrescreve no próximo start.

### História 6.8: Volume por rega

Como pesquisador, quero o volume de cada rega, para somar litros por canteiro.

**Critérios de aceite:**

**Dado** uma rega encerrada
**Quando** o backend integra o child 21 durante a rega, com um único canal do `mux99` ativo
**Então** o volume fica no registro da rega e é atribuído ao canteiro
**E** os totais diários saem do banco, nunca de acumulador em memória.

---

## Épico 7: Avisos e relatórios no Telegram

O técnico e o produtor recebem o que importa uma vez por evento e consultam a estufa pelo bot, agora servido pelo backend.

### História 7.1: Alertas de rede e de comando

Como técnico, quero um alerta por transição, para não ignorar o que importa.

**Critérios de aceite:**

**Dado** o adaptador Telegram ligado para todos os alertas
**Quando** o gateway cai e volta
**Então** chega um alerta de queda e um de volta, com motivo e horário
**E** `factory_reset` sempre alerta e suprime o offline seguinte
**E** alertas de comando (falha, caixa postal, administrativo, `set_interval`) saem com seus cooldowns (RF-NOT-04).

### História 7.2: Bot de consulta e assinantes

Como produtor, quero consultar status e previsão pelo bot, para acompanhar sem painel.

**Critérios de aceite:**

**Dado** os assinantes migrados na 4.11
**Quando** envio `/status`, `/tempo` ou `/start <convite>`
**Então** recebo a resposta ou sou inscrito
**E** um comando desconhecido recebe ajuda.

### História 7.3: Boletim e relatório diário

Como técnico, quero o boletim 4 vezes por dia e o relatório às 08:00, para ter o resumo sem pedir.

**Critérios de aceite:**

**Dado** as abas Telegram e IA do Node-RED desligadas
**Quando** dá 06, 11, 16 e 21 h, e 08:00
**Então** saem o boletim e o relatório com métricas de 24 h do banco e comparação com o dia anterior
**E** sem o modelo de linguagem o relatório sai só com números
**E** toda notificação é registrada, e uma falha de envio não interrompe a origem.

---

## Épico 8: Indicadores do experimento e corte do Node-RED

O pesquisador tira o Índice Manejo360 da plataforma, e a estufa passa a depender só do backend.

### História 8.1: Ciclo e colheita

Como pesquisador, quero registrar o ciclo e a colheita, para o Índice sair sem planilha.

**Critérios de aceite:**

**Dado** um ciclo com transplante e colheita registrados
**Quando** lanço perdas semanais, massa por canteiro e variedade, peso médio de 10 cabeças e nota visual
**Então** a plataforma mostra litros por canteiro, eficiência hídrica (kg/L), economia de água em % e o Índice Manejo360
**E** o ciclo marca se rodou congelado (PRD §6).

### História 8.2: Exportação dos dados do experimento

Como pesquisador, quero exportar tudo do ciclo, para refazer a análise fora da plataforma.

**Critérios de aceite:**

**Dado** um ciclo registrado
**Quando** peço a exportação
**Então** recebo CSV por tabela (leituras, regas, volumes, avaliações), filtrado pelo ciclo, com horários em UTC.

### História 8.3: Corte e desligamento do Node-RED

Como técnico, quero desligar o Node-RED com rota de volta, para a estufa depender de um sistema só.

**Critérios de aceite:**

**Dado** os épicos 4 a 7 concluídos, o backend dono de todos os grupos e o checklist de paridade da spec §12.3 completo
**Quando** o Node-RED é desligado depois de um backup final
**Então** passam 14 dias seguidos com SM-2 ≥ 99 %, SM-3 = 0 e SM-4 = 288/dia
**E** o rollback (religar o Node-RED, devolver a posse e pôr o backend em somente leitura) está escrito e testado
**E** `nr_deploy.py` e o `DEPLOY.md` passam a ser marcados como legado.

---

## Épico 9: Rádio por propriedade e gateway seguro

O instalador configura o canal de rádio e protege o gateway no próprio portal. Os nós acompanham a troca de canal sem regravação, e um gateway novo nunca se passa por outro. Vem antes da plataforma multi-propriedade porque ela depende disso.

### História 9.1: Gateway sem prefixo de produção de fábrica

Como administrador, quero que um gateway novo ou resetado não publique em nenhum prefixo real, para ele nunca se passar pela estufa piloto.

**Critérios de aceite:**

**Dado** um gateway recém-gravado ou após factory reset
**Quando** ele sobe
**Então** fica em AP sem publicar até receber UF e código pelo portal
**E** o heartbeat leva o ID do chip e o campo `schema`, e o mesmo campo `schema` vai para `gateway/status` e para os eventos.

### História 9.2: Piloto fora do canal de fábrica e canal no portal

Como instalador, quero gravar o canal de rádio no portal, para configurar a propriedade em campo.

**Critérios de aceite:**

**Dado** a estufa em LoRa (3.13), o byte 516 do nó e o campo `radioChannel` da `M360DeviceConfig` documentados no AD-12
**Quando** gravo k = 0..63 no portal
**Então** o gateway sobe em `915,2 + 0,2·k` MHz e informa `radioChannel` no heartbeat
**E** todo binário sai de fábrica em k = 8
**E** a estufa piloto passa para um canal diferente de 8.

### História 9.3: Troca de canal dos nós com presença física

Como instalador, quero trocar o canal dos nós por comando, para não regravar cada um.

**Critérios de aceite:**

**Dado** nós no canal k e o instalador com ímã no local
**Quando** o backend envia `SET_RADIO:<k2>` como `V_CUSTOM` no child 0, durante a janela aberta pelo ímã
**Então** cada nó ecoa antes de trocar e reaparece no canal k2
**E** um nó que não acha o gateway em k2 volta para k
**E** fora da janela de presença o comando é ignorado
**E** `act_99` e `lp_01` continuam com pelo menos 512 B de RAM livre.

### História 9.4: Autenticação do portal do gateway

Como administrador, quero o portal protegido por credencial, para ninguém na rede local reconfigurar ou apagar o gateway.

**Critérios de aceite:**

**Dado** o gateway em modo STA
**Quando** alguém acessa `/save` ou `/reset` sem credencial
**Então** o acesso é negado
**E** o modo AP de fábrica continua permitindo a primeira configuração.

---

## Épico 10: Várias propriedades numa plataforma

O administrador leva uma propriedade nova do gateway ligado às regras habilitadas sem editar arquivo nem recompilar, com isolamento total entre propriedades.

### História 10.1: Instalações e isolamento

Como administrador, quero cadastrar instalações isoladas, para atender várias propriedades numa instância.

**Critérios de aceite:**

**Dado** duas instalações com prefixos diferentes no simulador
**Quando** ambas publicam e recebem comandos
**Então** nenhum nó, comando, regra ou alerta vaza entre elas
**E** o código do prefixo tem até 7 caracteres `[A-Za-z0-9]`, é atribuído pela plataforma e é recusado se duplicado na UF.

### História 10.2: Descoberta e estados da instalação

Como administrador, quero que um gateway novo apareça como pendente, para confirmar antes de qualquer comando.

**Critérios de aceite:**

**Dado** gateways com o firmware da 9.1 e tráfego num prefixo desconhecido
**Quando** o backend o recebe
**Então** cria a instalação `PENDENTE`, ingere e alerta o chat de operação da plataforma, sem comandar nem notificar a instalação
**E** a confirmação leva a `COMISSIONANDO` (comandos manuais sim, regras não)
**E** `SUSPENSA` ingere sem alertar e recusa ON, mas nunca recusa OFF
**E** outro ID de chip no mesmo prefixo gera `gateway_conflict` e suspende comandos.

### História 10.3: Checklist de comissionamento

Como instalador, quero um checklist com evidência por item, para entregar uma propriedade comprovadamente funcionando.

**Critérios de aceite:**

**Dado** uma instalação `COMISSIONANDO` com a posse dos grupos no backend
**Quando** os itens C1 a C12 da spec §15.4 têm evidência
**Então** a instalação passa a `ATIVA`, e as regras podem ser ligadas
**E** C9 (failsafe) só é dispensado com a marca `failsafe_nao_verificado`
**E** o piloto entra como `ATIVA` por migração, com dispensa registrada dos itens já provados em produção.

### História 10.4: Credencial, local e assinantes por instalação

Como administrador, quero credencial MQTT, coordenadas, fuso e assinantes por instalação, para cada propriedade ter sua previsão e seus avisos.

**Critérios de aceite:**

**Dado** uma instalação com credencial própria
**Quando** ela tenta publicar fora do prefixo, ou com `retain` em `in/#`
**Então** o broker recusa
**E** Open-Meteo, boletim e horários usam as coordenadas e o fuso dela
**E** o convite do bot vincula o assinante à instalação.

### História 10.5: Canal de rádio por instalação

Como administrador, quero atribuir um canal de rádio por propriedade, para vizinhos não se ouvirem.

**Critérios de aceite:**

**Dado** o piloto fora do canal 8 (9.2) e uma instalação ativa no canal k a 5 km de outra
**Quando** tento cadastrar a outra no mesmo canal, ou no canal de fábrica k = 8
**Então** o cadastro é recusado, salvo força administrativa justificada e registrada.

### História 10.6: Papéis por instalação

Como administrador, quero operadores que veem só as propriedades deles, para dar acesso sem expor as outras.

**Critérios de aceite:**

**Dado** a Q-8 (LGPD) decidida e um operador vinculado a uma instalação
**Quando** ele acessa painel e API
**Então** vê e comanda só a instalação dele
**E** o administrador continua vendo todas
**E** dados pessoais (chat_id, nomes) seguem a regra de retenção e acesso decidida na Q-8.

### História 10.7: Kit Hélio como segunda instalação

Como técnico, quero o Kit Hélio no mesmo painel, para acompanhar as duas propriedades num lugar.

**Critérios de aceite:**

**Dado** D-08 decidido como segunda instalação só leitura
**Quando** o Kit Hélio passa a usar o mesmo fonte do gateway Horta (`dispatchCommand`, `parseTopicByte`, `reportCommandRejected`, `send(..., false)`), e o gateway próprio dele é removido
**Então** o Nó 11 aparece na instalação dele, separado da Horta, sem comandos
**E** as duas redes nRF24 não operam ao alcance uma da outra, ou usam `MY_RF24_BASE_RADIO_ID` distintos.

### História 10.8: Código por tipo, configuração por instalação

Como desenvolvedor de firmware, quero uma pasta de configuração por propriedade e um só fonte por tipo de nó, para atender uma propriedade nova sem copiar código.

**Critérios de aceite:**

**Dado** a 10.7 concluída, com um gateway único para Horta e Kit Hélio
**Quando** os fontes vão para `src/firmware/` e a configuração de cada propriedade para `src/instalacoes/<UF>-<código>-<nome>/` (AD-23)
**Então** nenhum fonte em `src/firmware/` contém rótulo, ID de nó ou dado de propriedade; os rótulos dos Nós 1 e 2 saem dos `.cpp` para `nodeConfig.h`
**E** cada instalação tem `platformio.ini` (envs com prefixo da instalação e rede de rádio), `inventario.md` e um `nodeConfig.h` por nó
**E** todos os envs compilam, e cada binário gerado é igual ao de antes em children, rótulos, `MY_NODE_ID` e rádio (conferido pela metadata e pela apresentação no simulador)
**E** workflow `m360-node-factory`, `CLAUDE.md`, `AGENTS.md`, skill de codificação e `project-context.md` descrevem a estrutura nova
**E** criar uma propriedade de teste exige só copiar uma pasta de instalação e editar configuração, sem tocar em `src/firmware/`.

---

## Épico 11: Pareamento e firmware genérico *(futuro)*

O instalador põe um nó novo na rede sem gravar um env por nó, e o backend define identidade e perfil. Este épico sucede o spine de 2026-09-08, revisto no addendum do PRD §3, e depende de D-09 e da spec §15.

### História 11.1: Janela de pareamento

Como instalador, quero abrir uma janela de pareamento pelo portal ou pela plataforma, para registrar um nó virgem.

**Critérios de aceite:**

**Dado** um nó virgem e a janela aberta por 60 s
**Quando** o nó liga
**Então** recebe um ID atribuído pelo backend para aquele slot UF+código
**E** fora da janela nenhum nó virgem entra.

### História 11.2: Perfil do nó na memória e alteração remota

Como técnico, quero mudar o perfil de um nó à distância, para trocar sensores ativos e cadência sem regravar.

**Critérios de aceite:**

**Dado** a faixa de EEPROM do perfil documentada no spine, acima de 520
**Quando** o backend envia um perfil novo
**Então** o nó o grava, ecoa e se reapresenta com os children do perfil
**E** o binário cabe no orçamento de RAM do AD-20.

### História 11.3: Reset remoto do nó

Como técnico, quero realocar um nó por comando, para reaproveitá-lo sem abrir a caixa.

**Critérios de aceite:**

**Dado** um nó em operação
**Quando** o backend envia o comando de reset em `V_CUSTOM` no child 0, nunca no 254
**Então** o nó apaga ID e perfil, preserva o intervalo e volta a virgem
**E** o backend marca o slot como livre.

---

## Validação final

- **Cobertura:** todos os FRs de FR-1 a FR-61 estão no mapa e em pelo menos uma história. FR-33, FR-39, FR-44, FR-46 e FR-61 ganharam critérios de aceite na revisão 2. Os NFR-14 e NFR-15 têm as histórias 4.10 e 5.7.
- **Dependências:**
  - Nenhuma história depende de uma posterior no mesmo épico.
  - Entre épicos, as dependências apontam para trás: 5.6 → 3.5; 9.2 → 3.13; 10.x → 9.x.
  - Dentro do épico 10, a 10.8 (estrutura por instalação, AD-23) vem depois da 10.7 (gateway único).
  - A 3.13 está bloqueada pela PRD Q-16, e a 4.0 depende de decisões do dono.
- **Banco:** cada tabela nasce na história que a usa:
  - agendador e notificações: 4.4
  - log: 4.5
  - registro: 4.6
  - comandos: 5.1
  - estado de atuador: 5.1
  - regas: 6.1
  - avaliações: 6.4
  - ciclo: 8.1
  - instalação: 10.1
- **Starter:** não há starter template. A 4.1 cria o projeto depois da 4.0.
- **Prioridade P0:** FR-9, FR-10, FR-11, FR-25, FR-31, FR-32 e FR-34 têm critérios de aceite nas histórias 1.4, 3.5, 3.10, 5.1, 6.1, 6.2 e 6.6.

## Rastreio da revisão 2 (relatório de prontidão)

| Achado | Onde foi resolvido |
|---|---|
| C1 | História 4.0 |
| C2 | 3.12 em nRF24; 3.13 bloqueada pela PRD Q-16 |
| C3 | Agendador na 4.4 |
| M1 | Failsafe renumerado para 3.5, antes das bancadas |
| M2 | Antiga 4.1 dividida em 4.1 a 4.4 |
| M3 | Bloco legível no inventário, na 4.3 |
| M4 | Variante de placa cadastrada (4.6) |
| M5 | Notificador de registro na 4.4 |
| M6 | Avisos de rega na 6.5, antes da troca de posse |
| M7 | 5.7 |
| M8 | 4.9, 5.1, 5.5 e 8.3 |
| M9 | 6.1 e 7.1 |
| M10 | Lista do spine ampliada |
| M11 | Épicos 9 e 10 trocados; referência do spine corrigida |
| M12 | 4.10 e 4.11 |
| M13 | D-06 na 4.0 |
| m1 | ANATEL fora do critério da 3.7 |
| m2 | Limite de 3,0 V na 3.11; rega manual na 3.12 |
| m3 | Repetidor na 3.10; alcance na 3.12; perfil bateria na 3.9 |
| m4 | Peristáltica na 3.5 |
| m5 | Failsafe gravado antes da 5.6 em todos os documentos |
| m6 | Vetores na lib (4.3); `schema` na 9.1 |
| m7 | Limite de 0,1 % na 4.12 |
| m8 | 4.1, 4.5, 5.1 e 6.4 |
| m9 | LGPD como pré-condição (10.6) |
| m10 | Campo no AD-12; RAM na 9.3 |
| m11 | Mapa de telas na 4.9 |
| m12 | 3.8 e sprint status regenerado |
| m13 | 6.2 |
