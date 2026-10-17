---
stepsCompleted: [step-01-validate-prerequisites, step-02-design-epics, step-03-create-stories, step-04-final-validation]
inputDocuments:
  - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/prd.md
  - _bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/addendum.md
  - _bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-10-17/ARCHITECTURE-SPINE.md
  - docs/especificacao-backend-m360.md
  - hardware/ARQUITETURA_FAMILIA.md
  - src/DRY/horta/inventario.md
created: 2026-10-17
updated: 2026-10-17
supersedes:
  - _bmad-output/planning-artifacts/epics.md (2026-06)
  - docs/epics.md (2026-06)
  - _bmad-output/planning-artifacts/2-1-develop-node-umidade-temperatura-core.md
  - _bmad-output/planning-artifacts/2-2-gestao-de-energia-e-bateria.md
---

# M360 Horta — Épicos e histórias

## Visão geral

Este documento decompõe o PRD de 2026-10-17 e o spine de arquitetura em épicos e histórias.

- **Épicos 1 e 2:** registram o que já está em produção.
- **Épico 3:** está em andamento. Projeto e firmware estão prontos; faltam fabricação, bancada e a correção do failsafe.
- **Épicos 4 a 8:** são o backend (entrega E2), na ordem das fases da spec §10, com o ajuste do spine.
- **Épicos 9 e 10:** são a multi-propriedade (E3).
- **Épico 11:** é o firmware genérico, para o futuro.

O ajuste do spine é este: o grupo `mux99` (children 31–37) muda de dono inteiro, de uma vez (AD-6). Por isso, irrigação A, motor do B e botões do MUX passam juntos para o backend, na história 6.5.

Não há documento de UX: a interface do backend segue a spec §8 e os RF-UI.

**Regras para todas as histórias de backend:**

- Os FRs de F9 a F14 obrigam o backend, não o Node-RED.
- Só o despachante publica (AD-5).
- Atuador só é comandado pelo dono do grupo (AD-6).
- Nenhum estado bloqueia OFF (AD-22).
- Enums vêm da spec §5.

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

- **NFR-1 Durabilidade:** nenhuma ação futura vive só em memória (RNF-01).
- **NFR-2 Persistência:** contadores e estado sobrevivem a reinícios (RNF-02).
- **NFR-3 Idempotência e reconexão:** nenhuma confirmação se perde numa reconexão (RNF-03, RNF-10).
- **NFR-4 Segredos:** um segredo obrigatório ausente impede o start (RNF-04).
- **NFR-5 Rastreabilidade:** um comando falho é rastreável em até três consultas (RNF-05).
- **NFR-6 CI sem hardware:** a CI roda com simulador, teste de contrato e vetores de validação (RNF-06, AD-1, AD-4).
- **NFR-7 Fuso:** UTC persistido; agenda e exibição no fuso da instalação (RNF-07).
- **NFR-8 Autenticação:** painel, API e broker exigem autenticação; `retain` é proibido em `in/#` (RNF-08, AD-4).
- **NFR-9 Desempenho:** comando→publish abaixo de 100 ms; séries de 7 dias abaixo de 1 s (RNF-09).
- **NFR-10 Convivência:** o backend roda em paralelo ao Node-RED, só lendo, até assumir cada grupo (RNF-11, AD-6).
- **NFR-11 Retenção:** log por 30 dias, leituras sem limite, avaliações por 90 dias (RNF-12).
- **NFR-12 Firmware:** pelo menos 512 B de RAM livre em `act_99` e `nano_99reles`; a interrupção de vazão nunca é bloqueada (AD-20, PRD §8.2).
- **NFR-13 Instalação:** 5,0 V travados; USB e borne nunca juntos; FTDI sem VCC (PRD §8.2).

### Requisitos adicionais da arquitetura

- **AD-1:** uma mudança de child, failsafe ou grupo atualiza na mesma história o firmware, o `inventario.md` e a configuração do backend. O teste de contrato falha na divergência.
- **AD-2:** o failsafe vale sobre o tempo contínuo ligado. A correção do rearme é P0 e precede a história 6.5.
- **AD-3:** só o eco de aplicação confirma. O comando externo visto em `in/#` entra na correlação.
- **AD-6:** a posse é por grupo físico, publicada num tópico retido e trocada com o grupo quieto, levando o estado junto.
- **AD-7:** todo OFF é linha persistida no agendador. Um ON vencido é descartado no restart.
- **AD-12 e AD-13:** valem para o firmware novo da E3, que segue o mapa de EEPROM por MCU e não pode ter padrão de fábrica igual a um prefixo de produção.
- **Spec do backend:** precisa dos ajustes listados no spine antes da fase 0. Isso faz parte da história 4.1.
- **Starter template:** não há. O backend é criado do zero na história 4.1.

