# PRD Quality Review — Manejo360 Horta (M360), 2026-10-17

## Veredito geral
O PRD tem tese explícita ("a rega guiada por dados produz mais alface por litro de água que a rega por timer", §1), escopo honesto (§5 com dez não objetivos que fazem trabalho real, premissas indexadas) e FRs quase todos com consequência verificável e referência rastreável à spec do backend (as referências conferidas, como RF-CMD-05, RF-CMD-09, RF-ING-10, RF-CMD-12 e a contagem de 83 RFs, existem na spec v1.3). O risco está na ponte entre a tese e o MVP: o critério de sucesso (SM-1 "> 1" num único ciclo, com um canteiro por tratamento) não sustenta a decisão "o produto vale", e o MVP (E0+E1+E2) amarra a medição da tese à troca de toda a pilha sem dizer se a pilha pode mudar no meio do ciclo medido. Alguns FRs de firmware e o gesto do ímã no gateway ainda são vagos demais para virar história.

## Decision-readiness — adequate
As decisões estão escritas como decisões, não como "considerações": um gateway por propriedade, firmware por placa, fonte de clima no Nó 4, papel único `admin` na v1 (§2.1), separação por canal (FR-16) e o registro de "Vigente" contra os documentos antigos (addendum §2 e §4). Os não objetivos nomeiam o que se perde ("Sem internet, o motor do B para e o failsafe garante que nada fica ligado", §5), o que é um trade-off honesto. As questões em aberto (§9.1) são questões de fato, com coluna "Bloqueia?" e gatilho de revisão.

O ponto fraco é a decisão que o PRD diz existir para tomar: "o produto vale se ele passar de 1" (§1). Esse é o único critério de go/no-go do piloto, e ele é binário, sem margem, sobre uma amostra de um canteiro por tratamento e um ciclo (SM-1, "35–45 dias"). Diferença de posição entre canteiros, sombra ou um defeito de sonda produzem um Índice de 1,05 ou 0,95 sem dizer nada sobre a tese. A contra-métrica SM-C1 está certa, mas o piso dela está em aberto (Q-2).

### Findings
- **[high]** Regra de decisão da tese frágil (§1, SM-1, Q-2) — "Índice > 1" num único ciclo, n = 1 por tratamento, sem margem mínima nem tratamento de ruído entre canteiros, é o que decide se "o produto vale". Um resultado marginal não permite decidir nada, e o PRD não diz o que acontece nesse caso. *Fix:* definir um limiar com margem (p. ex. Índice ≥ 1,15 com massa do B ≥ X % da do A), dizer quantos ciclos são necessários ou se haverá troca de tratamento entre canteiros num segundo ciclo, e o que se faz com resultado inconclusivo.
- **[medium]** Q-10 bloqueia o MVP sem data (§9.1, §8.4) — sete decisões de tecnologia (D-01..03, D-05..08) "Bloqueia a fase 0 de E2", e E2 é parte do MVP. O gatilho "Antes da fase 0" é circular. *Fix:* dar data ou marco externo à Q-10 e listar no PRD (ou no addendum) a recomendação atual para cada D-xx, para que o dono decida em uma sessão.

## Substance over theater — strong
Pouca mobília. A Visão é específica do produto (estufa de alface em Brasília, A × B, Índice) e não troca para outro PRD. As NFRs de hardware têm valores ou proibições concretas ("915 MHz", "nem 12 V nem 220 V passam pela placa ACT", "relés normalmente abertos"), e onde não há número o PRD admite ("não estão definidos (Q-5, Q-6)", §8.2). Os não objetivos e o addendum §2 registram decisões reais.

### Findings
- **[medium]** Folga de RAM por adjetivo (§8.2, "Memória") — "cabe num ATmega328P (2 KB de RAM) com folga para a pilha. O Nó 99 é o caso crítico" não dá limite, embora a falta de folga já seja citada como risco em §8.4 (assinatura). Sem número, nenhuma história de firmware sabe quando reprova. *Fix:* fixar um piso de RAM livre medido no pior caso (p. ex. ≥ N bytes no Nó 99) e o método de medição.
- **[low]** Atores sem efeito em FR do MVP (§2.1) — "Desenvolvedor de firmware" e "Administrador" não mudam nenhum FR do MVP; o próprio texto diz que na v1 o administrador é o técnico. *Fix:* marcar como atores de E3/Futuro ou dizer que não guiam requisitos do MVP.

