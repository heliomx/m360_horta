---
title: "Reconciliação do PRD com as fontes"
prd: prd.md + addendum.md (draft de 2026-10-17)
data: 2026-10-17
escopo: só leitura; o PRD não foi editado
---

# Reconciliação do PRD com as fontes

Classificação de cada achado: **(a)** omissão silenciosa de capacidade, restrição ou intenção; **(b)** erro factual ou referência errada; **(c)** estado de implementação exagerado. Severidade: **alta** (muda decisão, contrato ou segurança), **média**, **baixa**.

Verificação das referências: todos os IDs citados em "Ref." existem na spec v1.3. A contagem de 83 RFs confere: ING 10, REG 13, CMD 13, IRR 10, VOL 3, WDG 3, NOT 8, UI 8, API 3 e PRV 12. Os problemas de referência estão listados na §1.1.

---

## 1. `docs/especificacao-backend-m360.md` (v1.3)

### 1.1 Referências com sentido errado

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| S1 | (b) alta | FR-53 | §5.2 (`installation.state PENDENTE\|COMISSIONANDO\|ATIVA\|SUSPENSA`); RF-PRV-09 | O PRD lista "resetada" entre os estados da **instalação**. Na spec, `RESETADO` é marca do **gateway**, aplicada ao receber `factory_reset`. A instalação tem só quatro estados. | Tirar "resetada" de FR-53 e escrever: "o gateway que envia `factory_reset` é marcado como resetado (RF-PRV-09)". Se a intenção for criar um quinto estado, marcar como [PREMISSA] e levar à spec. |
| S2 | (b) média | FR-57, "Ref.: D-06" | §13 D-06 | D-06 recomenda **um papel `admin`** e deixa OIDC como alternativa. Papéis por instalação não estão em D-06 nem em lugar nenhum da spec. Em RF-PRV-10, "operadores" são assinantes do Telegram, não contas com papel. | Marcar FR-57 como [PREMISSA] nova, sem Ref., ou citar D-06 como "a ser revista". |
| S3 | (b) média | §8.1, "Degradação sem internet… (RF-CMD-12)" | RF-CMD-12 | O RF-CMD-12 trata do **startup do backend**. O estado fica `incerto` **só** se o gateway ou o Nó 99 rebootaram depois do último estado persistido. Não fala em "quando a conexão volta" nem em marcar sempre como incerto. | Reescrever: "na subida do backend, o estado persistido é recarregado e fica incerto se gateway ou Nó 99 rebootaram (RF-CMD-12)". A queda de internet sem reboot é coberta por RNF-10 (sessão persistente). |
| S4 | (b) baixa | FR-45, "Ref.: RF-UI-07, RF-PRV-11" | RF-PRV-11 | O RF-PRV-11 audita mudanças de **estado da instalação, de atuadores e de canteiros**. A trilha dos parâmetros do motor está em RF-UI-07. | Manter só RF-UI-07, ou explicar que RF-PRV-11 cobre o cadastro. |
| S5 | (b) baixa | FR-52, "não comanda nem alerta" | RF-PRV-03 | Na descoberta, a spec **envia um alerta ao chat administrativo**. Só as notificações da própria instalação ficam desligadas. | "…não comanda nem dispara alertas próprios; a descoberta gera um único alerta ao administrador". |
| S6 | (b) baixa | FR-16, consequência "nunca recebem o mesmo canal" | RF-PRV-12 | O administrador pode forçar o cadastro, com justificativa registrada. | Acrescentar "salvo exceção forçada e justificada pelo administrador". |