### Mapa de cobertura dos FRs

| FR | Épico |
|---|---|
| FR-1, FR-2, FR-4, FR-6, FR-8 a FR-12, FR-14 (nRF24), FR-15, FR-17 a FR-20, FR-28 (firmware) | 1 (concluído) |
| Comportamento atual de F9–F12 no Node-RED | 2 (concluído; não obriga os FRs) |
| FR-1, FR-2, FR-8 a FR-15, FR-17 a FR-23 nas placas M360 | 3 |
| FR-3, FR-7 (acumulado), FR-35 a FR-38, FR-44 (leitura), FR-46 (leitura), FR-47 | 4 |
| FR-24 a FR-28, FR-61, FR-43 (bombas), FR-44 (comandos), FR-46 (comandos) | 5 |
| FR-5, FR-7 (por rega), FR-29 a FR-34, FR-43 (MUX), FR-45 | 6 |
| FR-39 a FR-42 | 7 |
| FR-48 a FR-50 | 8 |
| FR-16 (plataforma), FR-51 a FR-54, FR-57 | 9 |
| FR-16 (firmware), FR-55, FR-56 | 10 |
| FR-58 a FR-60 | 11 |

## Lista de épicos

| # | Épico | Entrega | Estado |
|---|---|---|---|
| 1 | Rede de campo com motor comum | E0 | Concluído |
| 2 | Automação do piloto no Node-RED | E0 | Concluído |
| 3 | Placas M360 em campo | E1 | Em andamento |
| 4 | Histórico e vigilância confiáveis | E2, fases 0–2 | Próximo |
| 5 | Comandos confirmados pelo eco | E2, fase 3 | Backlog |
| 6 | Irrigação durável A × B | E2, fases 4–5 | Backlog |
| 7 | Avisos e relatórios no Telegram | E2, fase 6 | Backlog |
| 8 | Indicadores do experimento e corte do Node-RED | E2, fase 7 | Backlog |
| 9 | Várias propriedades numa plataforma | E3 | Backlog |
| 10 | Rádio por propriedade e gateway seguro | E3 | Backlog |
| 11 | Pareamento e firmware genérico | Futuro | Backlog |

O épico 3 e os épicos 4 a 8 andam em paralelo. A única ligação entre eles é a história 3.11: a correção do failsafe, já no firmware, precisa estar gravada no Nó 99 antes da história 6.5. Os épicos 9 e 10 dependem do 3 e dos épicos 4 a 8. O épico 11 depende dos épicos 9 e 10.

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

### História 1.4: Nó 99 atuador com failsafe *(concluída; rearme do failsafe corrigido na 3.11)*

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

O técnico troca os protótipos por placas próprias e protegidas, com rádio LoRa, que se mantêm com um ímã sem abrir a caixa. O critério de pronto é a SM-7: todos os itens de bancada aprovados antes de aposentar os protótipos.

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

### História 3.5: Conferências antes do pedido PCBA

Como técnico, quero conferir o que só se vê na peça física, para não fabricar um lote com erro.

**Critérios de aceite:**

**Dado** a lista do `ARQUITETURA_FAMILIA.md` §9
**Quando** confiro o pino 1 do RJ45, os furos A4–A7 do Pro Mini com paquímetro, as rotações no visualizador da JLCPCB e o estoque do C25804
**Então** cada item tem resultado registrado no §9
**E** o pedido só sai com todos aprovados.

### História 3.6: Validar a banda do RFM95W

Como técnico, quero confirmar que o módulo transmite em 915 MHz, para não comprar em volume a banda errada.

**Critérios de aceite:**

**Dado** um RFM95W no mezanino
**Quando** ele transmite em 916,8 MHz e um RTL-SDR observa
**Então** o sinal aparece na frequência certa
**E** o resultado e a consulta à ANATEL (Q-7) ficam registrados antes do pedido em volume.

