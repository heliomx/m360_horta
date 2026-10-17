---
title: "PRD — Manejo360 Horta (M360)"
status: final
created: 2026-10-17
updated: 2026-10-17
supersedes:
  - _bmad-output/planning-artifacts/prd.md (2026-06-21)
  - docs/prd.md (2026-06)
normative_companions:
  - docs/especificacao-backend-m360.md (v1.3) — requisitos detalhados do backend
  - hardware/ARQUITETURA_FAMILIA.md — família de placas M360
  - src/DRY/horta/inventario.md — contrato de nós e children com a plataforma
---

# PRD — Manejo360 Horta (M360)

## 0. Propósito do documento

Este PRD descreve **o produto inteiro** do projeto M360 Horta. Abrange os nós de campo e o gateway, a família de placas M360, a automação que hoje roda no Node-RED e o backend que vai substituí-lo. Ele substitui os dois PRDs de junho de 2026, que tinham sido escritos por engenharia reversa e divergiam entre si e do sistema real.

O PRD fixa **o quê** e **por quê**. O **como** fica nos documentos normativos que ele referencia:

| Assunto | Documento normativo | O que o PRD faz com ele |
|---|---|---|
| Backend (RF-ING, RF-CMD, RF-IRR…) | `docs/especificacao-backend-m360.md` v1.3 | Agrupa os 83 RFs em capacidades e herda os critérios de aceite |
| Placas | `hardware/ARQUITETURA_FAMILIA.md` | Herda decisões e pendências de bancada |
| Nós e children | `src/DRY/horta/inventario.md` | Herda o contrato; nenhum child ID é redefinido aqui |
| Motor agroclimático | `docs/motor_agroclimatico_manejo360.md` + spec §7.3 | Herda a lógica das cinco camadas |

Detalhe técnico que não cabe aqui está em `addendum.md`. Itens marcados **[PREMISSA]** foram inferidos e estão listados na §9.2.

**Leitores:** o dono do projeto, quem implementa o backend e o firmware, e quem planeja os épicos. O rigor segue o de um lançamento interno: alto na segurança da atuação, médio no resto [PREMISSA P-1].

## 1. Visão

O **Manejo360** é um extensor rural digital: sensores de solo, clima e vazão ligados por rádio a uma plataforma que decide quando e quanto irrigar e avisa o técnico pelo celular. O **M360 Horta** é o piloto, numa estufa de alface em Brasília/DF com dois canteiros lado a lado.

O piloto existe para provar uma tese: **a rega guiada por dados produz mais alface por litro de água que a rega por timer.** O Canteiro A é regado por timer fixo; o Canteiro B, pelo motor agroclimático do Manejo360. A razão entre as eficiências hídricas é o **Índice Manejo360**, e o produto vale se ele passar de 1 sem perda de produção. A estufa continua com dois canteiros e alface como cultura de referência [PREMISSA P-2].

**Regra de decisão.** Um ciclo tem um canteiro por tratamento, então um Índice perto de 1 não decide nada. Se o Índice ficar abaixo do limiar de conclusão (Q-2), o resultado é inconclusivo e o ciclo seguinte troca os tratamentos de canteiro, para separar o efeito do manejo do efeito da posição [PREMISSA P-8].

A estratégia tem três tempos:

1. **Agora:** fechar o piloto com hardware próprio e confiável. As placas M360 substituem os Arduinos de protótipo, e um backend auditável substitui o Node-RED.
2. **Depois:** levar a mesma pilha a outras propriedades sem retrabalho de engenharia. Isso exige comissionamento guiado, isolamento de rádio entre vizinhos e um gateway por propriedade.
3. **Futuro:** pareamento de nós sem regravar firmware, com o backend definindo identidade e perfil.

## 2. Usuários-alvo

### 2.1 Jobs-to-be-done

| Ator | Job | Canal |
|---|---|---|
| **Técnico / pesquisador** (operador principal) | Saber se a estufa está regando certo sem ir até ela; agir quando algo falha; tirar os números do experimento | Painel web + Telegram |
| **Produtor** | Ser avisado do que acontece na estufa e confiar que a rega não falha | Telegram (só recebe e consulta) |
| **Instalador** | Montar gateway e nós, configurar e provar que tudo funciona antes de entregar | Portal do gateway, ímã, checklist |
| **Administrador** | Cadastrar propriedades, ajustar parâmetros, gerir acesso | Painel web |
| **Desenvolvedor de firmware** | Criar um nó novo sem reescrever o ciclo de vida | Lib M360-DRY, workflow `m360-node-factory` |

Técnico e pesquisador são a mesma pessoa no piloto; as jornadas usam um ou outro nome conforme a tarefa. Na v1 há **um único papel, `admin`**, exercido por essa pessoa. Administrador distinto e operadores por instalação vêm com a multi-propriedade (E3). O desenvolvedor de firmware não guia requisitos do MVP; está listado porque o workflow de criação de nós é parte do produto.

### 2.2 Não usuários

- **Produtor operando pelo painel web.** Ele não opera; só recebe e consulta pelo Telegram.
- **Agrônomo como usuário do sistema.** As referências agronômicas (Embrapa, Incaper, FAO-56) entram como parâmetros do motor, não como alguém que usa o produto.
- **Terceiros com acesso ao broker MQTT.** O contrato MQTT é interno.

### 2.3 Jornadas

**UJ-1 — Dia comum de rega (técnico).** Às 07:00 o Canteiro A rega por 5 minutos. Ao longo do dia o motor avalia o Canteiro B a cada 5 minutos. Quando o solo do B seca e nenhuma trava está ativa (chuva prevista, insolação, risco de orvalho), o motor abre a solenóide. O Telegram avisa "IRRIGAÇÃO INICIADA" com duração e motivo, e depois "FINALIZADA" com o volume. O técnico só abre o painel se algo destoa.

