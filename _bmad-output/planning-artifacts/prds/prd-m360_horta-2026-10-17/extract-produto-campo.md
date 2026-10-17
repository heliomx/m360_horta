# Extração de fontes — produto e campo (M360 Horta / Manejo360)

**Data:** 2026-10-17 · **Finalidade:** insumo para o PRD (BMad). É síntese, não cópia.
**Convenção:** cada fato traz `arquivo:seção`. `[INFERIDO]` marca o que não está escrito na fonte.

| Sigla | Arquivo |
|---|---|
| PRD-B | `_bmad-output/planning-artifacts/prd.md` (21/06/2026, "John (BMAD PM Agent)", engenharia reversa) |
| PRD-D | `docs/prd.md` (sem data; mesmo período) |
| PLAN | `docs/MVP-Planejamento da estufa.md` |
| PROT | `docs/MVP-Protocolo de Testes Canteiros A e M.md` |
| MOTOR | `docs/motor_agroclimatico_manejo360.md` |
| ENT | `docs/relatorio/entregaveis_especificacao_manejo360.md` |
| IDX | `docs/relatorio/index.md` |
| R01–R04 | `docs/relatorio/01..04_relatorio_entrega_*.md` |
| SPEC | `docs/Documento de Especificação e Detalhamento Técnico da Rede de Nós IoT da Estufa.md` (v3.5) |
| INV | `src/DRY/horta/inventario.md` (referência vigente, usada só para checar contradições) |

---

## 1. Visão do produto e objetivos

- **Marca:** "Manejo360" (M360) é o nome do produto/plataforma; "M360 Horta" é o projeto/repositório do MVP (IDX:§1; PRD-B:§1). A SPEC subtitula o projeto como **"Manejo360 — Extensor Rural Digital"** (SPEC:cabeçalho). **ViridIoTech** aparece só como pasta de origem dos documentos (`ViridIoTech/Projetos/Manejo360/Documentação`) (IDX:§3). [INFERIDO] É a empresa/marca guarda-chuva do Manejo360.
- **O que é:** plataforma IoT de monitoramento microclimático, de umidade/temperatura do solo e de controle da irrigação (IDX:§1); "ecossistema IoT de telemetria e automação agrícola" para otimizar água e monitorar o cultivo de forma hiperlocalizada (PRD-B:§1). O ENT-DOC-00 o descreve como "Sistema de Cultivo Baseado em Dados", com "Módulos de Manejo" e extração assistida por IA (ENT:§1).
- **Para quem:** produtor agrícola de pequeno e médio porte (ENT:§1, ENT-DOC-07/08, BMC e "Dor e Persona"). Esses documentos **não estão no repositório**; são só citados.
- **Por que importa / problemas:** desperdício de água por irrigação "no escuro"; necessidade de ligar e desligar bombas à mão; falta de visibilidade de clima e umidade (PRD-D:§1).
- **Objetivos de negócio declarados** (PRD-B:§1): zero desperdício hídrico (irrigar por dado real de solo e vazão, não por timer); resiliência local (a rede de rádio opera sem internet); governança dos dados via MQTT para uma plataforma central.
- **Objetivo agronômico do motor:** trocar a irrigação estática por balanço hídrico preditivo, reduzindo água e energia e prevenindo doenças fúngicas (MOTOR:§1).
- **Tese a provar no MVP:** a eficiência hídrica (kg/L) do Manejo360 é maior que a do manejo tradicional por timer, ou seja, "Índice Manejo360 > 1" (PROT:§7; PRD-B:§6.2).
- **Direção futura:** o Node-RED é solução temporária de baixo custo; a arquitetura definitiva, "com agente LLM dedicado", está em desenvolvimento (IDX:§2, nota). Estão previstos: integração MCP para consultas conversacionais ("Qual a umidade média do Canteiro A?") e ações auditadas (ENT:§2.3); backend Node.js/Express com MongoDB/PostgreSQL (ENT:§1, ENT-DOC-01/02); DSL de regras (ENT:§2.2).

## 2. Atores humanos e JTBD

