# Índice de `docs/`

## Planejamento BMad (fora desta pasta)

Os documentos de planejamento vivem em `_bmad-output/planning-artifacts/`. Os antigos `docs/prd.md`, `docs/architecture.md` e `docs/epics.md` foram removidos em 2026-10-17.

- **[PRD](../_bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/prd.md)** — produto inteiro: visão, FR-1 a FR-61, métricas, entregas E0–E3
- **[Addendum do PRD](../_bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/addendum.md)** — contradições dos documentos antigos, spine antigo revisto, fases
- **[Spine de arquitetura](../_bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-10-17/ARCHITECTURE-SPINE.md)** — decisões AD-1 a AD-22 e ajustes pendentes na spec
- **[Épicos e histórias](../_bmad-output/planning-artifacts/epics.md)** — 11 épicos, dos concluídos ao firmware genérico
- **[Contexto do projeto](../project-context.md)** — regras críticas para agentes de IA

## Especificações normativas

- **[especificacao-backend-m360.md](./especificacao-backend-m360.md)** — backend substituto do Node-RED, v1.3: RFs, esquema, migração, provisionamento
- **[motor_agroclimatico_manejo360.md](./motor_agroclimatico_manejo360.md)** — motor de rega do Canteiro B; clima e profundidades defasados (ver addendum §2)

## Experimento de campo (MVP)

- **[MVP-Planejamento da estufa.md](./MVP-Planejamento%20da%20estufa.md)** — dimensões, canteiros, solo e gotejamento da estufa
- **[MVP-Protocolo de Testes Canteiros A e M.md](./MVP-Protocolo%20de%20Testes%20Canteiros%20A%20e%20M.md)** — desenho A × B e KPIs; papéis dos canteiros invertidos

## Documentos históricos (defasados)

- **[Backend Definitivo M360.md](./Backend%20Definitivo%20M360.md)** — guia que precedeu a spec do backend; motor B antigo
- **[Documento de Especificação e Detalhamento Técnico da Rede de Nós IoT da Estufa.md](./Documento%20de%20Especifica%C3%A7%C3%A3o%20e%20Detalhamento%20T%C3%A9cnico%20da%20Rede%20de%20N%C3%B3s%20IoT%20da%20Estufa.md)** — rede v3.5 com nós que não existem mais
- **[Nó de Sensoriamento Tridimensional de Umidade do Solo (1).md](./N%C3%B3%20de%20Sensoriamento%20Tridimensional%20de%20Umidade%20do%20Solo%20(1).md)** — nó de solo de 18 sondas, v2.5, não construído
- **[conversa de refactoring com o Claude.txt](./conversa%20de%20refactoring%20com%20o%20Claude.txt)** — transcrição de sessão de refatoração da lib

## Referências e imagens

- **[mysensors.pdf](./mysensors.pdf)** — documentação do MySensors
- **[RFM95_96_97_98W.pdf](./RFM95_96_97_98W.pdf)** — datasheet do rádio LoRa HopeRF
- **[S2472c939b8b9415fac8c6652f218cfc6Y.pdf](./S2472c939b8b9415fac8c6652f218cfc6Y.pdf)** — PDF sem metadados; conteúdo não identificado
- **[modulo rele.jpeg](./modulo%20rele.jpeg)** — foto do módulo de relés
- **[quadro nó atuador.jpeg](./quadro%20n%C3%B3%20atuador.jpeg)** — foto do quadro do Nó 99
- **[prominiBack.jpg](./prominiBack.jpg)**, **[prominiFront.jpg](./prominiFront.jpg)**, **[prominiZoom.jpeg](./prominiZoom.jpeg)** — fotos do Pro Mini para medir furos
- **[tp4056.jpeg](./tp4056.jpeg)** — foto do módulo de carga TP4056

## Subpastas

### datasheets/

- **[README.md](./datasheets/README.md)** — índice dos datasheets e do que foi conferido em cada um
- PDFs da JLCPCB que fundamentam decisões das placas M360 (reed, RJ45, TVS, bornes, headers)

### relatorio/

- **[index.md](./relatorio/index.md)** — índice dos relatórios de entrega
- **[01_relatorio_entrega_13-02-2026.md](./relatorio/01_relatorio_entrega_13-02-2026.md)** — marco 1: arquitetura e scaffolding da lib
- **[02_relatorio_entrega_04-05-2026.md](./relatorio/02_relatorio_entrega_04-05-2026.md)** — marco 2: estufa, LibDRY, perfis de energia
- **[03_relatorio_entrega_07-08-2026.md](./relatorio/03_relatorio_entrega_07-08-2026.md)** — marco 3: comissionamento em campo
- **[04_relatorio_entrega_23-09-2026.md](./relatorio/04_relatorio_entrega_23-09-2026.md)** — relatório consolidado de atividades
- **[dados_commits_brutos.md](./relatorio/dados_commits_brutos.md)** — commits minerados por janela de entrega
- **[entregaveis_especificacao_manejo360.md](./relatorio/entregaveis_especificacao_manejo360.md)** — lista dos entregáveis documentais ENT-DOC
- **[matriz_atividades.md](./relatorio/matriz_atividades.md)** — rastreio de commits contra atividades
- **[relatorio_atividades_desenvolvidas.md](./relatorio/relatorio_atividades_desenvolvidas.md)** — relatório de atividades para NFS-e

### csv/

- **[analise_mqtt_log_m360_1786542346141.md](./csv/analise_mqtt_log_m360_1786542346141.md)** — análise de log MQTT e das regras de rega
- **[mqtt_log_m360_1786717567940.csv](./csv/mqtt_log_m360_1786717567940.csv)** — captura de log MQTT exportada do Node-RED