**UJ-2 — Algo falhou (técnico).** Um nó para de transmitir, ou o gateway cai. O técnico recebe um único alerta, com o motivo e o horário. Um comando manual que o nó não confirma vira "falhou" depois de três tentativas, também com alerta. Ninguém precisa descobrir a falha olhando um gráfico parado.

**UJ-3 — Rega manual (técnico).** No painel, o técnico escolhe o canteiro e a duração. O botão fica âmbar até o nó confirmar e verde depois. Uma contagem regressiva mostra o tempo restante, com botão de parar. O desligamento acontece mesmo que o servidor reinicie no meio.

**UJ-4 — Manutenção sem abrir a caixa (instalador).** Encostar o ímã na caixa de um nó de solo força uma leitura imediata. Num atuador, que fica dentro do quadro elétrico, o instalador abre a porta do quadro e mantém o ímã 3 s na caixa da placa: a ligação com o gateway é refeita sem reiniciar o microcontrolador. No gateway, o ímã restaura a configuração de fábrica.

**UJ-5 — Comissionar uma propriedade nova (instalador e administrador, versão seguinte).** O instalador liga o gateway em modo de configuração e grava WiFi, broker, UF, CAR e canal de rádio. A plataforma detecta o tráfego e cria a instalação como pendente. O administrador confirma os dados. Um checklist de comissionamento, com evidência por item, libera as regras de rega.

**UJ-6 — Fechar o ciclo do experimento (pesquisador).** Ao fim do ciclo de alface, o pesquisador pesa a colheita de cada canteiro e lança os dados. A plataforma já tem os litros por canteiro e entrega a eficiência hídrica de cada um e o Índice Manejo360.

## 3. Glossário

| Termo | Significado |
|---|---|
| **Canteiro A** | Parcela de controle: rega por timer fixo, sem sensores na decisão (solenóide no child 31 do Nó 99) |
| **Canteiro B** | Parcela Manejo360: rega pelo motor agroclimático (child 32). Documentos antigos o chamam de "Canteiro M" e trocam A por B; vale esta definição [PREMISSA P-5] |
| **Plataforma** | O software que decide e registra: o Node-RED na E0 e o backend a partir da E2. Os FRs de F9 a F14 obrigam o **backend**; o Node-RED não precisa cumpri-los durante a convivência |
| **Motor do B** | Motor agroclimático do Canteiro B, avaliado a cada 5 minutos (spec §7.3) |
| **Fase N** | Fase 0 a 7 da migração do Node-RED para o backend (spec §10; resumo no addendum §6) |
| **Índice Manejo360** | Eficiência hídrica do B (kg/L) dividida pela do A no mesmo ciclo |
| **Nó** | Dispositivo de campo MySensors com ID fixo (1, 2, 4, 99; 11 no Kit Hélio) |
| **Child** | Sensor ou atuador dentro de um nó; os IDs 253, 254 e 255 são reservados |
| **Gateway** | Ponte rádio ↔ MQTT; um por propriedade (UF+CAR) |
| **Eco de aplicação** | Mensagem que o nó envia depois de aplicar um comando; é a única confirmação real |
| **ACK de transporte** | Confirmação do próximo salto de rádio; não prova que o nó aplicou o comando |
| **Failsafe** | Tempo máximo ligado aplicado pelo próprio nó atuador, independente da plataforma |
| **Caixa postal** | Fila de comandos para nós que dormem, entregue quando o nó acorda |
| **Instalação** | Uma propriedade na plataforma, identificada pelo prefixo `m360/{UF}/{CAR}` |
| **Placas M360** | Família própria: RF (mezanino de rádio), LP (nó sensor), ACT (nó atuador), GW (gateway) |
| **Protótipos** | Nano, Pro Mini e D1 mini com nRF24, em campo hoje |
| **Solo (escala)** | ADC bruto 0–1023; valor alto significa solo seco |

## 4. Funcionalidades

Cada funcionalidade traz o **estado** atual:

- **Em produção:** roda hoje na estufa.
- **Implementado, sem bancada:** compila nas placas M360, mas nenhuma placa foi montada.
- **Especificado:** existe só na especificação.
- **Futuro:** fora das três primeiras entregas.

"Ref." aponta o requisito detalhado no documento normativo. Quando um FR diverge do estado da sua funcionalidade, ele traz o próprio estado.

**Prioridade P0 (segurança da atuação):** FR-9, FR-10, FR-11, FR-25, FR-31, FR-32 e FR-34. Nenhuma entrega pode regredir nesses FRs.

### F1 — Umidade de solo multiponto
*Estado:* em produção nos protótipos; implementado, sem bancada, na M360-LP.

- **FR-1** Cada canteiro tem um nó com seis sondas, em três distâncias e duas profundidades. O nó lê a cada ciclo e publica a leitura bruta de cada sonda quando o valor muda e, no mínimo, a cada 10 ciclos.
  - *Consequência:* o painel mostra seis séries por canteiro, identificadas pelo rótulo do child. Ref.: inventário §4–§5.
- **FR-2** As sondas só recebem energia durante a leitura, para evitar eletrólise.
  - *Consequência:* fora da varredura, a tensão nas sondas é zero.
- **FR-3** A plataforma exibe a umidade do solo de forma legível sem perder o valor bruto, que é o que o motor usa.
  - *Consequência:* cada leitura guarda o bruto e o convertido, com a conversão derivada do tipo do child (`S_MOISTURE`). Ref.: RF-ING-06.