### 1.2 Capacidades e restrições omitidas

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| S7 | (a) alta | §8.1 (lista os "RNF que definem o produto") | §1 objetivo 7; RNF-06; §10 fase 0; §12.1 | **Testabilidade sem a estufa** é um dos 7 objetivos da spec e o critério de saída da fase 0: simulador de nó e CI sem hardware. O PRD não cita o simulador nem a testabilidade em parte alguma. | Incluir em §8.1: "Testabilidade (RNF-06): simulador de gateway e nós, com eco, preempção, failsafe e sono, e CI sem hardware". Citar como pré-requisito de E2. |
| S8 | (a) alta | F4 / F9 | RF-CMD-08; regra de ouro da §10 | O PRD só descreve a exclusão mútua do MUX **no firmware** (FR-9). Não cita o grupo de concorrência do lado da plataforma: nunca dois comandos em voo em 31–37, fila por nó e estado OFF dos irmãos pelo eco de preempção. | Novo FR em F9: "comandos aos canais do MUX são serializados por nó; a preempção atualiza o estado dos demais canais. Ref.: RF-CMD-08". |
| S9 | (a) média | F10 | RF-CMD-11; RF-IRR-06; cenários 4 e 5 | Faltam dois desfechos de rega. (1) Um `V_STATUS=0` não solicitado, vindo do failsafe ou da preempção, encerra a run com `failsafe_or_preempt`. (2) Se o comando de abrir falha, a run termina em `command_failed` e o OFF agendado é cancelado. É esta segunda regra que corrige "o Node-RED segue contando o tempo de uma válvula que nunca abriu". | Acrescentar a F10: "Rega cuja abertura falha é encerrada e avisada; desligamento não pedido pelo nó encerra a rega em curso. Ref.: RF-IRR-06, RF-CMD-11". |
| S10 | (a) média | §8.1 | §11 (backup diário `pg_dump` fora do host, migrações versionadas, TLS no proxy) | O PRD não pede **backup**. Os dados do experimento (litros, colheita, avaliações) são o produto do piloto (F14), e perdê-los invalida o SM-1. | Novo item em §8.1: "Backup diário do banco fora do host (spec §11)". |
| S11 | (a) média | §8.1 | RNF-03 (idempotência), RNF-10 (sessão MQTT persistente; queda do broker por 30 s não perde confirmação) | Os dois RNFs sustentam o SM-3 (zero desligamento duplicado) e o FR-25, mas ficaram fora da lista. | Citar RNF-03 e RNF-10 em §8.1. |
| S12 | (a) baixa | F9 / F13 | RF-CMD-13; RF-API-03 | A chave operacional que desliga a confirmação por eco (equivalente ao `ENABLE/DISABLE_ACK_SYNC`) é um modo de diagnóstico **visível na UI**. Ela muda o significado de "confirmado". | Mencionar em F9 como modo de diagnóstico, sempre sinalizado. |
| S13 | (a) baixa | FR-24 | §5.3 | Os estados do comando estão incompletos: faltam `AGUARDANDO_DESPERTAR`, `CANCELADO` e `DIVERGENTE`. O FR-27 já usa a caixa postal e o FR-28 fala em confirmação do intervalo. | Completar a lista ou escrever "conforme a máquina de estados da spec §5.3". |
| S14 | (a) baixa | F10, F12 / §10 | §10, migração de dados (contador `flow_vol_node_99_21`, assinantes, D-1); rollback da fase 7 | A continuidade dos dados ao trocar o Node-RED pelo backend não aparece: volume acumulado, assinantes do Telegram, snapshot D-1 e rollback documentado. | Acrescentar uma linha em §6 (E2): "migração única dos contadores, assinantes e D-1; rollback documentado". |
| S15 | (b) baixa | FR-38 / FR-30 / FR-50 | RNF-12 (`rule_evaluation` 90 dias, `message_log` 30 dias) | O FR-30 promete que "toda decisão pode ser explicada depois" e o FR-50 exporta as avaliações. Mas a retenção das avaliações é de 90 dias. Para comparar ciclos, elas se perdem. | Ou o FR-50 exige exportar ao fechar o ciclo, ou F14 pede retenção sem limite das avaliações dentro de um ciclo registrado. |

