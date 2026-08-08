# Entregáveis de Especificação e Arquitetura Técnica — Sistema M360 (Manejo360)

Este documento consolida os artefatos formais de especificação técnica, arquitetura de software, banco de dados, motor de inferências, modelo de negócios e protocolo MCP do sistema **M360** (Manejo360), incorporados como entregáveis estruturais do repositório.

---

## 1. Inventário dos Entregáveis Técnicos Incorporados

| Código Entregável | Nome do Documento | Descrição e Escopo Técnico |
|---|---|---|
| **ENT-DOC-00** | `00. Documento Técnico Manejo360.md` | Documento Técnico Avançado: Visão geral do Sistema de Cultivo Baseado em Dados M360, conceito de Módulos de Manejo e extração assistida por IA. |
| **ENT-DOC-01** | `01. API do Backend e Estrutura de Tópicos MQTT - v1.0.md` | Especificação completa da API RESTful do backend (Node.js/Express) e taxonomia estrita dos tópicos MQTT para gateway e nós. |
| **ENT-DOC-02** | `02. Design de Banco de Dados - v1.0.md` | Modelagem de dados em MongoDB/PostgreSQL: coleções de telemetria, módulos de manejo, usuários, propriedades e regras operacionais. |
| **ENT-DOC-03** | `03. Motor de Inferências e DSL de Regras - v1.0.md` | Especificação da Linguagem de Domínio Específico (DSL) para avaliação de regras de irrigação, climáticas e manejo automatizado em tempo real. |
| **ENT-DOC-04** | `04. Integração com Protocolo MCP - v1.1.md` | Arquitetura de integração com o Model Context Protocol (MCP) para acoplamento de agentes de IA e interoperabilidade conversacional. |
| **ENT-DOC-05** | `05. Especificação de Hardware IoT.md` | Especificação física de pinagem, esquemas elétricos, sensores (solo, clima, pH), nós (ATmega328P) e gateway (ESP8266 NodeMCU). |
| **ENT-DOC-06** | `Arquitetura IOT.md` | Visão holística da arquitetura de comunicação IoT: Rede RF24 MySensors, Bridge Gateway ESP8266, Broker MQTT e Node-RED. |
| **ENT-DOC-07** | `BMC Manejo360 v_3.2.md` | Business Model Canvas (v3.2) detalhando proposta de valor, segmento de produtores agrícolas, estrutura de custos e fontes de receita. |
| **ENT-DOC-08** | `Dor e Persona.md` | Mapeamento empírico de dores do produtor agrícola de pequeno e médio porte e definição das personas de usuário da plataforma M360. |
| **ENT-DOC-09** | `Gateway IoT Wemos D1 Mini JSON.md` | Manual de implementação do Gateway ESP8266 Wemos D1 Mini com serialização JSON de mensagens MySensors. |
| **ENT-DOC-10** | `Manejo360 com Rede LoRa Neutra.md` | Estudo de expansão da conectividade agrícola via transceptores RFM95W e integração a redes LoRaWAN neutras. |

---

## 2. Resumo da Arquitetura M360 Integrada aos Entregáveis

### 2.1 Backend e Tópicos MQTT (`ENT-DOC-01`)
Os tópicos MQTT seguem a hierarquia padronizada do M360:
- **Telemetria de Saída (Gateway -> Backend):** `m360/out/{gateway_id}/{node_id}/{sensor_id}/{sub_type}`
- **Comandos de Entrada (Backend -> Gateway):** `m360/in/{gateway_id}/{node_id}/{sensor_id}/{sub_type}`

### 2.2 Motor de Inferências e DSL (`ENT-DOC-03`)
O motor de decisão avalia regras do tipo:
```text
SE (Umidade_Solo_10cm < 35%) E (Temperatura_Ar > 28 C)
ENTAO (Acionar Solenoide Irrigacao No 99 por 15 minutos)
E (Notificar Operador via Node-RED Dashboard)
```

### 2.3 Integração com Agentes via MCP (`ENT-DOC-04`)
O protocolo MCP expõe os sensores e atuadores da horta como recursos dinâmicos, permitindo que agentes LLM realizem consultas conversacionais ("Qual a umidade média do Canteiro A?") e executem ações auditadas.

---

## 3. Matriz de Vinculação dos Entregáveis com os Capítulos de NF

- **Capítulo 1 (NF 13/02/2026):** Incorporação de `ENT-DOC-00` (Doc Técnico), `ENT-DOC-05` (HW IoT) e `ENT-DOC-06` (Arquitetura IoT).
- **Capítulo 2 (NF 04/05/2026):** Incorporação de `ENT-DOC-07` (BMC), `ENT-DOC-08` (Dor e Persona), `ENT-DOC-09` (Gateway Wemos D1 Mini) e `ENT-DOC-10` (Rede LoRa Neutra).
- **Capítulo 3 (NF 07/08/2026):** Incorporação de `ENT-DOC-01` (API MQTT), `ENT-DOC-02` (Design DB), `ENT-DOC-03` (Motor de Inferências) e `ENT-DOC-04` (Integração MCP).