### História 3.7: Bancada da M360-LP

Como técnico, quero a LP testada em bancada, para confiar na leitura de solo e na bateria.

**Critérios de aceite:**

**Dado** uma LP montada
**Quando** testo VSW, MUX, as 6 sondas, bateria pelo A6 com bandgap, corte em 3,0 V, ímã pela parede da caixa, corrente de sono e fuses por ISP
**Então** cada item passa e o resultado vai para o §9
**E** com a fonte descendo até o corte, a LP para de transmitir e não reinicia em 1 h.

### História 3.8: Bancada da M360-ACT

Como técnico, quero a ACT testada em bancada, para instalar sem risco de relé batendo no boot.

**Critérios de aceite:**

**Dado** uma ACT montada com a correção da 3.11
**Quando** ligo, desligo e reinicio a placa
**Então** nenhum relé fecha durante o boot
**E** passam o failsafe, a preempção do MUX, a vazão com as duas variantes do YF-S201, o CE do nRF24 no D2 e o repareamento por ímã
**E** a RAM livre medida respeita o NFR-12.

### História 3.9: Bancada da M360-GW

Como técnico, quero a GW testada em bancada, para instalar um gateway que não reinicia sob carga.

**Critérios de aceite:**

**Dado** uma GW montada
**Quando** WiFi e LoRa transmitem juntos
**Então** o 3,3 V não cai abaixo do mínimo dos módulos
**E** passam o LED verde por Q1, o CE do nRF24 no D1, o modo AP e o factory reset por ímã através da parede.

### História 3.10: Corte da estufa para as placas M360

Como técnico, quero trocar toda a rede da estufa de uma vez, para cumprir a regra de um rádio por rede.

**Critérios de aceite:**

**Dado** as histórias 3.7 a 3.9 aprovadas e nenhum ciclo congelado em curso
**Quando** gateway e nós da estufa são trocados no mesmo dia, com o 5 V do quadro travado em 5,0 V
**Então** todos os nós aparecem com os children do inventário, e o canteiro B regou pelo menos uma vez com eco confirmado
**E** o env gravado em cada nó fica registrado no inventário
**E** existe roteiro de volta para os protótipos nRF24.

### História 3.11: Failsafe sobre o tempo contínuo ligado (P0) *(implementada em 2026-10-17; falta gravar e testar)*

Como técnico, quero que ONs repetidos não estendam o tempo máximo ligado, para que o failsafe seja de fato a última barreira.

**Critérios de aceite:**

**Dado** uma solenóide ligada há 500 s
**Quando** chega outro ON para o mesmo child
**Então** o nó ecoa o estado sem rearmar o prazo, e a solenóide desliga aos 600 s contados do primeiro ON
**E** um OFF seguido de ON começa um prazo novo
**E** a mudança vale para `nano_99reles` e `act_99`, atualiza o inventário §7 e compila dentro do orçamento de RAM.

---

## Épico 4: Histórico e vigilância confiáveis

O técnico vê a rede inteira num painel novo, com histórico que sobrevive a reinícios e um critério único de online e offline. O backend roda ao lado do Node-RED, só lendo.

### História 4.1: Backend com simulador, contrato e CI

Como técnico, quero um backend com simulador de nó e testes automáticos, para validar o comportamento antes de tocar a estufa.

**Critérios de aceite:**

**Dado** D-01, D-02, D-03 e D-05 decididos (Q-10) e a spec do backend ajustada conforme o spine
**Quando** a CI roda sem hardware
**Então** o simulador reproduz eco, preempção do MUX, failsafe e sono de nó LP
**E** o teste de contrato compara o inventário com a configuração e passa
**E** os vetores de validação de comando (`test/vectors/`) rodam e passam
**E** o backend recusa iniciar sem um segredo obrigatório
**E** o health mostra broker, banco e versão, e a posse publicada de `mux99`, 38 e 39 aponta o Node-RED.

### História 4.2: Ingestão e log auditável

Como técnico, quero todo o tráfego do gateway guardado, para investigar qualquer problema depois.

**Critérios de aceite:**

