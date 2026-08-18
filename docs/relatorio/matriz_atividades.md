# Matriz de Rastreabilidade: Commits vs. Atividades de Engenharia, Entregáveis Documentais e Campo

Este documento correlaciona o histórico de modificações do repositório Git com as atividades formais de engenharia de software, especificação de hardware, entregáveis de documentação técnica do sistema **Manejo360** e comissionamento físico em campo.

---

## 1. Mapeamento de Campo e Estrutura Física

| Período de Execução | Subsistema / Atividade Físico-Operacional | Componentes & Módulos Associados | Commits e Artefatos de Firmware/HW |
|---|---|---|---|
| **01/05 a 01/06/2026** | **Construção e Estruturação Física da Estufa MVP** | Estrutura física agrícola, canteiros A e M, barramento elétrico 12V/5V, tubulações e suportes de fixação. | `docs/MVP-Planejamento da estufa.md`, `docs/MVP-Protocolo de Testes Canteiros A e M.md` |
| **01/07 a 31/07/2026** | **Instalação Física e Comissionamento do Gateway** | ESP8266 + NRF24L01+ + Módulo WiFi/MQTT e interface web AP. | `src/DRY/horta/gateway/`, `ngm/` (config, wifi, mqtt, leds, webserver) |
| **01/07 a 31/07/2026** | **Instalação Física e Comissionamento do Nó 01 (Solo 3D)** | `01nodeSolo3dNano` em canteiro agrícola + 3 sensores brutos ADC (0-1023) profundidades 10cm, 20cm, 30cm. | `src/DRY/horta/nos/01nodeSolo3dNano/`, `01nodeSolo3dMini.cpp` |
| **01/07 a 31/07/2026** | **Instalação Física e Comissionamento do Nó 02 (Solo 3D)** | `02nodeSolo3dNano` em canteiro agrícola + 3 sensores brutos ADC (0-1023) profundidades 10cm, 20cm, 30cm. | `src/DRY/horta/nos/02nodeSolo3dNano/` |
| **01/07 a 31/07/2026** | **Instalação Física e Comissionamento do Nó 04 (Solar Mini)** | `04noodeSolarMini` (Estação Solar Mini) + DHT11 (Temp/Umidade do ar) + painel solar e perfil low-power. | `src/DRY/horta/nos/04noodeSolarMini/` |
| **01/07 a 31/07/2026** | **Instalação Física e Comissionamento do Nó 99 (Relés/Solenoide)** | `99nodeReles` (Controlador de válvulas solenoides de irrigação e acionamento MUX). | `src/DRY/horta/nos/99nodeReles/`, `diagrama_blocos.svg` |
| **01/07 a 31/07/2026** | **Integração de 12 Sensores em Campo** | 6x Umidade do Solo (ADC), 2x Temperatura do Solo (DS18B20), 2x Temp/Umidade Ar (DHT11), 1x pH do Solo, 1x Medidor de Vazão YF-S201. | `sensorDrivers.cpp`, `node_engine.h` |

---

## 2. Rastreabilidade dos Entregáveis Documentais Incorporados

| Código do Documento | Título do Entregável | Marco / NF | Status & Localização |
|---|---|---|---|
| **DOC-00** | Documento Técnico Manejo360 | Marco 1 (13/02/2026) | Incorporado (`00. Documento Técnico  Manejo360.md`) |
| **DOC-01** | API do Backend e Estrutura de Tópicos MQTT v1.0 | Marco 2 (04/05/2026) | Incorporado (`01. API do Backend e Estrutura de Tópicos MQTT - v1.0.md`) |
| **DOC-02** | Design de Banco de Dados v1.0 | Marco 2 (04/05/2026) | Incorporado (`02. Design de Banco de Dados - v1.0.md`) |
| **DOC-03** | Motor de Inferências e DSL de Regras v1.0 | Marco 3 (07/08/2026) | Incorporado (`03. Motor de Inferências e DSL de Regras - v1.0.md`) |
| **DOC-04** | Integração com Protocolo MCP v1.1 | Marco 3 (07/08/2026) | Incorporado (`04. Integração com Protocolo MCP - v1.1.md`) |
| **DOC-05** | Especificação de Hardware IoT | Marco 2 (04/05/2026) | Incorporado (`05. Especificação de Hardware IoT.md`) |
| **DOC-06** | Arquitetura IoT Manejo360 | Marco 1 (13/02/2026) | Incorporado (`Arquitetura IOT.md`) |
| **DOC-07** | Business Model Canvas Manejo360 v3.2 | Marco 1 (13/02/2026) | Incorporado (`BMC  Manejo360 v_3.2.md` / `pdf`) |
| **DOC-08** | Mapeamento de Dor e Persona | Marco 1 (13/02/2026) | Incorporado (`Dor e Persona.md`) |
| **DOC-09** | Durabilidade de Sensores Manejo360 | Marco 2 (04/05/2026) | Incorporado (`Durabilidade_Sensores_Manejo360.pdf`) |
| **DOC-10** | Gateway IoT Wemos D1 Mini JSON | Marco 2 (04/05/2026) | Incorporado (`Gateway IoT Wemos D1 Mini JSON.md`) |
| **DOC-11** | Manejo360 com Rede LoRa Neutra | Marco 2 (04/05/2026) | Incorporado (`Manejo360 com Rede LoRa Neutra.md`) |
| **DOC-12** | Pitch Deck Convergência-GO Manejo360 | Marco 1 (13/02/2026) | Incorporado (`PicthDeck_(Convergencia-GO)_Manejo 360.pdf`) |
| **DOC-13** | Especificações MySensors | Marco 3 (07/08/2026) | Incorporado (`MySensors/`) |