### F2 — Clima
*Estado:* em produção (Nó 4), exceto o FR-5.

- **FR-4** Um nó a energia solar, instalado fora da estufa, mede temperatura e umidade do ar e temperatura do solo a cada 30 minutos. Ele é a **fonte de clima** do motor do B, do relatório diário e do `/status`.
  - *Consequência:* trocar a fonte de clima exige mudar só configuração. Ref.: D-04.
- **FR-5** *(Novo; hoje o Node-RED cai em 20 °C e 20 % sem aviso.)* Sem leitura de clima recente (mais de 1 h), o motor usa a previsão meteorológica como reserva e registra que usou.
  - *Consequência:* nenhuma avaliação roda com valor padrão silencioso. Ref.: spec §7.3.

### F3 — Vazão e volume
*Estado:* vazão em produção; volume persistente especificado.

- **FR-6** O nó atuador mede a vazão da linha de irrigação.
  - *Consequência:* a leitura da vazão não perde pulsos enquanto o nó atende rádio e comandos, e é publicada em L/min.
- **FR-7** A plataforma integra a vazão em volume acumulado que sobrevive a reinícios, sem inventar consumo em lacunas, e atribui volume a cada rega.
  - *Consequência:* reiniciar o servidor não altera o acumulado. Ref.: RF-VOL-01..03, RF-IRR-10.

### F4 — Atuação segura
*Estado:* em produção no protótipo; implementado, sem bancada, na M360-ACT.

- **FR-8** O nó atuador comanda até nove cargas: três solenóides, quatro peristálticas e duas bombas.
  - *Consequência:* cada carga é um child endereçável. Ref.: inventário §7.
- **FR-9** Entre solenóides e peristálticas, só uma fica ligada por vez. Ligar outra desliga a anterior, e o nó avisa o desligamento antes de confirmar a nova.
  - *Consequência:* nunca há dois canais do MUX ativos.
- **FR-10** O próprio nó desliga cada solenóide e cada peristáltica ao atingir o tempo máximo ligado, mesmo sem gateway, rede ou plataforma. As bombas NFT (38 e 39) não têm limite, porque a hidroponia exige operação contínua.
  - *Consequência:* uma solenóide ligada e esquecida desliga sozinha. Bomba ligada sem supervisão é risco aceito (§8.4).
- **FR-11** Em reset, queda de energia ou falha, nenhuma carga liga sozinha.
  - *Consequência:* durante o boot da ACT nenhum relé fecha.

### F5 — Energia
*Estado:* em produção (fonte fixa e Nó 4 solar); modo bateria da LP implementado, sem bancada.

- **FR-12** Cada nó opera em um de quatro perfis: fonte fixa, bateria com sono, passivo (lê só sob comando) ou repetidor. O perfil é escolhido na configuração do nó.
  - *Consequência:* um nó de bateria só fica acordado para ler, transmitir e esperar a janela de comandos; a corrente média alvo depende da meta de autonomia (Q-5).
- **FR-13** Nós a bateria reportam a carga no child 255. Na M360-LP, abaixo de 3,0 V medidos com correção pelo bandgap, o nó para de transmitir e só dorme, em vez de entrar em laço de reset. O Nó 4 protótipo não tem esse corte.
  - *Consequência:* com uma fonte variável descendo até o corte, a LP para de transmitir e não reinicia em 1 h. Ref.: `ARQUITETURA_FAMILIA.md` §9 (fuses ainda a confirmar por ISP).

### F6 — Rede de rádio
*Estado:* nRF24 em produção; LoRa 915 MHz implementado, sem bancada.

- **FR-14** As placas M360 aceitam LoRa 915 MHz ou nRF24, um rádio por placa, escolhido na compilação.
  - *Consequência:* o mesmo fonte gera as duas variantes.
- **FR-15** Um nó atuador pode operar como repetidor para cobrir propriedades maiores que o alcance direto.
  - *Consequência:* um nó fora do alcance direto do gateway entrega pelo Nó 99 repetidor.
- **FR-16** O isolamento de rádio entre propriedades vizinhas é feito por canal de frequência, um por propriedade, de uma grade fixa. *(Versão seguinte)*
  - *Consequência:* dois gateways a menos de 10 km nunca recebem o mesmo canal. Ref.: D-09; RF-PRV-12.

### F7 — Gateway e configuração em campo
*Estado:* em produção no protótipo; implementado, sem bancada, na M360-GW.

- **FR-17** O gateway faz a ponte entre o rádio e o broker MQTT da propriedade. Valida cada comando antes de transmiti-lo e publica o motivo de cada recusa.
  - *Consequência:* um comando malformado gera o evento `command_rejected`, nunca um relé acionado por engano.
- **FR-18** Sem configuração válida, ou com o ímã encostado durante o boot, o gateway abre um ponto de acesso de configuração (`M360-Config`) com formulário de WiFi, broker, UF e CAR. Segurar o ímã desde o boot nunca apaga a configuração.
  - *Consequência:* configurar um gateway não exige regravar firmware.
- **FR-19** O gateway publica heartbeat periódico e eventos de nó descoberto, perdido e reconectado.
  - *Consequência:* a plataforma sabe a idade do último contato de cada nó.
- **FR-20** LEDs indicam o estado de WiFi e MQTT sem ferramenta.
  - *Consequência:* verde significa WiFi e MQTT ok.

### F8 — Manutenção sem abrir a caixa
*Estado:* implementado, sem bancada nas placas M360 (firmware de 2026-10-17). O factory reset também roda nos gateways protótipo.