### 1.3 Exageros de estado

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| S16 | (c) média | F8, estado "implementado, sem bancada" | Spec §15.5, cenário 22: "firmware verificado em bancada"; ARQ §9: "Falta o teste de bancada" | As fontes se contradizem sobre o factory reset do gateway, e o PRD seguiu o ARQ sem registrar a divergência. O factory reset também roda nos gateways **protótipo** (`d1_mini_gateway` e Kit Hélio), não só na M360-GW. | Separar o estado por placa (protótipo × M360-GW). Abrir questão para conciliar a spec com o ARQ §9. |
| S17 | (c) baixa | FR-21, consequência "o despertar abre a janela da caixa postal" | Spec cenário 15b ("Reed da LP: Dado o Nó 4…") | Hoje nenhum nó **dorme e tem reed** ao mesmo tempo. Os nós LP 1 e 2 são `ALWAYS_ON`, e o Nó 4, único `LOW_POWER`, fica no Pro Mini sem reed (ARQ §1; PRD §5). A própria spec usa o Nó 4 no cenário da LP. | Escrever a consequência no condicional ("um nó LP a bateria…"). Registrar a incoerência do cenário 15b como pendência da spec. |

---

## 2. `hardware/ARQUITETURA_FAMILIA.md`

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| H1 | (b) alta | UJ-4 ("Manutenção sem abrir a caixa… Num atuador, manter o ímã 3 s"), título de F8, FR-22 | §5.2, linha do reed da ACT: "o usuário **abre a porta do quadro** e passa o ímã junto do SW1" (decisão de 2026-09-14) | O reed da ACT fica na placa, dentro do quadro. Repareá-la exige abrir a porta. Só na LP e na GW o ímã age através da parede. | Restringir "sem abrir a caixa" à LP e à GW. Em FR-22: "com a porta do quadro aberta, sem desligar nem reiniciar". |
| H2 | (b) alta | FR-10 ("desliga **cada** carga ao atingir o tempo máximo") | Inventário §7.3; spec §2.4/§9.1; ARQ §5 | As bombas 38 e 39 **não têm** limite, por projeto. Elas também ficam fora do backstop do reconciliador (RF-IRR-07). Como está escrito, o FR-10 promete proteção que não existe para duas das nove cargas. | "…cada solenóide (600 s) e cada peristáltica (120 s); as bombas NFT ficam em regime contínuo, sem failsafe". Acrescentar o risco em §8.4 (bomba ligada sem supervisão). |
| H3 | (c) média | §6, E1: "Projeto e firmware prontos; falta fabricar e testar" | §9 Pendências | Ainda há conferências **antes do pedido**: rotação de D1, D2, Q1, SW1 e U1–U8 no visualizador da JLCPCB; estoque do C25804; furos livres do Pro Mini medidos por foto, a confirmar com paquímetro; pino 1 do RJ45, que se espelhado põe VCC e GND nas linhas de sinal; banda do RFM95W, a validar antes de comprar em volume. | "Projeto pronto, com conferências pré-pedido pendentes (ARQ §9)". Levar os itens críticos (pino 1 do RJ45, banda) para §8.4 Riscos. |
| H4 | (a) média | §8.2 / UJ-5 | §5.1 (5 V do quadro ajustado em 5,0 V, no máximo 5,1 V); §6.1 (GW: "não alimentar por USB e borne ao mesmo tempo"; fonte travada em 5,0 V); §4.2/§8 (gravar com FTDI **sem VCC**) | Regras de instalação e manutenção que protegem as placas não aparecem. O instalador é ator do PRD (§2.1) e o comissionamento depende delas. | Acrescentar "Regras de instalação" em §8.2 ou no checklist de E1: 5 V em 5,0 V; USB e borne nunca juntos; FTDI sem VCC; placa fora da caixa para gravar. |
| H5 | (b) média | FR-13 ("Nós a bateria… param de transmitir abaixo de um limiar seguro") | §9, firmware: corte `LP_VBAT_CUTOFF` só na LP; fuses/BOD 2,7 V a confirmar antes de fixar o limiar | O corte existe só no firmware da LP, com padrão de 3,0 V, e o limiar depende dos fuses ainda não lidos. O Nó 4, único nó a bateria em produção, não tem esse corte. Consulta: `grep "VBAT\|cutoff\|CUTOFF"` em `src/DRY/horta/nos/04noodeSolarMini/*.cpp,*.h` voltou vazio; busca vazia não é prova. | "Nós **M360-LP** a bateria…". Registrar que o limiar depende da leitura dos fuses (ARQ §9). |
| H6 | (a) baixa | §8.4 Riscos | §9: trilho de 3,3 V da GW perto do limite (ESP com WiFi e LoRa transmitindo); LDO do Pro Mini alimentado de volta; distância do ímã ao reed (~6 mm, "justo") | São três riscos de bancada que podem invalidar FR-21 e FR-23 em campo. | Citar em §8.4 ou apontar para ARQ §9 com SM-7. |
| H7 | (a) baixa | §5 (não objetivos) e Q-4 | §1: a M360-GW atende o nó 0 "Horta **e Kit Hélio**" | O PRD fala em não migrar o **nó** do Kit Hélio, mas não diz se o **gateway** do Kit Hélio vai para a M360-GW. | Esclarecer em §5 ou Q-4. |