| Ator | Base na fonte | JTBD |
|---|---|---|
| **Produtor / agricultor** (pequeno e médio porte) | ENT:§1 (persona do ENT-DOC-08); PROT:§4 ("costume local do agricultor"); PROT:§8 (transplante) | Irrigar a quantidade certa sem estar presente; gastar menos água e energia sem perder produção; saber se o sistema se paga (payback) (PROT:§6) |
| **Equipe técnica / instalador** | PROT:§8 ("Equipe Técnica": preparo de solo, hidráulica, calibração de vazão, pesagem); R03:§1 (instalação, cabeamento, comissionamento) | Instalar e comissionar nós e gateway; calibrar sensores; manter a rede funcionando 24/7 (R04:§3, "Sustentação Contínua") |
| **Operador da estufa / quem recebe notificações** | MOTOR:§4.2 (Telegram `@M360HortaBot`); ENT:§2.2 ("Notificar Operador via Node-RED Dashboard") | Ser avisado do início e do fim da rega, com o porquê; acompanhar o dashboard; desligar o timer do canteiro de controle quando chove (PROT:§8, obs.) |
| **Pesquisador / avaliador do experimento** [INFERIDO: papel exercido pela "Equipe Técnica"] | PROT:§5–§7 | Coletar dados diários, semanais e da colheita; calcular os KPIs; comprovar a superioridade do produto |
| **Contratante dos relatórios / tomador do serviço** | R04:cabeçalho (comprovação de serviços para NFS-e); IDX:§2 | Ver evidência de entrega por marco que justifique as Notas Fiscais. **As fontes não dizem quem é o contratante.** |
| **Desenvolvedor de firmware** | PRD-B:NFR01; PRD-D:§4 | Criar nós novos sem reescrever o ciclo de vida (lib M360-DRY) |
| **Agente de IA (ator não humano)** | ENT:§2.3; IDX:§2 | Consultar sensores e executar ações auditadas via MCP |
| Agrônomo | **Não aparece** como papel nas fontes. As referências agronômicas vêm da Embrapa e do Incaper (PLAN:§4; PROT:§3–§4) | — |

## 3. Jornadas (candidatas a User Journeys)

Nenhuma fonte nomeia pessoas; os protagonistas abaixo são papéis.

1. **Rega inteligente automática (canteiro do Manejo360).** O motor avalia a cada 5 min. Se a mediana do solo está seca, nenhuma trava (chuva, insolação, orvalho) está ativa e a lâmina projetada passa de 3 min, ele aciona a solenóide e manda pelo Telegram duração, lâmina, ET₀, VPD etc. Como se sabe que deu certo: chega a mensagem "IRRIGAÇÃO INICIADA" e depois a de fim, e o nó ecoa o estado da válvula (MOTOR:§3–§4; CLAUDE.md:Sincronizador ACK).
2. **Rega adiada por chuva, insolação ou risco fúngico.** O motor fica em "Standby"/"Adiar" com o motivo. Em estresse crítico (≥ 700 ADC) a rega acontece mesmo com chuva prevista (MOTOR:§3.3). [INFERIDO] O operador veria o motivo no dashboard ou no Telegram; a fonte só documenta a notificação de início e fim.
3. **Canteiro de controle por timer.** Rega fixa 2×/dia (07:30 e 16:30, 10–15 min) (PROT:§4); em dia de chuva o operador desliga o timer à mão (PROT:§8, obs.).
4. **Ciclo do experimento.** Semana 0: preparo, instalação e calibração → Dia 1: transplante → semanas 1–6: monitoramento diário e fitossanidade → colheita: pesagem e análise (PROT:§8).
5. **Instalação e comissionamento em campo.** A equipe instala o gateway e os 4 nós, faz o cabeamento e testa 24/7 (R03:§1, julho/2026). Como se sabe que deu certo: a telemetria chega continuamente ao Node-RED (R03:§6).
6. **Provisionamento do gateway.** Sem Wi-Fi ou MQTT válido, o gateway sobe um AP com formulário web (PRD-B:FR07).
7. **Ajuste remoto de intervalo / leitura forçada.** Comandos `SET_INTERVAL` e `FORCE_UPDATE` enviados pelo dashboard (PRD-B:FR09).
8. **Consulta conversacional (futuro).** "Qual a umidade média do Canteiro A?" via agente MCP (ENT:§2.3).
9. **Prestação de contas por marco.** Um relatório por NF, com commits, entregáveis e atividades (IDX:§2; R04:§3).