**Dado** o backend assinando o prefixo da estufa
**Quando** chegam medições, ACKs de transporte, heartbeat, `out/events` e comandos em `in/#`
**Então** todos entram no log com direção correta e retenção de 30 dias
**E** só medições numéricas viram leitura, com o valor bruto e o convertido
**E** um tópico com segmento não numérico é rejeitado e contado
**E** uma mensagem retida em `in/#` gera alerta
**E** leituras entregues de fila depois de uma reconexão são marcadas como atrasadas.

### História 4.3: Registro automático de nós e children

Como técnico, quero a lista de nós montada pelo que os nós anunciam, para não editar lista nenhuma ao incluir um nó.

**Critérios de aceite:**

**Dado** um nó que se apresenta
**Quando** o backend recebe a apresentação
**Então** o nó e seus children aparecem com tipo e rótulo
**E** um `REPRESENT` zera a tabela de children sem apagar o estado de atuadores
**E** o perfil de energia e a variante de placa saem do nome do sketch.

### História 4.4: Status único e vigilância

Como técnico, quero um só critério de online, para que painel, API e alertas nunca discordem.

**Critérios de aceite:**

**Dado** o registro preenchido
**Quando** o gateway passa 120 s sem contato
**Então** ele fica `OFFLINE` e todos os dados aparecem como "sem contato"
**E** cada nó tem o status da spec §5, calculado só no backend
**E** eventos de nó do gateway ficam registrados como evidência, sem disparar alerta por si.

### História 4.5: Volume acumulado persistente

Como pesquisador, quero o volume de água que não zera em reinício, para confiar nos litros do experimento.

**Critérios de aceite:**

**Dado** leituras de vazão do Nó 99
**Quando** o backend integra com `0 < dt ≤ 300 s`, sem leituras atrasadas
**Então** o acumulado é persistido a cada atualização
**E** reiniciar o backend não altera o acumulado
**E** o contador do Node-RED entra uma vez como deslocamento inicial datado.

### História 4.6: Painel de leitura e API

Como técnico, quero gráficos, mapa da rede, vigilância e log no painel novo, para deixar de abrir o dashboard antigo.

**Critérios de aceite:**

**Dado** o login no painel
**Quando** abro gráficos de 7 dias, mapa, vigilância e log
**Então** cada tela responde em menos de 1 s e o log exporta CSV
**E** a API de leitura e o fluxo em tempo real exigem token; sem token, a resposta é 401.

### História 4.7: Sete dias de paridade com o Node-RED

Como técnico, quero comparar o backend com o Node-RED por uma semana, para saber que ele vê a mesma rede.

**Critérios de aceite:**

**Dado** backend e Node-RED ligados juntos
**Quando** passam 7 dias
**Então** a contagem de mensagens é igual à do Central Logger
**E** o status de cada nó é igual ao do watchdog do Node-RED, ou a diferença está explicada.

---

## Épico 5: Comandos confirmados pelo eco

O técnico comanda nós e bombas pelo painel novo e sabe, sem ambiguidade, se o nó aplicou o comando. Os canais do MUX ficam com o Node-RED até o épico 6.

### História 5.1: Despachante e ciclo de vida do comando

Como técnico, quero cada comando registrado com estado e origem, para rastrear qualquer falha.

**Critérios de aceite:**

**Dado** um pedido de `atuar` a um atuador apresentado, de um grupo cujo dono é o backend
**Quando** o despachante publica, sem `retain`
**Então** o comando percorre os estados da spec §5.3 com horário e origem
**E** só o eco do nó com o payload pedido o confirma; o ACK de transporte é só anotado
**E** um child desconhecido, ou um grupo de outro dono, é recusado com motivo e sem publicar
**E** um comando em `in/#` que o backend não emitiu entra como externo e casa com o seu eco.

### História 5.2: Retry, falha e rejeição do gateway

Como técnico, quero saber em segundos que um comando falhou, para agir antes que o canteiro sofra.

**Critérios de aceite:**

**Dado** o nó desligado no simulador
**Quando** um comando de atuação é enviado
**Então** há 3 tentativas (5, 10 e 20 s) e o comando termina `FALHOU` em cerca de 35 s
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
**E** são entregues dentro da janela de despertar declarada no inventário
**E** expiram em 24 h.

### História 5.5: Comandos administrativos

Como técnico, quero ajustar intervalo, forçar leitura, reapresentar e diagnosticar qualquer nó, para manter a rede sem ir ao campo.