- **FR-21** No nó sensor, o ímã força leitura e envio imediatos de todos os sensores e acorda o nó se ele estiver dormindo.
  - *Consequência:* a leitura chega em segundos, e o despertar abre a janela da caixa postal. Ref.: RF-CMD-09.
- **FR-22** No nó atuador, com a porta do quadro aberta, o ímã mantido 3 s na caixa da placa refaz a ligação com o gateway sem reiniciar o microcontrolador e sem mexer nos relés. O estado dos relés não é reenviado.
  - *Consequência:* uma solenóide aberta continua aberta. Ref.: RF-ING-10.
- **FR-23** No gateway já em operação, nos primeiros 30 s depois do boot, afastar o ímã e mantê-lo 3 s restaura a configuração de fábrica e reinicia em modo de configuração. O botão "Restaurar padrões de fábrica" do portal faz o mesmo. Com MQTT conectado, o gateway publica `factory_reset` antes de apagar.
  - *Consequência:* com MQTT conectado, a plataforma marca o gateway como resetado, e não como falha. Sem MQTT (modo de configuração ou broker fora) não há evento, e a plataforma vê só um gateway offline. Ref.: RF-PRV-09.

### F9 — Comandos com confirmação
*Estado:* em produção no Node-RED, com falhas conhecidas; especificado no backend.

- **FR-24** Todo comando a um nó é um registro auditável com estado: pendente, enviado, confirmado, falhou, rejeitado ou expirado. O registro guarda a origem (usuário, regra ou reconciliador) e o horário de cada transição.
  - *Consequência:* qualquer comando falho é rastreável em até três consultas. Ref.: RF-CMD-01, RF-CMD-10, RNF-05.
- **FR-25** Uma atuação só é confirmada pelo **eco do próprio nó**, com o valor pedido. O ACK de transporte não confirma.
  - *Consequência:* com o nó desligado, o comando termina em "falhou", nunca em "confirmado". Ref.: RF-CMD-04.
- **FR-26** Sem eco, a plataforma tenta três vezes, marca a falha e alerta.
  - *Consequência:* a falha de última milha aparece em cerca de 35 s. Ref.: RF-CMD-05.
- **FR-27** *(Novo; hoje a caixa postal guarda um comando por nó e sobrescreve.)* Comandos a nós que dormem esperam numa fila por nó e são entregues no próximo despertar, expirando em 24 h.
  - *Consequência:* nenhum comando é sobrescrito sem aviso. Ref.: RF-CMD-09.
- **FR-28** O técnico pode, à distância, mudar o intervalo de leitura, forçar leitura, pedir reapresentação e pedir diagnóstico de rede de qualquer nó.
  - *Consequência:* a mudança de intervalo é confirmada pela leitura do próprio nó. Ref.: RF-CMD-02, RF-CMD-07.
- **FR-61** A plataforma nunca tem dois comandos em voo para os canais do MUX do Nó 99, e um eco de desligamento não pedido (failsafe ou preempção) encerra a rega ativa daquele canal.
  - *Consequência:* o estado de cada atuador na plataforma segue o que o nó informa, inclusive quando o nó desliga sozinho. Ref.: RF-CMD-08, RF-CMD-11.

### F10 — Irrigação A × B
*Estado:* em produção no Node-RED; especificado no backend.

- **FR-29** O Canteiro A rega por horário fixo (07:00 e 17:00, 300 s), sem nenhuma inteligência. Se o A é suspenso em dia de chuva, como pede o protocolo de testes, é a Q-14.
  - *Consequência:* o controle do experimento é reprodutível. Ref.: RF-IRR-03.
- **FR-30** O motor agroclimático avalia o Canteiro B a cada 5 minutos. As camadas são tempo mínimo entre regas, mediana dos sensores, travas de chuva, insolação e orvalho, lâmina estimada a partir do déficit do solo em ADC com um fator atmosférico, e duração mínima. Solo crítico se sobrepõe às travas: com mediana ≥ 700 a rega acontece mesmo com chuva prevista, e com mediana ≥ 650 insolação e orvalho não bloqueiam. Cada avaliação é registrada com as entradas e a camada que decidiu.
  - *Consequência:* toda decisão do motor pode ser explicada depois. Ref.: RF-IRR-04, RF-IRR-05.
- **FR-31** O desligamento de toda rega é agendado de forma durável e acontece mesmo se a plataforma reiniciar no meio.
  - *Consequência:* matar o processo durante uma rega resulta em exatamente um desligamento. Ref.: RF-IRR-01, RNF-01.
- **FR-32** A duração de qualquer rega fica abaixo do failsafe do nó, e um reconciliador desliga o que o agendador perder.
  - *Consequência:* o failsafe do firmware é a última barreira, não a primeira. Ref.: RF-IRR-02, RF-IRR-07.
- **FR-33** A rega manual passa pelo mesmo caminho da automática: registro, limite, desligamento durável e aviso.
  - *Consequência:* não há atalho que ignore o limite. Se o comando de abertura falha, a rega termina como falha, o desligamento agendado é cancelado e o Telegram avisa. Ref.: RF-IRR-06, RF-IRR-08.
- **FR-34** As regras de cada canteiro podem ser ligadas e desligadas sem reimplantar, e um canteiro nunca é comandado por dois sistemas ao mesmo tempo.
  - *Consequência:* durante a migração do Node-RED para o backend, só um dos dois comanda as solenóides. Ref.: RF-IRR-09.

### F11 — Registro, histórico e vigilância da rede
*Estado:* em produção no Node-RED, parcial e sem persistência; especificado no backend.

- **FR-35** A plataforma ingere todo o tráfego do gateway num log auditável, inclusive os eventos que hoje são descartados.
  - *Consequência:* o log fica retido por 30 dias. Ref.: RF-ING-01..10.