## 4. Experimento / MVP de campo

**Local e estrutura**
- Estufa em Brasília/DF, coordenadas `-15.963944, -47.804028` (MOTOR:cabeçalho). O PROT deixa o campo "Local" em branco (PROT:§2).
- Estufa de ~9 × 6 m, pé-direito de 2,5 m, mourões e sombrite 50% no teto e nas laterais (PLAN:§1–§2). Dois canteiros de tijolo de 7,0 × 1,20 m (8,4 m²), com ~25–30 cm de altura e fundo em solo (PLAN:§3). Construção física entre 01/05 e 01/06/2026 (R02:§1).
- Solo de Cerrado: análise, calagem (pH 6,0–6,8), composto, fosfato, NPK 10-10-10 (PLAN:§4).
- Gotejamento com 2 fitas por canteiro, emissores de 1–1,5 L/h a cada 0,20–0,30 m e registro por canteiro (PLAN:§5). Demanda típica da alface: 3–3,5 mm/dia (PLAN:§5).

**Desenho experimental** (PROT:§2)
- Cultura: alface Crespa e Roxa, 50/50 em cada canteiro; espaçamento de 0,25 × 0,25 m (~134 plantas por canteiro).
- Duração: 35 a 45 dias, do transplante à colheita.
- Tratamentos: **Manejo360** (sensores e atuadores, rega conforme a necessidade) × **Tradicional** (timer fixo com solenóide, 2×/dia). Os dois canteiros têm sensor de vazão digital.
- Padronização: mesmo solo, mesma adubação, mulching em ambos ou em nenhum, mesma fitossanidade (PROT:§3).
- Gatilho previsto no PROT: sensores a 10 e 25 cm; ligar quando a tensão de água no solo chegar a 20–30 kPa e desligar na capacidade de campo (PROT:§4; PRD-B:§6.1).
- **O que foi implementado de fato** (MOTOR:§3), em agosto/2026 (MOTOR:cabeçalho): pipeline de 5 camadas.
  - Soak time de 15 ou 20 min.
  - Mediana dos sensores válidos; standby abaixo de 350 ADC.
  - Travas: chuva > 70% nas próximas 4 h; radiação > 700 W/m² ou horário de 12 a 13 h; margem de orvalho noturna < 1,5 °C.
  - Lâmina pelo FAO-56 com fator atmosférico (teto de 4,5 mm); vazão de cálculo de 2,5 L/min.
  - Corte mínimo de 180 s.
  - Open-Meteo consultado a cada 30 min; avaliação a cada 5 min.

**Coleta** (PROT:§5)
- Diária (automática): litros por canteiro; umidade do canteiro inteligente; chuva e temperatura; log de cada ativação (hora, duração, volume); "gráfico de escada de umidade" A × B.
- Semanal (manual): sinais de estresse; contagem de perdas.
- Colheita: massa fresca total; peso médio de 10 amostras por variedade em cada canteiro; qualidade visual de 1 a 5.

**KPIs** (PROT:§6–§7; PRD-B:§6.2)
- Eficiência hídrica (kg/L); % de economia de água; custo de irrigação por kg; custo por kg produzido; economia projetada por hectare; payback (em ciclos).
- Métricas proprietárias: **Índice Manejo360** = EH(inteligente) / EH(tradicional), em que > 1 confirma a superioridade (a "estrela-guia", PRD-B:§6.2); **Índice de Estabilidade Hídrica** (menor oscilação de umidade); percentual de economia real.
- No PROT as fórmulas são **imagens embutidas**, ilegíveis como texto, e o item 7 da §7 está truncado ("comparado ao").