---

## 3. Matriz de Atividades de Desenvolvimento por Marco de NF

### 3.1 Marco 1 — 13/02/2026 (Concepção & Arquitetura Base)

| ID Atividade | Categoria | Descrição da Atividade de Engenharia | Commits Relacionados |
|---|---|---|---|
| **ATV-M1-01** | Arquitetura | Estruturação da árvore monorepo e convenções da arquitetura Manejo360. | `1dbd524` |
| **ATV-M1-02** | Firmware | Criação de drivers base de sensores e suporte inicial ao nó `80nodeAqua`. | `4292d4f`, `dac8fd6` |
| **ATV-M1-03** | Gateway | Integração do gateway ESP8266 com abstração de conectividade WiFi/MQTT. | `6d88b02`, `7bd97b3` |
| **ATV-M1-04** | Protocolo | Centralização e validação de tópicos MQTT padronizados (`buildTopicGatewayStatus`). | `9cb7511` |
| **ATV-M1-05** | Qualidade | Execução de code review estático corrigindo 15 vulnerabilidades/bugs de estouro de buffer. | `9d04285` |

---

### 3.2 Marco 2 — 04/05/2026 (Infraestrutura, LibDRY & Estufa MVP)

| ID Atividade | Categoria | Descrição da Atividade de Engenharia | Commits Relacionados |
|---|---|---|---|
| **ATV-M2-01** | Documentação | Elaboração de protocolos de teste de bancada e plano do MVP da Estufa Agrícola. | `2523fe2`, `3502290` |
| **ATV-M2-02** | Firmware DRY | Desenvolvimento da biblioteca `M360Node` com perfis de energia (`M360PowerProfile`). | `e0eeb63`, `0974957` |
| **ATV-M2-03** | Protocolo | Criação da classe `M360Translator` para conversão bidirecional entre MySensors e JSON. | `235415f`, `1f3165c` |
| **ATV-M2-04** | Node-RED | Implementação dos fluxos de recebimento de telemetria e gráficos no Dashboard. | `da0af3c`, `4b2f07e` |
| **ATV-M2-05** | Hardware PCB | Projeto eletrônico no KiCad do adaptador de soquete RFM95W (LoRa) para NRF24L01+. | `ea314f5`, `a3c4cfd` |
| **ATV-M2-06** | MySensors | Refatoração de compatibilidade com a biblioteca MySensors v2.3.2. | `4bd106b`, `7df81e8` |

---

### 3.3 Marco 3 — 07/08/2026 (Instalação Campo, 4 Nós, Gateway & Node-RED)

| ID Atividade | Categoria | Descrição da Atividade de Engenharia | Commits Relacionados |
|---|---|---|---|
| **ATV-M3-01** | Atuadores | Implementação do controlador de válvulas/relés `99nodeReles` com multiplexação. | `3c70283` |
| **ATV-M3-02** | Protocolo | Correção do dimensionamento de buffer de mensagens MySensors (`messages[NODE_ITEMS_COUNT + 3]`). | `6a097e4` |
| **ATV-M3-03** | Node-RED | Implementação do subsistema de auto-apresentação `REPRESENT` e atribuição por canteiro. | `12fab7d`, `e950ced` |
| **ATV-M3-04** | Agente IA | Orquestração do Agente Repórter da Horta via Node-RED com logging em RAM para estabilidade de I/O. | `065efd1`, `58ce360` |
| **ATV-M3-05** | Monorepo DRY | Reorganização das plataformas Horta e Kit-Hélio agregadas no PlatformIO via `extra_configs`. | `06338d8` |
| **ATV-M3-06** | Motor DRY | Padronização macros `node_engine.h` (apresentação, ciclo de bateria, intervalos EEPROM). | `6b0c7a0`, `c05fb7e` |
| **ATV-M3-07** | Sensor Nó | Implementação do nó `miniDHT` com documentação e configuração PlatformIO isolada. | `e6ca9df` |
