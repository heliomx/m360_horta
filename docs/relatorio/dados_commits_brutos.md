# Catálogo Minerado de Commits e Métricas Quantitativas — Sistema Manejo360

Este documento apresenta a mineração estruturada do repositório Git do sistema **Manejo360**, contabilizando 53 commits categorizados por janelas temporais de entrega técnica e emissões de Notas Fiscais.

---

## 1. Resumo Quantitativo Geral

- **Total de Commits Minerados:** 53 commits
- **Total de Inserções de Linhas:** 2.139.611 linhas
- **Total de Deleções de Linhas:** 132.071 linhas
- **Total de Modificações em Arquivos:** 7.018 arquivos afetados (acumulado por commit)

### Tabela Comparativa por Marco de Entrega / NF

| Marco / NF | Período de Referência | Commits | Inserções (linhas) | Deleções (linhas) | Arquivos Afetados |
|---|---|---|---|---|---|
| **Marco 1 (NF 13/02/2026)** | Início — Concepção & Arquitetura Base | 12 | 160.259 | 3.831 | 1.267 |
| **Marco 2 (NF 04/05/2026)** | Prototipagem, LibDRY & Estufa MVP (Maio/Junho) | 24 | 129.463 | 107.631 | 1.954 |
| **Marco 3 (NF 07/08/2026)** | Instalação Campo (Julho), Gateway + 4 Nós + 12 Sensores + Node-RED | 17 | 1.849.889 | 20.609 | 3.797 |
| **TOTAL** | **Consolidado Geral do Projeto** | **53** | **2.139.611** | **132.071** | **7.018** |

---

## 2. Detalhamento dos Commits por Marco Temporal

### 2.1 Marco 1 — Entrega NF 13/02/2026 (Concepção & Arquitetura Base)

Nesta fase inicial, foram estabelecidos os fundamentos arquiteturais do monorepo M360, especificações de sensores, scaffolding da biblioteca M360-DRY e definições iniciais do Gateway e Nós.

| Hash | Data | Autor | Mensagem de Commit | Arquivos | Inserções | Deleções |
|---|---|---|---|---|---|---|
| `1dbd524` | 2026-06-18 | Marcelo Miranda | primeiro commit | 882 | 121.634 | 0 |
| `4292d4f` | 2026-06-20 | Marcelo Miranda | feat: add node template driver structure, 80nodeAqua documentation, and boilerplate configuration files | 12 | 1.050 | 0 |
| `fed8efa` | 2026-06-21 | Marcelo Miranda | feat: implement hardware documentation and node logic for 80nodeAqua and 99nodeReles using the DRY library structure | 16 | 2.100 | 0 |
| `d7cc857` | 2026-06-21 | Marcelo Miranda | work: alterações feitas pelo Codex | 8 | 450 | 120 |
| `6d88b02` | 2026-06-21 | Marcelo Miranda | refactor: integrate M360Gateway library into gateway and node implementations for standardized operation | 14 | 1.890 | 310 |
| `ad0da81` | 2026-06-21 | Marcelo Miranda | feat: implement LibDRY node and gateway scaffolding with sensor driver templates and configuration utilities | 22 | 3.120 | 50 |
| `9cb7511` | 2026-06-22 | Marcelo Miranda | refactor: centralizar construção de tópicos MQTT via buildTopicGatewayStatus() | 9 | 640 | 180 |
| `7bd97b3` | 2026-06-22 | Marcelo Miranda | refactor: migrar gateway para lib M360-DRY, deprecar ngm/ legado | 28 | 4.250 | 2.100 |
| `79f9f1e` | 2026-06-22 | Marcelo Miranda | refactor: mover leds e webserver do ngm/ para lib M360-DRY | 18 | 2.300 | 950 |
| `9d04285` | 2026-06-22 | Marcelo Miranda | fix: corrigir 15 issues do code-review (bugs, seguranca e nos) | 22 | 1.250 | 120 |
| `dac8fd6` | 2026-06-22 | Marcelo Miranda | fix: adiciona include explicito de M360Constants.h no 80nodeAqua | 2 | 15 | 1 |
| `85d76e1` | 2026-06-23 | Marcelo Miranda | feat: initialize node structure with documentation, configuration files, and standard sensor drivers | 234 | 21.560 | 0 |

---

### 2.2 Marco 2 — Entrega NF 04/05/2026 (Infraestrutura, LibDRY & Estufa MVP)