**Critérios de aceite:**

**Dado** o painel admin de um nó
**Quando** envio `set_interval` de 1 a 1440
**Então** o comando é confirmado ou marcado como divergente pela leitura do child 254
**E** `force_update`, `represent` e `debug_net` saem como `V_CUSTOM` no child 0, e a resposta do `DEBUG_NET` fica ligada ao comando.

### História 5.6: Bombas no painel e corte dos botões de bomba do Node-RED

Como técnico, quero ligar e desligar as bombas pelo painel novo com o estado real, para nunca acreditar num verde falso.

**Critérios de aceite:**

**Dado** os grupos 38 e 39 quietos e passados ao backend, com os botões de bomba do Node-RED desativados
**Quando** aperto ligar numa bomba
**Então** o botão fica âmbar até o eco e verde depois
**E** 20 comandos seguidos terminam confirmados pelo eco
**E** uma falha simulada termina `FALHOU` em 35 s com um único alerta registrado.

---

## Épico 6: Irrigação durável A × B

O pesquisador tem as duas parcelas regando sozinhas, com cada rega registrada e desligamento garantido mesmo com o servidor caindo. Cada decisão do motor fica explicada. Tudo é construído no simulador e em modo sombra, e o `mux99` muda de dono numa história só, a 6.5.

### História 6.1: Agendador durável e reconciliador

Como técnico, quero que todo desligamento sobreviva a um reinício, para nunca alagar o canteiro.

**Critérios de aceite:**

**Dado** uma rega de 60 s em curso no simulador
**Quando** o backend é morto aos 20 s e volta aos 70 s
**Então** sai exatamente um OFF, e a rega fica marcada como fechada com atraso
**E** um ON agendado e vencido é descartado e registrado, nunca executado
**E** o reconciliador, a cada 30 s, desliga solenóide ligada há mais de 660 s sem rega ativa.

### História 6.2: Rega manual e botões do MUX

Como técnico, quero regar um canteiro e acionar peristálticas pelo painel, com contagem regressiva, para intervir sem atalho perigoso.

**Critérios de aceite:**

**Dado** o painel de irrigação no simulador
**Quando** peço 900 s numa solenóide
**Então** a duração é truncada para `failsafe − 15 s` contados do primeiro publish, e o truncamento é registrado
**E** a solenóide só liga como rega, com registro, OFF agendado, contagem regressiva, botão parar e volume ao fim
**E** peristálticas pedem confirmação
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
**E** 7 dias de avaliações conferem com as decisões do Node-RED.

### História 6.5: Troca de posse do `mux99` para o backend

Como técnico, quero passar a irrigação inteira para o backend de uma vez, para que só um sistema comande o MUX.

**Critérios de aceite:**

**Dado** as histórias 3.11, 6.1 a 6.4 concluídas, o `mux99` quieto e nenhum ciclo congelado em curso
**Quando** a aba Irrigação e os botões do MUX do Node-RED são desligados e a posse é publicada para o backend
**Então** `last_irrigation_at`, soak e o estado dos atuadores passam junto
**E** A, B e rega manual rodam no backend por 3 dias com OFF confirmado em toda rega
**E** o teste de kill no meio de uma rega real resulta em um único OFF
**E** o rollback devolve a posse ao Node-RED pela mesma regra.

### História 6.6: Regras ligáveis e parâmetros auditados

Como técnico, quero ligar e desligar regras e ajustar parâmetros pelo painel, para mudar o manejo sem reimplantar.

**Critérios de aceite:**

**Dado** o painel de irrigação
**Quando** desligo a regra do B ou altero um parâmetro do motor
**Então** a mudança vale sem reimplantar e fica na trilha com usuário e horário
**E** a configuração alterada fica no banco, e o YAML não a sobrescreve no próximo start.

### História 6.7: Volume por rega

Como pesquisador, quero o volume de cada rega, para somar litros por canteiro.

**Critérios de aceite:**

**Dado** uma rega encerrada
**Quando** o backend integra o child 21 durante a rega, com um único canal do `mux99` ativo
**Então** o volume fica no registro da rega e é atribuído ao canteiro
**E** os totais diários saem do banco, nunca de acumulador em memória.

---

## Épico 7: Avisos e relatórios no Telegram