**O que conta como sucesso:** Índice Manejo360 > 1 (PROT:§7). [INFERIDO] As fontes não definem limiar mínimo de economia, significância estatística nem patamar de produção ou qualidade que o produto precise igualar.

**Estado do experimento:** nenhuma das fontes dá a data do transplante, diz se o ciclo começou ou terminou, nem traz resultados. Há uma análise de log MQTT que aponta sensores com falha (`docs/csv/analise_mqtt_log_m360_*.md`). Esse arquivo está fora da lista pedida e foi lido só para conferir a nomenclatura.

## 5. Entregáveis contratuais e marcos

| Marco | NF | Escopo declarado | Fonte |
|---|---|---|---|
| 1 | 13/02/2026 | Arquitetura Manejo360, scaffolding da lib M360-DRY, gateway inicial, documentação conceitual (Doc Técnico, Arquitetura IoT) | IDX:§2; R01:§1–§2; R04:§3 |
| 2 | 04/05/2026 | Estufa MVP (01/05–01/06), LibDRY, perfis de energia, adaptador RFM95W (KiCad), especificações de backend e hardware | IDX:§2; R02; R04:§3 |
| 3 | 07/08/2026 | Comissionamento em campo (julho/2026): gateway, 4 nós, "12 sensores", automação Node-RED; motor de inferências/MCP | IDX:§2; R03; R04:§3 |
| — | — | "Sustentação contínua 2026": manutenção de firmware, conectividade, calibração | R04:§3 |

- Documentos entregáveis ENT-DOC-00 a 10: doc técnico, API/MQTT, banco, motor/DSL, MCP, hardware, arquitetura, BMC v3.2, Dor e Persona, gateway JSON, LoRa neutra (ENT:§1). O IDX lista 14 artefatos, incluindo o Pitch Deck (Convergência-GO) e o estudo de durabilidade de sensores (IDX:§3). **Nenhum deles está no repositório**; os links apontam para `file:///d:/Meu Drive/...`.
- Enquadramento fiscal: NFS-e, código 01.07.01 (LC 116/03, suporte técnico em informática) (R04:cabeçalho).
- O R04 (23/09/2026) é um relatório consolidado, não um 4º marco com escopo próprio (R04:§3). [INFERIDO] Pode corresponder a uma 4ª NF; a fonte não diz.
- **As fontes não trazem:** valor contratado, contratante, cronograma futuro, critérios de aceite dos marcos.

## 6. Restrições de negócio, regulatórias e de operação

- **Resiliência local / conectividade rural:** automação e coleta devem funcionar na rede de rádio local, sem internet (PRD-B:§1; PRD-D:§1). [INFERIDO] O motor atual depende do Open-Meteo e do Node-RED (internet e servidor), então a rega inteligente hoje não é local (MOTOR:§2.1).
- **Energia:** o nó de clima é solar (painel, TP4056, bateria 18650) e usa smartSleep (SPEC:Nó 04; INV:§6); os atuadores ficam em fonte fixa (PRD-B:§3.2).
- **Segurança da carga:** failsafe de tempo máximo para bomba e válvulas (PRD-B:FR01); canais do MUX nunca acionados ao mesmo tempo, para proteger as fontes do pico de partida (SPEC:Nó 00).
- **Durabilidade em campo:** alimentar os sensores de solo só durante a leitura, para evitar eletrólise (PRD-B:FR05; SPEC:Nó 01); vedar as junções contra a atmosfera corrosiva; usar snubber contra EMI (SPEC:§4).
- **Agronômicas:** proteção anti-fúngica à noite; evitar regar no pico solar (MOTOR:§3.3).
- **Custo:** payback e custo por kg são KPIs (PROT:§6). A "Planilha de Custos" é citada, mas não está presente (PROT:§3). O Node-RED foi adotado por ser de baixo custo (IDX:§2).
- **Rádio / ANATEL:** há estudo de LoRa (RFM95W) e de redes LoRaWAN neutras (ENT:ENT-DOC-10). **A ANATEL não é mencionada** nas fontes lidas.
- **LGPD / privacidade:** **não é mencionada** nas fontes lidas. [INFERIDO] Passa a ser relevante num backend multiusuário (usuários e propriedades, ENT-DOC-02).
- **Fiscal:** cada entrega precisa comprovar o serviço para a NFS-e (R04).