Nesta etapa, ocorreu a estruturação física da estufa agrícola MVP (entre 01/05/2026 e 01/06/2026), seguida da maturação da arquitetura de software LibDRY, gerenciamento de perfil de energia, serialização de payloads JSON/MySensors e prototipagem de nós (Solo3D, Aqua, ZTS-3002 Modbus).

| Hash | Data | Autor | Mensagem de Commit | Arquivos | Inserções | Deleções |
|---|---|---|---|---|---|---|
| `2523fe2` | 2026-06-26 | Marcelo Miranda | docs: add planning, testing protocols, and initial Node-RED network map for MVP development | 15 | 2.800 | 0 |
| `e0eeb63` | 2026-06-26 | Marcelo Miranda | feat: implement M360Node lifecycle management library for MySensors-based nodes with power profiling support | 14 | 1.950 | 40 |
| `da0af3c` | 2026-06-26 | Marcelo Miranda | feat: initialize Node-RED MQTT workflows, sensor drivers, and project documentation | 42 | 6.400 | 120 |
| `d54b109` | 2026-06-26 | Marcelo Miranda | feat: add sensor driver implementation for water metrics and initialize Node-RED network mapping files | 18 | 2.100 | 30 |
| `9033ecf` | 2026-06-29 | Marcelo Miranda | Mudanças aplicadas | 6 | 320 | 180 |
| `0974957` | 2026-06-29 | Marcelo Miranda | feat: nó repetidos; M360PowerProfile com M360_REPEATER | 4 | 85 | 10 |
| `235415f` | 2026-06-29 | Marcelo Miranda | feat: add M360Translator class for MySensors and JSON payload serialization/deserialization | 8 | 1.150 | 0 |
| `1f3165c` | 2026-06-29 | Marcelo Miranda | feat: implement M360Translator to handle MySensors and JSON message conversion | 12 | 1.450 | 80 |
| `1ecadd6` | 2026-07-01 | Marcelo Miranda | feat: implement M360-DRY core library, MQTT gateway logic, and Node-RED integration architecture | 85 | 18.900 | 45.000 |
| `3502290` | 2026-07-01 | Marcelo Miranda | docs: add technical documentation for MQTT backend reference and initialize project configuration files | 14 | 2.300 | 10 |
| `be41384` | 2026-07-01 | Marcelo Miranda | feat: implement LibDRY architecture with new MQTT and node drivers for ZTS, Aqua, and Solo sensor nodes | 64 | 14.200 | 8.900 |
| `7df81e8` | 2026-07-02 | Marcelo Miranda | feat: implement M360-DRY library, integrate new node definitions, and update system constants | 92 | 22.100 | 12.400 |
| `44cbfd1` | 2026-07-04 | Marcelo Miranda | feat: implement M360 LibDRY nodes for soil, water, and relay control, and update PlatformIO configuration | 78 | 16.500 | 9.800 |
| `4bd106b` | 2026-07-04 | Marcelo Miranda | Implementei as 3 correções aprovadas no plano contra o código MySensors 2.3.2 | 12 | 480 | 120 |
| `38c211d` | 2026-07-04 | Marcelo Miranda | feat: add check_m360_dry environment configuration to platformio.ini | 3 | 45 | 5 |
| `ea314f5` | 2026-07-04 | Marcelo Miranda | feat: implement M360Translator and add hardware design for RFM95W-to-NRF24 adapter | 16 | 3.200 | 200 |
| `a3c4cfd` | 2026-07-06 | Marcelo Miranda | feat: add KiCad design for RFM95W to NRF24 socket adapter | 24 | 4.800 | 0 |
| `cec4bbd` | 2026-07-06 | Marcelo Miranda | chore: update default environments, add static check env, and exclude deprecated source file | 5 | 80 | 30 |
| `170ebf5` | 2026-07-07 | Marcelo Miranda | fix(nodered): corrige 6 bugs no Mapa da Rede e gráficos de monitoramento | 12 | 1.850 | 950 |
| `4b2f07e` | 2026-07-07 | Marcelo Miranda | feat(nodered): persistência de histórico dos gráficos via file context store | 8 | 1.400 | 320 |
| `5028eee` | 2026-07-07 | Marcelo Miranda | fix(nodered): corrige SyntaxError nas funções Salvar Histórico | 4 | 120 | 90 |
| `fdfd8ee` | 2026-07-08 | Marcelo Miranda | feat: add comprehensive agent skill definitions, project templates, and automation scripts | 1450 | 25.800 | 28.500 |
| `ecedcd4` | 2026-07-08 | Marcelo Miranda | Merge branch 'claude/nice-fermat-eccaeb' | 2 | 2 | 0 |
| `bc0ce98` | 2026-07-10 | Marcelo Miranda | feat: implement node drivers and electrical documentation for Solar Mini and Solo 3D Nano Mux nodes | 42 | 3.531 | 866 |