- **FR-36** A lista de nós e sensores é montada a partir do que os nós anunciam, sem lista fixa no código.
  - *Consequência:* um nó novo aparece sozinho. Ref.: RF-REG-01..13.
- **FR-37** Um critério único define se cada nó e o gateway estão online, e é o mesmo no painel, na API e nos alertas.
  - *Consequência:* com o gateway offline, todo dado aparece como "sem contato". Ref.: RF-REG-10, RF-WDG-01..03.
- **FR-38** As leituras ficam guardadas sem limite inicial e consultáveis em janelas longas.
  - *Consequência:* uma consulta de 7 dias responde em menos de 1 s. Ref.: RNF-09, RNF-12.

### F12 — Avisos pelo Telegram
*Estado:* em produção no Node-RED; especificado no backend.

- **FR-39** Alertas de rede, de comando e de rega chegam ao Telegram uma vez por transição, com tempo de espera contra repetição.
  - *Consequência:* uma queda gera um alerta, não dezenas. Ref.: RF-NOT-03..05.
- **FR-40** Um bot responde a consultas de status e de previsão do tempo e aceita novos assinantes por convite.
  - *Consequência:* o produtor consulta a estufa sem painel. Ref.: RF-NOT-01, RF-NOT-02.
- **FR-41** Um boletim meteorológico sai quatro vezes por dia, e um relatório diário às 08:00 compara as últimas 24 h com o dia anterior.
  - *Consequência:* sem o modelo de linguagem, o relatório sai só com números, mas sai. Ref.: RF-NOT-06, RF-NOT-07.
- **FR-42** Toda notificação é registrada, e uma falha de envio não interrompe o fluxo que a originou.
  - *Consequência:* uma rega continua mesmo se o Telegram falhar. Ref.: RF-NOT-08.

### F13 — Painel web e API
*Estado:* dashboard Node-RED em produção; painel e API especificados.

- **FR-43** O painel tem botões por atuador cuja cor segue o estado real do comando, com confirmação nas peristálticas.
  - *Consequência:* o botão só fica verde com o eco do nó. Ref.: RF-UI-01.
- **FR-44** O painel mostra gráficos de 7 dias com zoom, mapa da rede, tabela de vigilância, log MQTT com exportação, regas em andamento e histórico, e a lista de comandos.
  - *Consequência:* o painel cobre a lista fechada de RF-UI-02 a RF-UI-08, que é a paridade com o dashboard do Node-RED (spec §12.3). Trocar a placa do Nó 99 pela ACT tira o gráfico "Clima (Nó 99)", porque a ACT não tem DHT. Ref.: RF-UI-02..08.
- **FR-45** Os parâmetros do motor e das regras são editáveis no painel, com trilha de quem alterou o quê.
  - *Consequência:* toda alteração fica auditada. Ref.: RF-UI-07, RF-PRV-11.
- **FR-46** Uma API autenticada e documentada expõe leituras, comandos, regas e estado, com fluxo em tempo real.
  - *Consequência:* a API é o contrato para integrações e para o futuro agente conversacional. Ref.: RF-API-01..03.
- **FR-47** Painel e API exigem autenticação.
  - *Consequência:* uma chamada sem token recebe 401. Ref.: RNF-08.

### F14 — Indicadores do experimento
*Estado:* hoje manual; **novo neste PRD** [PREMISSA P-3].

- **FR-48** A plataforma totaliza litros por canteiro por dia e por ciclo, a partir do volume atribuído a cada rega.
  - *Consequência:* o volume de A e B no ciclo sai de uma consulta.
- **FR-49** O pesquisador registra o ciclo (datas de transplante e colheita), as perdas semanais, e na colheita a massa fresca por canteiro e por variedade, o peso médio de 10 cabeças por variedade e a nota visual de 1 a 5. A plataforma calcula a eficiência hídrica (kg/L) de cada canteiro, o Índice Manejo360 e a economia de água em %.
  - *Consequência:* o Índice sai sem planilha externa.
- **FR-50** Os dados do experimento (leituras, regas, volumes, avaliações do motor) podem ser exportados.
  - *Consequência:* exportação em CSV por tabela, filtrada por ciclo, com horários em UTC; a análise pode ser refeita fora da plataforma.

### F15 — Multi-propriedade *(versão seguinte)*
*Estado:* especificado (spec §15); no firmware, só o factory reset está pronto.

- **FR-51** Uma instância da plataforma atende várias propriedades com isolamento total de nós, comandos, regras e alertas.
  - *Consequência:* não há vazamento entre instalações. Ref.: RF-ING-09.
- **FR-52** Tráfego de um gateway desconhecido cria uma instalação pendente, que ingere dados mas não comanda nem alerta até ser confirmada.
  - *Consequência:* um gateway novo nunca recebe comando antes da confirmação. Ref.: RF-PRV-03.
- **FR-53** A instalação passa pelos estados pendente, comissionando, ativa e suspensa; o gateway de uma instalação pode ser marcado como resetado. As regras só rodam em ativa, depois de um checklist com evidência por item.
  - *Consequência:* comissionar não exige editar arquivo nem recompilar. Ref.: RF-PRV-01..06, spec §15.4.
- **FR-54** Cada instalação tem credencial MQTT própria restrita ao seu prefixo, além de coordenadas, fuso, assinantes do Telegram e canal de rádio próprios.
  - *Consequência:* a previsão, os horários e os alertas são os da instalação. Ref.: RF-PRV-07, RF-PRV-08, RF-PRV-10, RF-PRV-12.