## 7. Fora de escopo ou adiado

- O Node-RED é provisório; o backend e o agente LLM definitivos estão "em desenvolvimento e homologação" (IDX:§2).
- Mulching ou cobertura morta apenas "se definido" (PROT:§3).
- Sensores ZTS/Modbus, hidroponia NFT, canteiro C e nó de luminosidade (SPEC v3.5; PRD-B:FR03) **não existem na rede vigente** (INV:§1, Apêndice B). [INFERIDO] Foram adiados ou abandonados; nenhuma fonte de produto diz qual dos dois.
- LoRa: existe só o estudo e o adaptador (ENT-DOC-10; R02).
- MCP, DSL, API e banco existem só como especificação (ENT:§2). [INFERIDO] Nada disso foi implementado no repositório.

## 8. Contradições e itens defasados (não herdar no PRD)

1. **Papel dos canteiros invertido.** PROT:§2 e PRD-B:§6: **A = Manejo360**, B = controle por timer. Na realidade (INV:§8; MOTOR; `flows.json`, nó "Controlador Canteiro A (Timer Fixo 5 min)"): **A = timer tradicional** e **B = Manejo360** (motor agroclimático, solenóide no child 32).
2. **"Canteiro M" × "Canteiro B".** O título do PROT ("A e M"), R02:§1.1, R03:§1–§3 e a análise de log usam "M"; INV, MOTOR e flows usam "B" (commit `e950ced`, 16/07/2026, R03:§5).
3. **Rega do canteiro de controle:** PROT:§4 diz 07:30 e 16:30, 10–15 min; o flows diz "Timer Fixo 5 min"; INV:§7 diz no máximo 300 s.
4. **Gatilho em kPa** (tensão de água de 20–30 kPa; PROT:§4; PRD-B:§6.1) × sensores resistivos em **ADC bruto 0–1023, alto = seco**, com limiares de 350/650/700 ADC (MOTOR; INV:§9). Não há tensiômetro.
5. **Umidade em 0–100%** (PRD-B:FR04; PRD-D:§3.4; ENT:§2.2, "< 35%") × ADC bruto, que é o vigente.
6. **Profundidades:** PROT diz 10 e 25 cm; MOTOR e R03, 10/20/30 cm; SPEC, 10/20/30 cm a 1, 3 e 5 m; INV: **10 e 30 cm a 1, 3 e 5 m**.
7. **Sensores por nó de solo:** R03:§1 e R04:§1 falam em "3 sensores + DS18B20" e "12 sensores"; IDX:§1 em "6x"; SPEC põe 18 sensores num único nó; INV: **6 sondas por nó (nós 1 e 2), sem DS18B20**.
8. **Perfil de energia dos nós de solo:** PRD-B:§3.1 e PRD-D:§2 dizem "LOW_POWER/bateria"; R03, "Always-On / Low-Power"; INV: **ALWAYS_ON (5 V fixa)**.
9. **Fonte de clima:** MOTOR:§1–§2.2 usa o DHT11 do **Nó 99**; pelo INV:§6/§8 a fonte é o **Nó 4** desde 17/10/2026 (e o Nó 99 na placa ACT nem tem DHT).
10. **Nó 4:** SPEC diz Pro Mini 3,3 V, DHT22 + BH1750, sleep de 15 min; R04/IDX dizem DHT11 + **pH**; INV diz Pro Mini 16 MHz, **DHT11 + DS18B20**, intervalo de 30 min, sem pH.
11. **Nó 99:** IDX fala em "2 solenóides + pH"; R03/R04, em "4 solenóides"; INV: solenóides A/B/C (31–33), peristálticas (34–37), bombas NFT (38–39) e vazão (21), sem pH.
12. **Nós que não existem:** Nó 13 ZTS e Nó 80 caixas d'água (PRD-B:§3.1, FR03); `nodePump`, `nodeSelenoieVazao`, `nodeDHT11`, `nodeUmidadeTemperatura(2)`, `main_lumi_10` (PRD-D:§2); Nós 00 e 03 e o Nó 02 Modbus (SPEC). Vigentes: 1, 2, 4, 99 e o nó 11 do Kit Hélio; removidos: 5, 13, 80 (INV:Apêndice B).
13. **Tamanho do canteiro:** PLAN/PROT, 7,0 × 1,20 m (8,4 m²); MOTOR, 6 × 1 m (6 m², 35 plantas), e o cálculo de volume usa 6 m². Número de plantas: PROT ~134; PLAN 80–120; MOTOR 35.
14. **Tópicos MQTT:** ENT:§2.1 e R04 usam `m360/out/{gateway_id}/...`; o vigente é `m360/{UF}/{CAR}/out|in` (CLAUDE.md; MOTOR:§4.1 usa `m360/DF/0000/in/...`).
15. **Onde rodam as regras:** no gateway, por `rules.json` (SPEC:§1) × no Node-RED (MOTOR).
16. **Detalhes de lib defasados:** `MIN_AWAKE_TIME_MS` de 3000 ms e buffers `NODE_ITEMS_COUNT + 2` (PRD-D:§3–§4; PRD-B:NFR01), ou `+3` (R04:§2.1); a lib 2.0.0 não usa `messages[]` (CLAUDE.md).
17. **Capítulo de cada entregável:** ENT:§3 põe BMC e Dor e Persona no Cap. 2 e API/DB no Cap. 3; IDX:§3 põe BMC, Persona e Pitch no Cap. 1 e API/DB no Cap. 2.
18. **Data do Marco 2:** a NF é de 04/05/2026, mas a construção da estufa que esse marco relata foi de 01/05 a 01/06/2026 (R02:§1).
19. **Divergências entre os dois PRDs antigos:**
    - PRD-B tem FR01–FR09, NFR01–04, seção de MVP e KPIs, e os Nós 01/13/80.
    - PRD-D não tem MVP nem IDs. Lista outro conjunto de nós (DHT11, lumi, ZTS com Hall/LDR), cita relés Active LOW e `FORCE_UPDATE_N_READS` = 10, e usa o caminho `src/DRY/gateway`.
    - Os dois põem a umidade de solo em 0–100%, e os dois conjuntos de nós estão defasados.

