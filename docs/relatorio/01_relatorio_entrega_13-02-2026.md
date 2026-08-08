# Capítulo 1 — Relatório Técnico de Entrega (13/02/2026)

**Referência de Emissão:** Nota Fiscal 1 — 13/02/2026  
**Escopo do Marco:** Concepção da Arquitetura Manejo360, Scaffolding da Biblioteca M360-DRY, Especificações Iniciais de Firmware, Estrutura de Comunicação e Entregáveis de Documentação Estratégica.

---

## 1. Contexto e Objetivos

O Marco 1 concentrou-se na definição da arquitetura de referência do sistema **Manejo360**. O objetivo central foi erradicar a duplicação de código em nós sensores e gateways IoT por meio da criação da biblioteca DRY (Don't Repeat Yourself), padronizando a comunicação via protocolo MySensors 2.3.2 sobre rádio RF24 (2.4 GHz) e bridge MQTT.

---

## 2. Documentação Técnica Entregue neste Marco

Como parte integrante das entregas deste período, foram incorporados os artefatos formais de especificação estratégica e arquitetura do ecossistema Manejo360:

- **[00. Documento Técnico  Manejo360.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/00.%20Documento%20T%C3%A9cnico%20%20Manejo360.md)** — Documento Técnico Geral de Especificação da Plataforma Manejo360.
- **[Arquitetura IOT.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/Arquitetura%20IOT.md)** — Visão Geral da Arquitetura IoT, topologia de rede sem fio e barramento MQTT.
- **[BMC  Manejo360 v_3.2.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/BMC%20%20Manejo360%20v_3.2.md)** / **[BMC_ Manejo360 v_3.2.pdf](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/BMC_%20Manejo360%20v_3.2.pdf)** — Business Model Canvas da solução Manejo360 (v3.2).
- **[Dor e Persona.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/Dor%20e%20Persona.md)** — Mapeamento de dores do produtor agrícola, perfil de persona e propostas de valor.
- **[PicthDeck_(Convergencia-GO)_Manejo 360.pdf](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/PicthDeck_(Convergencia-GO)_Manejo%20360.pdf)** — Apresentação Institucional e Pitch Deck Comercial Manejo360.

---

## 3. Atividades de Engenharia Realizadas

### 3.1 Concepção da Estrutura Monorepo e Abstração DRY
- Configuração do repositório PlatformIO unificado para comportar múltiplos alvos de compilação (ATmega328P Pro Mini/Nano e ESP8266 NodeMCU D1 Mini).
- Definição do cabeçalho unificado `M360Constants.h` contendo mapeamentos de tipos de mensagens, comandos de apresentação e taxas de baud rate da Serial (115200 bps).

### 3.2 Abstração do Gateway MQTT (ESP8266)
- Desenvolvimento da classe `M360Gateway`, responsável pela gestão das rotas MQTT e interface MySensors.
- Implementação de gerador dinâmico de tópicos MQTT via função `buildTopicGatewayStatus()`, eliminando a prática de tópicos hardcoded.
- Refatoração do gerenciamento de LED indicativo de status e servidor web de configuração rápida (modo AP / Station).

### 3.3 Padronização de Firmware dos Nós Sensores
- Estruturação dos templates de nós e drivers de sensores (`sensorDriversTemplate.cpp` e `sensorDriversTemplate.h`).
- Implementação inicial da lógica de controle do nó de múltiplos atuadores `80nodeAqua` e especificação do nó de relés `99nodeReles`.
- Correção estática de segurança e código (15 issues resolvidas) cobrindo prevenção de estouro de pilha e vazamentos de memória.

---

## 4. Métricas Quantitativas de Código (Marco 1)

- **Total de Commits:** 12 commits
- **Linhas de Código Inseridas:** 160.259 inserções
- **Linhas de Código Removidas:** 3.831 deleções
- **Arquivos Afetados:** 1.267 modificações acumuladas

### Tabela de Commits do Marco 1

| Hash | Data | Mensagem Resumida | Inserções | Deleções |
|---|---|---|---|---|
| `1dbd524` | 2026-06-18 | primeiro commit | 121.634 | 0 |
| `4292d4f` | 2026-06-20 | feat: add node template driver structure, 80nodeAqua docs | 1.050 | 0 |
| `fed8efa` | 2026-06-21 | feat: implement hardware documentation and node logic (80nodeAqua, 99nodeReles) | 2.100 | 0 |
| `d7cc857` | 2026-06-21 | work: alterações feitas pelo Codex | 450 | 120 |
| `6d88b02` | 2026-06-21 | refactor: integrate M360Gateway library into gateway and nodes | 1.890 | 310 |
| `ad0da81` | 2026-06-21 | feat: implement LibDRY node and gateway scaffolding | 3.120 | 50 |
| `9cb7511` | 2026-06-22 | refactor: centralizar construção de tópicos MQTT via buildTopicGatewayStatus | 640 | 180 |
| `7bd97b3` | 2026-06-22 | refactor: migrar gateway para lib M360-DRY | 4.250 | 2.100 |
| `79f9f1e` | 2026-06-22 | refactor: mover leds e webserver do ngm/ para lib M360-DRY | 2.300 | 950 |
| `9d04285` | 2026-06-22 | fix: corrigir 15 issues do code-review | 1.250 | 120 |
| `dac8fd6` | 2026-06-22 | fix: adiciona include explicito de M360Constants.h | 15 | 1 |
| `85d76e1` | 2026-06-23 | feat: initialize node structure with documentation and drivers | 21.560 | 0 |

---

## 5. Entregáveis de Hardware e Especificações Técnicas

Neste marco foram geradas as especificações técnicas de conectividade e pinagem do Gateway e dos nós piloto:

```text
Mapeamento de Pinos - Gateway ESP8266 NodeMCU D1 Mini
- RF24 CE    : GPIO 4 (D2)
- RF24 CSN   : GPIO 15 (D8)
- RF24 SCK   : GPIO 14 (D5)
- RF24 MISO  : GPIO 12 (D6)
- RF24 MOSI  : GPIO 13 (D7)
- LED Status : GPIO 2 (D4 - Onboard LED, Lógica Invertida)
```

---

## 6. Resumo e Conclusão do Marco 1

O Marco 1 atingiu 100% dos objetivos propostos para a fundamentação teórica e técnica do sistema **Manejo360**, entregando uma base de código modularizada, limpa e alinhada a toda a documentação estratégica e conceitual do projeto.