- **FR-55** O gateway aceita o canal de rádio no portal e o informa no heartbeat. Os nós trocam de canal por comando, confirmando antes de trocar.
  - *Consequência:* o canal é configurável em campo. Ref.: spec §15.6.
- **FR-56** O portal do gateway exige autenticação [PREMISSA P-6].
  - *Consequência:* ninguém na rede local reconfigura ou apaga um gateway sem credencial.
- **FR-57** Há papéis por instalação: o operador vê e comanda só as propriedades dele [PREMISSA P-10].
  - *Consequência:* o administrador continua vendo tudo. A spec (D-06) prevê só o papel `admin`; os papéis por instalação estendem a recomendação.

### F16 — Pareamento e firmware genérico *(futuro)*
*Estado:* futuro. Era o spine da branch `feat/firmware-generico-autoconfig`; o addendum §3 o revisa contra as decisões vigentes.

- **FR-58** Um nó novo entra na rede por pareamento guiado, numa janela aberta pelo portal ou pela plataforma, e recebe ID e perfil do backend.
  - *Consequência:* instalar um nó não exige um env por nó.
- **FR-59** O perfil do nó (sensores ativos, cadência) fica na memória do nó e é alterável à distância.
  - *Consequência:* trocar o perfil de um nó não exige regravá-lo.
- **FR-60** Um nó pode ser realocado por comando remoto, sem abrir a caixa.
  - *Consequência:* o reset remoto usa um child administrativo próprio, e não o 254, que é o do intervalo.

## 5. Não objetivos

- **Operação local sem internet.** Sem internet, o motor do B para e o failsafe garante que nada fica ligado. Rodar a rega no gateway ou num servidor local não é objetivo.
- **Gatilho em kPa.** As sondas são resistivas e o sistema decide em ADC bruto. Calibração para kPa ou umidade volumétrica não está no escopo [PREMISSA P-4].
- **RS485/Modbus, sensores ZTS, pH e oxigênio** no nó atuador ou nas placas novas.
- **Hidroponia NFT e Canteiro C como parcelas com regra.** Os atuadores existem e ficam só manuais.
- **Gateway compartilhado entre propriedades** (Multi-CAR), descartado em 2026-09-10.
- **LoRaWAN** no gateway.
- **ESP8266/ESP32 em nós de campo.** Ficam restritos ao gateway.
- **Migrar o Nó 4 e o nó do Kit Hélio para placas M360.** Para o Nó 4, este não objetivo é reavaliado pela Q-16, porque o corte para LoRa depende dele.
- **Atualização de firmware por rádio (OTA).** A gravação é feita na bancada.
- **App móvel nativo.** O painel é web responsivo, e no celular o canal é o Telegram.
- **KPIs econômicos e de estabilidade do protocolo de testes** (custo por kg, payback, economia projetada por hectare, Índice de Estabilidade Hídrica). Saem dos dados exportados (FR-50), fora da plataforma.

## 6. Escopo e entregas

| Entrega | Conteúdo | Estado |
|---|---|---|
| **E0 — Piloto em operação** | Protótipos nRF24 (Nós 1, 2, 4, 99 e gateway), Node-RED, experimento A × B, Telegram | Em produção |
| **E1 — Placas M360** | F1–F8 nas placas LP, ACT, GW e RF; fabricação PCBA; bancada; troca dos protótipos, primeiro em nRF24 pelo header legado e depois em LoRa (Q-16) | Projeto e firmware prontos; falta fabricar e testar |
| **E2 — Backend** | F9–F14 no backend, primeiro em paralelo ao Node-RED e depois no lugar dele (fases 0–7 da spec §10) | Especificado (spec v1.3 em proposta) |
| **E3 — Multi-propriedade** | F15, canal de rádio por propriedade, Kit Hélio como segunda instalação | Especificado; factory reset pronto |
| **Futuro** | F16, OTA, assinatura de mensagens | — |

**MVP = E0 + E1 + E2.** O piloto está completo quando a estufa roda nas placas M360, comandada pelo backend, e o ciclo de alface produz o Índice Manejo360. E3 é a próxima versão.

**Por que E1 e E2 são pré-condição da medição.** O Índice só mede manejo se a atuação for confiável. O Node-RED já deu como confirmados comandos que não ligaram o relé, e perde o desligamento num reinício. Um Índice medido assim mede falhas.

**Congelamento do ciclo medido.** O ciclo que conta para o Índice começa com E1 e E2 em operação. Durante o ciclo, nenhuma troca de hardware, firmware, motor ou parâmetro de rega é feita, salvo correção de segurança registrada. Um ciclo em curso na pilha atual vale como ensaio, não como resultado [PREMISSA P-9].

E1 e E2 são independentes e podem andar em paralelo. E3 depende das duas: do firmware LoRa para o canal de rádio e do backend para as instalações.

## 7. Métricas de sucesso

### 7.1 Métricas

| ID | Métrica | Alvo | Como medir |
|---|---|---|---|
| **SM-1** | Índice Manejo360 no ciclo de alface | > 1, acima do limiar de conclusão (Q-2), com SM-C1 atendida | FR-49, ao fim de um ciclo congelado (35–45 dias, §6) |
| **SM-2** | Comandos de atuação confirmados pelo eco | 20/20 no corte da fase 3; ≥ 99 % depois [PREMISSA P-7] | Registro de comandos (FR-24) |
| **SM-3** | Regas com desligamento perdido ou duplicado | 0 | Teste de kill (RNF-01) e histórico de regas |
| **SM-4** | Avaliações do motor B registradas | 288 por dia | Registro de avaliações (FR-30) |
| **SM-5** | Disponibilidade do gateway | ≥ 99 % no mês [PREMISSA P-7] | Transições online/offline (FR-37) |
| **SM-6** | Dias seguidos com SM-2 ≥ 99 %, SM-3 = 0 e SM-4 = 288/dia depois de desligar o Node-RED | 14 | Fase 7 da migração |
| **SM-7** | Itens de bancada das placas M360 aprovados | 100 % antes de aposentar os protótipos | Pendências do `ARQUITETURA_FAMILIA.md` §9 |