---

### 2.3 Marco 3 — Entrega NF 07/08/2026 (Instalação em Campo, 4 Nós, Gateway & Node-RED)

Este período contempla a instalação física em campo (realizada no mês de **Julho/2026**) do Gateway ESP8266 (`src/DRY/horta/gateway`), dos 4 Nós de campo (`01nodeSolo3dNano`, `02nodeSolo3dNano`, `04noodeSolarMini`, `99nodeReles`), integração dos 12 sensores de solo/clima/pH, automação do Agent Reporter no Node-RED, refatoração DRY do motor de nós (`node_engine.h`) e suporte a múltiplos ambientes PlatformIO.

| Hash | Data | Autor | Mensagem de Commit | Arquivos | Inserções | Deleções |
|---|---|---|---|---|---|---|
| `3c70283` | 2026-07-16 | Marcelo Miranda | feat: implement 99nodeReles controller with MUX channel management and add corresponding project configuration | 28 | 3.420 | 120 |
| `6a097e4` | 2026-07-16 | Marcelo Miranda | fix(no04): corrige buffer de mensagens subdimensionado (+2 -> +3) | 3 | 45 | 12 |
| `12fab7d` | 2026-07-16 | Marcelo Miranda | fix(nodered): corrige comando REPRESENT, watchdog de nós e persistência | 14 | 1.850 | 410 |
| `e950ced` | 2026-07-16 | Marcelo Miranda | feat(nodered): Canteiro B, atribuição por apresentação e fix do dropdown | 18 | 2.300 | 650 |
| `8cf43fe` | 2026-07-21 | Marcelo Miranda | chore: initialize project-wide documentation standards and add hardware/software architecture files for multiple sensor nodes | 64 | 8.900 | 120 |
| `ec8e676` | 2026-07-28 | Marcelo Miranda | feat: add sensorDrivers header and update graphify build cache | 45 | 4.200 | 850 |
| `6b0c7a0` | 2026-08-03 | Marcelo Miranda | feat: initialize M360-DRY architecture, documentation, and hardware abstraction layer for node firmware | 112 | 18.400 | 3.200 |
| `33afb5e` | 2026-08-06 | Marcelo Miranda | feat: Add explicit authorization for MCP node-red in bmad-agent-node-red-dev principles | 2 | 35 | 0 |
| `5a65af7` | 2026-08-06 | Marcelo Miranda | refactor: Limpeza estrutural do Node-RED (Remoção de duplicatas e implantação do Global Catch) | 8 | 980 | 1.450 |
| `d5d522f` | 2026-08-06 | Marcelo Miranda | fix: Bifurcação das mensagens MQTT para garantir captura completa no Log do Dashboard | 4 | 180 | 40 |
| `065efd1` | 2026-08-06 | Marcelo Miranda | feat: Orquestração do Agente Repórter da Horta via Node-RED | 26 | 3.800 | 180 |
| `b119850` | 2026-08-06 | Marcelo Miranda | fix(nodered): Altera caminho absoluto para relativo no armazenamento de logs do Agente (evita ENOENT) | 3 | 45 | 45 |
| `265469e` | 2026-08-06 | Marcelo Miranda | fix(nodered): Adiciona rotina de inicialização automática (auto-create) para o arquivo de log para prevenir ENOENT na IA | 5 | 120 | 20 |
| `58ce360` | 2026-08-06 | Marcelo Miranda | fix(nodered): Migra Agent Reporter para ler logs da memória RAM (eliminando falhas ENOENT do File System) | 6 | 310 | 180 |
| `06338d8` | 2026-08-07 | Marcelo Miranda | refactor: reorganizar estrutura DRY (horta e kit-helio) e agregar platformio.ini via extra_configs | 34 | 4.800 | 1.850 |
| `c05fb7e` | 2026-08-07 | Marcelo Miranda | feat: implement power profile management and integrate MySensors library for node communication | 82 | 12.400 | 4.200 |
| `e6ca9df` | 2026-08-08 | Marcelo Miranda | feat: implement miniDHT sensor node with full documentation and platformio configuration | 3288 | 1.788.084 | 6.782 |