---

## 3. `src/DRY/horta/inventario.md`

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| I1 | (b) alta | F2, título "Clima da estufa"; FR-4 | §6 ("o nó fica **fora da estufa**"); CLAUDE.md; spec D-04 ("o clima é o de fora da estufa") | O Nó 4 mede o clima **externo**, e só a cada 30 min (`LOW_POWER`). O PRD apresenta esse dado como clima da estufa. Isso pesa no motor do B, que usa temperatura e umidade nas travas e no fator de estufa. | Renomear F2 para "Clima local (externo)". Dizer em FR-4 que o dado é externo e sai a cada 30 min. Citar a alternativa de D-04 (DHT dentro da estufa). |
| I2 | (b) média | FR-1 ("publica a leitura bruta de cada sonda **no intervalo configurado**") | §2, `reportIntervalMin` "não implementado": o envio sai quando o valor varia mais de 0,05 ou a cada 10 ciclos sem mudança | A cadência real é "por mudança, com reenvio forçado a cada 10 ciclos". Isso explica o timeout de 10× do intervalo (RF-REG-09). | Corrigir FR-1: "a cada ciclo, quando o valor muda, e no mínimo a cada 10 ciclos". |
| I3 | (a) baixa | F2 / F13 | §1 e §8: children 11/12 do Nó 99 só no Nano, alimentando o gráfico "Clima (Nó 99)"; spec RF-UI-03 | O PRD não diz que, ao trocar o Nano pela ACT, a série "Clima (Nó 99)" desaparece. | Uma linha em F2 ou E1: "a troca do Nó 99 pela ACT remove a série de clima do Nó 99". |
| I4 | (a) baixa | §8.3 | §8 nota sobre REPRESENT após renumerar; spec RF-REG-02/04 | A regra operacional "depois de renumerar children, reapresentar" fica só implícita no FR-36. | Opcional: citar em §8.3. |

As demais afirmações do PRD conferem com o inventário: seis sondas a 1, 3 e 5 m e 10/30 cm; nove cargas; children 31 e 32; failsafe de 600 e 120 s; vazão em L/min; IDs reservados 253, 254 e 255; ADC alto = seco.

---

## 4. `docs/motor_agroclimatico_manejo360.md`