## Strategic coherence — adequate
A tese existe e parte das entregas a serve diretamente: F10 (A × B), F3 (volume) e F14 (indicadores) medem exatamente o que a tese pede, e as contra-métricas (§7.2) são boas, em especial SM-C1 e SM-C4, que vigiam a forma óbvia de "ganhar" o Índice com estresse hídrico.

O que falta é o argumento que liga a tese ao tamanho do MVP. A tese poderia em princípio ser testada já em E0, com volume por canteiro; o PRD define "MVP = E0 + E1 + E2" e "O piloto está completo quando a estufa roda nas placas M360, comandada pelo backend, e o ciclo de alface produz o Índice" (§6), sem dizer por quê. Há uma razão forte (comandos que o Node-RED dava como confirmados sem o relé ligar, desligamento não durável): sem confiabilidade de atuação o Índice mede falhas, não manejo. Mas ela não está escrita, e sem ela o MVP parece "tudo o que queremos construir". Pior: se E1 e E2 trocam hardware e software durante o ciclo medido, o próprio experimento fica confundido.

### Findings
- **[high]** MVP amarrado à tese sem regra de congelamento (§6, §1, Q-1, Q-11) — o PRD não diz se o ciclo que produz o Índice roda inteiro sobre uma pilha fixa, nem se um ciclo em curso no Node-RED/protótipos (Q-1: "já começou ou terminou?") conta. Trocar protótipo por placa M360 ou Node-RED pelo backend no meio do ciclo muda a atuação do A e do B. *Fix:* escrever que o ciclo de medição começa só com E1+E2 em operação (ou dizer qual pilha vale) e que nenhuma troca de hardware, firmware ou motor ocorre durante o ciclo; registrar por que o MVP inclui E1+E2 (confiabilidade de atuação como pré-condição da medição).
- **[low]** "Regressão" indefinida em SM-6 (§7.1) — "14 dias sem regressão depois de desligar o Node-RED" não diz o que conta como regressão. *Fix:* ligar a SM-2, SM-3 e SM-4 (p. ex. "14 dias com SM-2 ≥ 99 %, SM-3 = 0, SM-4 = 288/dia").

## Done-ness clarity — adequate
A maioria dos FRs tem consequência testável e boa: FR-2 ("a tensão nas sondas é zero"), FR-25 ("com o nó desligado, o comando termina em 'falhou'"), FR-26 ("cerca de 35 s", coerente com RF-CMD-05 e a fase 3 da spec §10), FR-31 ("matar o processo durante uma rega resulta em exatamente um desligamento"), FR-47 ("401"). Onde o detalhe vai para a spec, a referência resolve. O problema está concentrado no firmware e em dois FRs de painel.

### Findings
- **[medium]** FRs com consequência vaga ou que não testa o requisito (F1, F3, F5, F6) — FR-3 "de forma legível"; FR-12 "passa a maior parte do tempo dormindo" (sem corrente média nem fração de sono); FR-13 "abaixo de um limiar seguro" sem limiar, e a consequência ("a bateria aparece em % no painel") não testa a parada de transmissão nem a ausência de laço de reset; FR-6 e FR-15 têm consequências que são artefatos ("é publicada em L/min", "existe uma variante repetidora"), não comportamento. *Fix:* FR-13 com tensão de corte e teste (fonte variável descendo até o corte, sem reset em 1 h); FR-12 com corrente média alvo ou fração de tempo em sono; FR-15 com "um nó fora do alcance direto do gateway entrega pelo repetidor"; FR-3 apontando a regra de conversão de RF-ING-06.
- **[medium]** Gesto do ímã no gateway ambíguo e aviso nem sempre possível (UJ-4, FR-18, FR-23) — o mesmo ímã "no boot" abre o modo AP (FR-18) e "restaura a configuração de fábrica" (FR-23). O comportamento real depende de janela de 30 s após o boot, soltar e manter 3 s; nada disso está no PRD. E "avisando a plataforma antes de apagar" só ocorre com MQTT conectado: em modo AP ou com broker fora não há evento, e a consequência "a plataforma marca o gateway como resetado, e não como falha" deixa de valer. *Fix:* descrever os dois gestos com tempos e escrever a consequência para o caso sem MQTT.
- **[medium]** FR-44 aponta para alvo móvel (F13) — "o painel cobre tudo o que o dashboard do Node-RED mostra hoje" depende de um servidor que está à frente do repositório e muda. *Fix:* tratar RF-UI-02..08 como lista fechada (ou listar as telas atuais no addendum) e retirar o "hoje".
- **[low]** FR-50 sem formato nem escopo (F14) — "podem ser exportados" não diz formato, período nem granularidade. *Fix:* p. ex. CSV por tabela, filtro por ciclo, timestamps em UTC.