O técnico e o produtor recebem o que importa uma vez por evento e consultam a estufa pelo bot, agora servido pelo backend.

### História 7.1: Alertas de rede, comando e rega

Como técnico, quero um alerta por transição, para não ignorar o que importa.

**Critérios de aceite:**

**Dado** o notificador ligado
**Quando** o gateway cai e volta
**Então** chega um alerta de queda e um de volta, com motivo e horário
**E** `factory_reset` sempre alerta e suprime o offline seguinte
**E** regas geram `INICIADA`, `FINALIZADA` ou `RECONCILIADA` com canteiro, duração e modo.

### História 7.2: Bot de consulta e assinantes

Como produtor, quero consultar status e previsão pelo bot, para acompanhar sem painel.

**Critérios de aceite:**

**Dado** os assinantes migrados do Node-RED
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

**Dado** os épicos 4 a 7 concluídos e o backend dono de todos os grupos
**Quando** o Node-RED é desligado depois de um backup final
**Então** passam 14 dias seguidos com SM-2 ≥ 99 %, SM-3 = 0 e SM-4 = 288/dia
**E** o rollback (religar o Node-RED, devolver a posse e pôr o backend em somente leitura) está escrito e testado
**E** `nr_deploy.py` e o `DEPLOY.md` passam a ser marcados como legado.

---

## Épico 9: Várias propriedades numa plataforma

O administrador leva uma propriedade nova do gateway ligado às regras habilitadas sem editar arquivo nem recompilar, com isolamento total entre propriedades.

### História 9.1: Instalações e isolamento

Como administrador, quero cadastrar instalações isoladas, para atender várias propriedades numa instância.

**Critérios de aceite:**

**Dado** duas instalações com prefixos diferentes no simulador
**Quando** ambas publicam e recebem comandos
**Então** nenhum nó, comando, regra ou alerta vaza entre elas
**E** o código do prefixo tem até 7 caracteres `[A-Za-z0-9]`, é atribuído pela plataforma e é recusado se duplicado na UF.

### História 9.2: Descoberta e estados da instalação

Como administrador, quero que um gateway novo apareça como pendente, para confirmar antes de qualquer comando.

**Critérios de aceite:**

**Dado** tráfego num prefixo desconhecido
**Quando** o backend o recebe
**Então** cria a instalação `PENDENTE`, ingere e alerta o chat de operação da plataforma, sem comandar nem notificar a instalação
**E** a confirmação leva a `COMISSIONANDO` (comandos manuais sim, regras não)
**E** `SUSPENSA` ingere sem alertar e recusa ON, mas nunca recusa OFF.

### História 9.3: Checklist de comissionamento

Como instalador, quero um checklist com evidência por item, para entregar uma propriedade comprovadamente funcionando.

**Critérios de aceite:**

**Dado** uma instalação `COMISSIONANDO` com a posse dos grupos no backend
**Quando** os itens C1 a C12 da spec §15.4 têm evidência
**Então** a instalação passa a `ATIVA`, e as regras podem ser ligadas
**E** C9 (failsafe) só é dispensado com a marca `failsafe_nao_verificado`
**E** o piloto entra como `ATIVA` por migração, com dispensa registrada dos itens já provados em produção.

### História 9.4: Credencial, local e assinantes por instalação

Como administrador, quero credencial MQTT, coordenadas, fuso e assinantes por instalação, para cada propriedade ter sua previsão e seus avisos.

**Critérios de aceite:**

**Dado** uma instalação com credencial própria
**Quando** ela tenta publicar fora do prefixo, ou com `retain` em `in/#`
**Então** o broker recusa
**E** Open-Meteo, boletim e horários usam as coordenadas e o fuso dela
**E** o convite do bot vincula o assinante à instalação.

### História 9.5: Canal de rádio por instalação

Como administrador, quero atribuir um canal de rádio por propriedade, para vizinhos não se ouvirem.

**Critérios de aceite:**

**Dado** uma instalação ativa no canal k a 5 km de outra
**Quando** tento cadastrar a outra no mesmo canal, ou no canal de fábrica k = 8
**Então** o cadastro é recusado, salvo força administrativa justificada e registrada.

### História 9.6: Papéis por instalação

Como administrador, quero operadores que veem só as propriedades deles, para dar acesso sem expor as outras.

**Critérios de aceite:**