### 7.2 Contra-métricas

| ID | O que vigiar | Por quê |
|---|---|---|
| **SM-C1** | Massa colhida no B em relação ao A | Um Índice > 1 obtido com produção menor é economia de água à custa de alface. O piso aceitável e o limiar de conclusão estão em aberto (Q-2) |
| **SM-C2** | Alertas por dia por instalação | Fadiga de alerta faz o técnico ignorar o que importa |
| **SM-C3** | Regas fechadas pelo reconciliador | Deveria ser zero; cada uma é um desligamento que o agendador perdeu |
| **SM-C4** | Dias com o motor do B em espera e solo seco | Bloqueio indevido por trava ou falta de clima causa estresse hídrico |
| **SM-C5** | Comandos expirados na caixa postal | Fila longa pode aplicar comando velho |

## 8. Requisitos transversais

### 8.1 Não funcionais da plataforma

O backend herda os RNF-01 a RNF-12 da spec §4. Os que definem o produto:

- **Durabilidade:** nenhuma ação futura vive só em memória (RNF-01).
- **Persistência:** reiniciar não altera contadores nem estado (RNF-02).
- **Configuração:** segredo obrigatório ausente impede o start (RNF-04).
- **Fuso:** horário persistido em UTC e exibido e agendado em Brasília (RNF-07).
- **Segurança:** painel, API e broker autenticados; segredos fora do repositório (RNF-08).
- **Convivência:** o backend roda em paralelo ao Node-RED, só consumindo, até o corte (RNF-11).
- **Idempotência e reconexão:** reexecutar uma ação não publica de novo, e uma queda do broker não perde confirmação em voo (RNF-03, RNF-10).
- **Testabilidade:** um simulador de nó reproduz eco, preempção, failsafe e sono, e a CI cobre comandos, irrigação e volume sem hardware. É o critério de saída da fase 0 (RNF-06, spec §12.1).
- **Backup:** backup diário do banco (spec §11).
- **Degradação sem internet:** sem internet, a plataforma deixa de comandar, e os nós continuam aplicando o failsafe. Ao subir, o backend recarrega o estado dos atuadores e o marca como incerto se o gateway ou o Nó 99 reiniciaram nesse intervalo (RF-CMD-12).

### 8.2 Hardware e ambiente

- **Memória:** o firmware dos nós cabe num ATmega328P (2 KB de RAM) e deixa pelo menos 512 B livres para a pilha, medidos na RAM estática do build [PREMISSA P-11]. O Nó 99 é o caso crítico: a ACT usa 825 B hoje.
- **Vazão:** nenhum código do nó bloqueia a interrupção do sensor de vazão; disso depende a exatidão do volume.
- **Fail-safe elétrico:** relés normalmente abertos, nenhum relé no boot, e nem 12 V nem 220 V passam pela placa ACT.
- **Proteção de campo:** TVS em toda linha que sai da caixa, proteção contra inversão de polaridade e contra sobretensão no 5 V.
- **Alimentação a bateria:** célula 18650 só através de módulo de carga com proteção, nunca direto no VCC das placas novas.
- **Rádio:** 915 MHz. A frequência padrão da biblioteca (868 MHz) é proibida no Brasil.
- **Instalação:** a fonte de 5 V do quadro é ajustada e travada em 5,0 V; USB e borne nunca alimentam a placa ao mesmo tempo; a gravação por FTDI é feita sem o VCC do adaptador.
- **Ambiente:** grau IP, faixa de temperatura, autonomia de bateria e alcance **não estão definidos** (Q-5, Q-6).

### 8.3 Integração

- **Contrato MQTT:** o contrato com os nós é o `inventario.md`. Mudar um child sem atualizar a plataforma faz o nó ignorar o comando em silêncio.
- **Comandos de relé:** o payload é só `"0"` ou `"1"`.
- **Serviços externos:** Open-Meteo (previsão), Telegram Bot API, modelo de linguagem para o relatório (com reserva numérica) e o broker MQTT.

### 8.4 Riscos

| Risco | Efeito | Mitigação |
|---|---|---|
| Enquadramento ANATEL do LoRa em canal fixo BW125 | Pode forçar BW500 e mudar a grade de canais | Consultar laboratório antes do pedido em volume (Q-7) |
| Placas não testadas | Defeito descoberto só em campo | SM-7; os protótipos ficam até a aprovação |
| Conferências pendentes antes do pedido PCBA (pino 1 do RJ45, furos do Pro Mini, rotações na JLCPCB, estoque, banda do RFM95W) | Lote fabricado com erro | Checklist do `ARQUITETURA_FAMILIA.md` §9 antes do pedido |
| Failsafe do Nó 99 rearmado a cada ON (`armFailsafe()`), achado em 2026-10-17 | ONs repetidos mantinham a solenóide ligada além do limite, contrariando o FR-10 | Corrigido no firmware em 2026-10-17 (história 3.5); falta gravar no Nó 99 antes da fase 3 e testar |
| Padrão de fábrica do gateway igual ao prefixo do piloto | Gateway resetado ou novo sobe publicando como a estufa piloto | Correção antes da E3 (épico 9, história 9.1) |
| Bombas NFT sem failsafe | Bomba ligada indefinidamente sem supervisão | Aceito por exigência da hidroponia; alerta de bomba ligada fora do esperado fica para a E3 (Q-3) |
| Spec do backend ainda em proposta | Sete decisões de tecnologia abertas atrasam a fase 0 | Decidir D-01..03 e D-05..08 numa sessão, a partir das recomendações do addendum §6 (Q-10) |
| Antena da ACT dentro de quadro metálico | Alcance desconhecido | Medir na instalação; usar repetidor |
| Portal do gateway sem autenticação | Qualquer um na LAN reconfigura ou apaga o gateway | FR-56 na versão seguinte |
| Assinatura MySensors sem folga de RAM comprovada | Mensagens de rádio sem autenticação | Futuro; medir em hardware |