## Scope honesty — strong
É a dimensão mais forte. §5 tem não objetivos que fazem trabalho real (operação local, kPa, OTA, Multi-CAR, migração do Nó 4), cada um com o motivo. Cada feature traz o estado (em produção, implementado sem bancada, especificado, futuro), o que separa o que existe do que é promessa. As premissas estão indexadas (§9.2) e as que mais pesam (P-3, P-6, P-7) estão marcadas inline. A densidade de itens abertos (12 Q + 7 P) é compatível com lançamento interno; só a Q-10 bloqueia, e está marcada como tal.

### Findings
- **[low]** Premissas sem marca inline (§9.2) — P-1, P-2 e P-5 estão no índice, mas não aparecem como **[PREMISSA]** nos locais citados (§0, §1/F10, §3), contrariando o que §0 promete ("Itens marcados [PREMISSA] foram inferidos"). *Fix:* marcar inline ou corrigir a coluna "Onde aparece".

## Downstream usability — adequate
IDs contíguos e únicos (FR-1..60, UJ-1..6, SM-1..7, SM-C1..C5, Q-1..12, P-1..7). Glossário presente e útil, com a resolução da confusão histórica A/B. As referências à spec funcionam como âncoras de extração. O que atrapalha é o substantivo central.

### Findings
- **[medium]** "Plataforma" não definida (§3, F9–F14) — os FRs de F9 a F14 dizem "a plataforma", mas o estado mistura Node-RED em produção e backend especificado. Uma história de E2 não sabe se, p. ex., FR-27 (fila FIFO, expiração em 24 h) ou FR-5 precisam valer também no Node-RED durante a convivência, ou só no backend. *Fix:* definir "plataforma" no glossário (o backend de E2; o Node-RED é a implementação de E0, sem obrigação de cumprir os FRs) e incluir "motor agroclimático / motor do B" e "fase N (spec §10)", também usados sem definição.
- **[low]** Referências a D-xx e fases que só resolvem na spec (FR-4, FR-16, FR-57, §6, SM-2, SM-6, Q-1) — aceitável pela delegação deliberada, mas um leitor de épico precisa abrir outro documento para entender um alvo de métrica. *Fix:* uma tabela curta no addendum com as fases 0–7 e os D-xx citados.

## Shape fit — strong
Forma adequada a um piloto interno brownfield com operador único: jornadas curtas e concretas, cada uma com protagonista nomeado, sem densidade de UJ excessiva; métricas operacionais em vez de engajamento; capacidades agrupadas por feature com referência à spec normativa. As referências ao sistema existente que conferi estão corretas (`M360-Config`, failsafe no nó, payload "0"/"1", Nó 4 como fonte de clima).

### Findings
- **[medium]** Estado por feature esconde estado por FR (F2, F9) — F2 está "em produção", mas FR-5 (reserva pela previsão, "nenhuma avaliação roda com valor padrão silencioso") contraria o comportamento atual do Node-RED, que cai em 20 °C / 20 % sem leitura. Do mesmo modo, FR-27 (fila FIFO) é o oposto do Node-RED atual, que guarda um comando por nó e sobrescreve. Quem planeja épicos pode tratar como pronto o que é novo. *Fix:* marcar estado por FR quando diverge do da feature, como já é feito em F3 ("vazão em produção; volume persistente especificado").

## Mechanical notes
- IDs: sem lacunas nem duplicatas em FR, UJ, SM, SM-C, Q e P. As referências internas (Q-2 em SM-C1, P-3 em F14, P-6 em FR-56, P-7 em SM-2/SM-5, Q-10 em §8.4) resolvem.
- Referências externas conferidas na spec v1.3: 83 RFs; RF-CMD-05, RF-CMD-09, RF-CMD-12, RF-ING-10, RF-PRV-11, RF-PRV-12, RF-UI-08, RF-VOL-03, RF-WDG-03, RF-IRR-10, RF-NOT-08, RNF-12, D-04 e D-09 existem.
- Roundtrip de premissas: P-1, P-2 e P-5 sem marca inline (ver Scope honesty).
- Glossário: "técnico / pesquisador" (§2.1) aparece como "técnico" em UJ-1..3 e "pesquisador" em UJ-6 e FR-49; vale uma linha dizendo que é a mesma pessoa. "Versão seguinte" e "E3" são usados como sinônimos (FR-16, F15, UJ-5).
- Não há `[NOTE FOR PM]`; as tensões estão nas Q, o que basta para este nível de risco.