Documento parcialmente defasado: cita o clima do Nó 99 e profundidades de 10/20/30 cm. A spec §7.3 prevalece.

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| M1 | (b) média | FR-30 ("lâmina pelo FAO-56"); §2.2 | §3.4; spec §7.3 passo 4 | A lâmina é uma **heurística sobre o ADC**: `(mediana−350)/500 × 3,5 mm × F_atmo`, com teto de 4,5 mm. O F_atmo usa VPD, vento e temperatura/umidade. **ET₀ e Kc não entram na conta**, apesar de o doc citar `ETc = ET0 × Kc`. "FAO-56" só nomeia o modo. | "Lâmina por déficit de solo com fator atmosférico (nome do modo: FAO-56)". Se a intenção é usar ET₀ de fato, registrar como evolução. |
| M2 | (a) média | FR-30 / UJ-1 | §3.3: trava de solo úmido (< 350) e **sobreposições**: chuva com mediana ≥ 700 dispara rega de emergência; radiação e orvalho não bloqueiam com mediana ≥ 650 | O PRD lista as travas sem as sobreposições por estresse crítico. É uma intenção agronômica central: "as plantas não podem mais esperar". Sem elas, um leitor entende que a chuva prevista sempre bloqueia. | Acrescentar em FR-30: "solo crítico sobrepõe as travas de chuva, insolação e orvalho (rega de emergência)". |
| M3 | (a) baixa | §1 / F10 | §1: objetivos de reduzir água **e energia**, prevenir doenças fúngicas e evitar choque térmico | O PRD guarda só a tese da água. A prevenção fúngica aparece só como trava. | Opcional: citar os objetivos agronômicos em §1 ou F10. |
| M4 | (b) baixa | Addendum §2, linha "Profundidades" | §1/§2.2 do motor ("10, 20 e 30 cm") | O addendum lista as divergências dos PRDs antigos e dos MVPs, mas não a do doc do motor, que é normativo para o PRD (§0) e segue com Nó 99 e 10/20/30 cm. | Acrescentar o motor aos "documentos antigos" no addendum §2 ou propor sua atualização. |

---

## 5. `docs/MVP-Planejamento da estufa.md` e `docs/MVP-Protocolo de Testes Canteiros A e M.md`

A inversão A/B não é tratada como lacuna, como pedido.

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| P1 | (a) alta | F14 (FR-49) / SM-1 / §9 | Protocolo §2 e Planejamento §3: canteiros de **7,0 × 1,2 m = 8,4 m²**; motor e spec §9.3: `area_m2 = 6,0` ("6 m × 1 m, 35 plantas") | A área do canteiro decide o volume da lâmina e a comparação por área. As fontes divergem (8,4 × 6,0 m²; 134 × 35 plantas), e o PRD não registra isso. | Nova Q em §9.1: "área irrigada e população reais do Canteiro B (8,4 ou 6,0 m²)", bloqueando a fase 5 de E2. |
| P2 | (a) alta | F10 / FR-29 / FR-34 | Protocolo, Observações: "em caso de chuva… o temporizador [do controle] deve ser desligado manualmente para não enviesar o teste" | A regra experimental de suspender o controle em dia de chuva sumiu. A estufa é coberta de sombrite, então a chuva entra. O FR-29 diz só "sem nenhuma inteligência". | Acrescentar em F10: "o técnico pode suspender a regra do A em dia de chuva, com registro do motivo (FR-34)". O Índice depende disso. |
| P3 | (a) média | F14 | Protocolo §5 (semanal: saúde das plantas, contagem de perdas; colheita: peso médio por cabeça em 10 amostras por variedade, qualidade visual 1–5) e §2 (alface Crespa e Roxa, 50/50) | O FR-49 registra só datas e massa total por canteiro. Variedade, perdas e qualidade não aparecem, e sem elas não há contra-métrica de qualidade (SM-C1 mede só massa). | Ampliar FR-49: registro por variedade, perdas semanais e nota de qualidade. Ou declarar em §5 que ficam em planilha. |
| P4 | (a) média | F14 / §7 | Protocolo §6–§7: % de economia de água, custo de irrigação por kg, custo por kg, economia por hectare, payback, **Índice de Estabilidade Hídrica**, % de economia real | Só a eficiência hídrica e o Índice Manejo360 entraram, e o descarte dos demais não está registrado. O Índice de Estabilidade sai dos dados que a plataforma já tem. | Incluir % de economia e Estabilidade Hídrica em F14, ou listar os demais KPIs como não objetivo ou futuro. |
| P5 | (a) baixa | F14 / FR-50 | Protocolo §3 (padronização de insumos: a irrigação é a única variável) e §5 (chuva diária em mm e temperatura) | A exportação do experimento não inclui o contexto climático diário, como a chuva da Open-Meteo, nem o registro das práticas iguais nos dois canteiros. | Acrescentar o contexto climático diário à exportação. A padronização pode ficar como premissa P-8. |
| P6 | (a) baixa | §2.1 / UJ-6 | Protocolo §8: semana 0 com calibração dos sensores de vazão | A calibração do YF-S201 (450 pulsos/L, inventário §9) não aparece. O volume, e portanto o Índice, depende dela. | Item do checklist (C8) ou de UJ-6: "vazão calibrada antes do transplante". |