**Total: 19 contradições.**

## 9. Perguntas em aberto para o dono do projeto

1. O ciclo de alface do experimento já começou ou terminou? Quais as datas de transplante e colheita? Há resultados?
2. Quem é o contratante dos relatórios e NFs, e quais critérios de aceite valem para o próximo marco?
3. O PRD novo cobre só o MVP da estufa, o produto Manejo360 comercial ou o backend que substitui o Node-RED?
4. A inversão de papéis (A = timer, B = inteligente) é definitiva? O protocolo será reescrito?
5. O critério de sucesso é só "Índice Manejo360 > 1", ou há meta de economia (%) e de produção mínima?
6. O gatilho em kPa sai do produto (fica o ADC), ou haverá calibração de ADC para kPa ou umidade volumétrica?
7. Quais são o horário e a duração oficiais da rega no canteiro de controle?
8. A rega inteligente precisa funcionar sem internet (objetivo de "resiliência local")? Hoje ela depende do Open-Meteo e do Node-RED.
9. Hidroponia NFT, canteiro C, peristálticas e Modbus estão no escopo ou fora?
10. A Persona, o BMC e o Pitch Deck (fora do repositório) podem ser fornecidos? Quem é a persona primária?
11. Quem opera no dia a dia (produtor, técnico ou pesquisador) e por qual canal (Telegram, dashboard, app)?
12. Há requisitos de LGPD, de multiusuário/multipropriedade e de homologação na ANATEL (LoRa 915 MHz)?
13. Qual é o modelo de receita e o preço-alvo do kit (base para o payback)?
14. Como o Kit Hélio (nó 11) entra no produto: outro cliente, outra propriedade ou outro produto?