**Dado** um operador vinculado a uma instalação
**Quando** ele acessa painel e API
**Então** vê e comanda só a instalação dele
**E** o administrador continua vendo todas
**E** o tratamento LGPD de chat_id, nomes, coordenadas e CAR está decidido (Q-8).

### História 9.7: Kit Hélio como segunda instalação

Como técnico, quero o Kit Hélio no mesmo painel, para acompanhar as duas propriedades num lugar.

**Critérios de aceite:**

**Dado** D-08 decidido como segunda instalação só leitura
**Quando** o gateway do Kit Hélio é alinhado ao gateway Horta (`dispatchCommand`, `parseTopicByte`, `reportCommandRejected`, `send(..., false)`)
**Então** o Nó 11 aparece na instalação dele, separado da Horta, sem comandos
**E** as duas redes nRF24 não operam ao alcance uma da outra, ou usam `MY_RF24_BASE_RADIO_ID` distintos.

---

## Épico 10: Rádio por propriedade e gateway seguro

O instalador configura o canal de rádio e protege o gateway no próprio portal. Os nós acompanham a troca de canal sem regravação, e um gateway novo nunca se passa por outro.

### História 10.1: Gateway sem prefixo de produção de fábrica

Como administrador, quero que um gateway novo ou resetado não publique em nenhum prefixo real, para ele nunca se passar pela estufa piloto.

**Critérios de aceite:**

**Dado** um gateway recém-gravado ou após factory reset
**Quando** ele sobe
**Então** fica em AP sem publicar até receber UF e código pelo portal
**E** o heartbeat leva o ID do chip, e outro ID no mesmo prefixo gera `gateway_conflict`.

### História 10.2: Piloto fora do canal de fábrica e canal no portal

Como instalador, quero gravar o canal de rádio no portal, para configurar a propriedade em campo.

**Critérios de aceite:**

**Dado** o byte 516 da EEPROM do nó e um campo novo em 521+ no gateway, documentados no spine (AD-12)
**Quando** gravo k = 0..63 no portal
**Então** o gateway sobe em `915,2 + 0,2·k` MHz e informa `radioChannel` no heartbeat
**E** todo binário sai de fábrica em k = 8
**E** a estufa piloto passa para um canal diferente de 8.

### História 10.3: Troca de canal dos nós com presença física

Como instalador, quero trocar o canal dos nós por comando, para não regravar cada um.

**Critérios de aceite:**

**Dado** nós no canal k e o instalador com ímã no local
**Quando** o backend envia `SET_RADIO:<k2>` como `V_CUSTOM` no child 0, durante a janela aberta pelo ímã
**Então** cada nó ecoa antes de trocar e reaparece no canal k2
**E** um nó que não acha o gateway em k2 volta para k
**E** fora da janela de presença o comando é ignorado.

### História 10.4: Autenticação do portal do gateway

Como administrador, quero o portal protegido por credencial, para ninguém na rede local reconfigurar ou apagar o gateway.

**Critérios de aceite:**

**Dado** o gateway em modo STA
**Quando** alguém acessa `/save` ou `/reset` sem credencial
**Então** o acesso é negado
**E** o modo AP de fábrica continua permitindo a primeira configuração.

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

- **Cobertura:** todos os FRs de FR-1 a FR-61 estão no mapa de cobertura e em pelo menos uma história.
- **Dependências:**
  - Nenhuma história depende de outra posterior no mesmo épico.
  - A 4.1 exige a Q-10 decidida e a spec ajustada.
  - A 3.10 exige as histórias 3.7 a 3.9.
  - A 6.5 exige as histórias 3.11 e 6.1 a 6.4.
  - As histórias de troca de posse (5.6, 6.5, 8.3) seguem o AD-6.
- **Banco:** cada tabela nasce na história que a usa. O log nasce na 4.2, o registro na 4.3, os comandos na 5.1, as regas na 6.1, o ciclo na 8.1 e a instalação na 9.1.
- **Starter:** não há starter template. A 4.1 cria o projeto depois de D-01 a D-05.
- **Prioridade P0:** FR-9, FR-10, FR-11, FR-25, FR-31, FR-32 e FR-34 têm critérios de aceite nas histórias 1.4, 3.8, 3.11, 5.1, 6.1, 6.2 e 6.5.