---

## 6. PRDs antigos (`_bmad-output/planning-artifacts/prd.md`, `docs/prd.md`)

Ideias ainda válidas que sumiram sem registro. A operação local sem internet está tratada como não objetivo em §5, o que é um descarte explícito e correto.

| # | Tipo / sev. | PRD | Fonte | Achado | Correção sugerida |
|---|---|---|---|---|---|
| O1 | (a) média | F4 / §8.4 | PRD-B §3.2 e FR01; PRD-A §2 (bomba com timeout "para jamais queimar trabalhando a seco") | A proteção da bomba contra funcionamento indefinido saiu, e com razão no caso das NFT. Mas o PRD não registra o risco que fica: bombas 38/39 sem failsafe, sem backstop e sem alerta de tempo ligado. Ver H2. | Risco em §8.4 com mitigação (alerta de bomba ligada há mais de X h, ou não objetivo explícito). |
| O2 | (a) baixa | F3 / §8.2 | PRD-B NFR02 (contagem da vazão por interrupção; não bloquear com `delay()`) | A restrição de firmware que protege a exatidão do volume, e portanto o Índice, não aparece. Ainda vale para o YF-S201 no D3/INT1. | Uma linha em §8.2 ou no addendum §1: "o firmware não pode bloquear a ISR da vazão". |
| O3 | (a) baixa | Corpo de F1–F16 | PRD-B: criticidade P0/P1/P2 por requisito | O PRD novo marca estado e entrega, mas não prioridade dentro de cada entrega. | Opcional: marcar os FRs críticos de segurança (FR-10, 11, 25, 31, 32) como P0. |
| O4 | (a) baixa | F13 | PRD-A, "Prevenção de dados estagnados" (`FORCE_UPDATE_N_READS`) | É o mesmo mecanismo de I2 e deve aparecer no FR-1, não ser omitido. | Coberto pela correção de I2. |

---

## 7. Resumo por prioridade

1. **Corrigir já (erro factual):** H1 (ímã da ACT exige abrir o quadro), H2 (bombas sem failsafe), I1 (o clima do Nó 4 é externo), S1 ("resetada" não é estado da instalação), S2 (FR-57 sem base em D-06).
2. **Omissões de peso:** S7 (simulador e testabilidade), S8 (serialização do MUX na plataforma), P1 (área do canteiro diverge entre as fontes), P2 (suspensão do controle em dia de chuva), S10 (backup).
3. **Estado exagerado:** H3 (pendências pré-pedido de E1), S16 (bancada do factory reset contraditória), H5 (corte de bateria só na LP), S17 (caixa postal pelo reed sem nó real que a use).