## 9. Questões em aberto e premissas

### 9.1 Questões em aberto

| ID | Questão | Bloqueia? | Dono | Revisar quando |
|---|---|---|---|---|
| **Q-1** | O ciclo de alface já começou ou terminou? Quais as datas de transplante e colheita? Há resultados? | Não; FR-49 independe | Marcelo | Antes da fase 5 de E2 |
| **Q-2** | Qual a produção mínima do B em relação ao A (SM-C1) e qual o limiar de conclusão do Índice (por exemplo, 1,15)? | Não | Marcelo | Antes do início do ciclo congelado |
| **Q-3** | Canteiro C, peristálticas e bombas NFT ganham regra algum dia, ou ficam só manuais? | Não | Marcelo | Planejamento de E3 |
| **Q-4** | O Kit Hélio entra como segunda instalação, só leitura (D-08)? | Não | Marcelo | Planejamento de E3 |
| **Q-5** | Quais as metas de autonomia de bateria e de alcance de rádio? | Não; nenhum nó LP usa bateria hoje | Marcelo | Primeiro nó LP a bateria |
| **Q-6** | Qual o grau IP e a faixa de temperatura das caixas? | Não | Marcelo | Antes do pedido de caixas |
| **Q-7** | Quem consulta o laboratório ANATEL? A homologação é requisito para vender? | Bloqueia o pedido de RFM95W em volume | Marcelo | Antes do pedido PCBA em volume |
| **Q-8** | Como tratar pela LGPD chat_id, nomes, coordenadas e CAR? | Não para o piloto; sim para E3 | Marcelo | Planejamento de E3 |
| **Q-9** | Os tempos de failsafe (600 s e 120 s) são requisito de produto ou parâmetro por instalação? | Não | Marcelo | Planejamento de E3 |
| **Q-10** | Decisões D-01, D-02, D-03, D-05, D-06, D-07 e D-08 da spec do backend; recomendações no addendum §6 | **Bloqueia a fase 0 de E2** | Marcelo | Primeira sessão de planejamento da E2 |
| **Q-11** | Os protótipos nRF24 migram para LoRa de uma vez ou convivem por um tempo? | Não | Marcelo | Fim da bancada (SM-7) |
| **Q-12** | Persona, BMC e Pitch Deck citados nos relatórios, que estão fora do repositório, devem entrar no projeto? | Não | Marcelo | — |
| **Q-13** | Qual a área real de cada canteiro? O MVP diz 8,4 m², o motor e a spec usam 6,0 m² e 35 plantas; a área define o volume da lâmina | Não, mas afeta o FR-30 | Marcelo | Antes do ciclo congelado |
| **Q-14** | O Canteiro A é suspenso em dia de chuva, como pede o protocolo, ou rega sempre? | Não, mas afeta o Índice | Marcelo | Antes do ciclo congelado |
| **Q-15** | A vazão é calibrada antes do transplante, e o hidrômetro mede só o ramo das solenóides? | Não, mas afeta o FR-48 | Marcelo | Antes do ciclo congelado |
| **Q-16** | Como o Nó 4, fonte de clima, entra numa rede LoRa? Ele roda direto na bateria, o que o RFM95W não admite, e migrar para placa M360 é não objetivo (§5). Opções: Nó 4 numa M360-LP a bateria com DHT e DS18B20 (revoga o não objetivo); um segundo gateway nRF24 só para ele; ou outra fonte de clima (D-04) | Bloqueia só o corte para LoRa (história 3.13); o corte em nRF24 segue | Marcelo | Antes da história 3.13 |

### 9.2 Premissas

| ID | Premissa | Onde aparece |
|---|---|---|
| **P-1** | O piloto é tratado como lançamento interno: rigor alto na segurança da atuação, médio no resto | §0 |
| **P-2** | A estufa continua com dois canteiros e alface como cultura de referência | §1 |
| **P-3** | Os indicadores do experimento (F14) entram na plataforma, e não numa planilha separada | F14 |
| **P-4** | O gatilho em kPa do protocolo de testes foi substituído pelo ADC bruto, sem calibração | §5 |
| **P-5** | O protocolo de testes deve ser lido com A = timer e B = Manejo360, ao contrário do texto original | §3 |
| **P-6** | A autenticação do portal entra junto com a multi-propriedade, quando o gateway sai da rede do dono | FR-56 |
| **P-7** | Os alvos de 99 % em SM-2 e SM-5 são propostos aqui; nenhuma fonte os fixa | §7.1 |
| **P-8** | Resultado inconclusivo leva a um segundo ciclo com os tratamentos trocados de canteiro | §1 |
| **P-9** | O ciclo que conta para o Índice roda congelado sobre E1 + E2 | §6 |
| **P-10** | Papéis por instalação estendem a recomendação de D-06, que prevê só `admin` | FR-57 |
| **P-11** | Piso de 512 B de RAM livre para a pilha nos nós | §8.2 |
