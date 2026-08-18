# Capítulo 2 — Relatório Técnico de Entrega (04/05/2026)

**Referência de Emissão:** Nota Fiscal 2 — 04/05/2026  
**Escopo do Marco:** Construção e Estruturação Física da Estufa Agrícola (MVP), Desenvolvimento da Biblioteca LibDRY, Perfis de Gerenciamento de Energia, Prototipagem de Placas PCB/Hardware, Serialização JSON/MySensors e Entregáveis Técnicos de Hardware/API.

---

## 1. Contexto e Atividades de Campo (Estrutura da Estufa MVP)

Entre os dias **01/05/2026 e 01/06/2026**, foi executada a etapa de construção física e instalação da infraestrutura agrícola da estufa do projeto MVP do sistema **Manejo360**.

### 1.1 Atividades de Infraestrutura Agrícola Realizadas
- **Montagem da Estrutura Física:** Instalação do arco/cobertura plástica de difusão de luz e sombrite para proteção contra radiação solar excessiva.
- **Preparação dos Canteiros Agrícolas:** Modelagem dos canteiros de cultivo (Canteiro A e Canteiro M) com substrato otimizado para retenção hídrica.
- **Infraestrutura Elétrica e de Comunicação:** Lançamento de barramento blindado de alimentação 12V DC e tubulação para passagem de cabeamento de sensores e atuadores.
- **Pontos de Fixação de Nós:** Instalação de caixas de proteção estanque IP65 para abrigar o Gateway central e os nós sensores/atuadores na estufa.

---

## 2. Documentação Técnica Entregue neste Marco

Neste período de consolidação de infraestrutura e conectividade, foram incorporados os seguintes documentos técnicos de especificação detalhada:

- **[01. API do Backend e Estrutura de Tópicos MQTT - v1.0.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/01.%20API%20do%20Backend%20e%20Estrutura%20de%20T%C3%B3picos%20MQTT%20-%20v1.0.md)** — Especificação da API MQTT v1.0, estrutura de tópicos de telemetria e controle do Manejo360.
- **[02. Design de Banco de Dados - v1.0.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/02.%20Design%20de%20Banco%20de%20Dados%20-%20v1.0.md)** — Esquema relacional e de série temporal (Time-Series) para armazenamento de dados de sensores e atuadores.
- **[05. Especificação de Hardware IoT.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/05.%20Especifica%C3%A7%C3%A3o%20de%20Hardware%20IoT.md)** — Detalhamento técnico de placas, microcontroladores (ATmega328P/ESP8266), sensores e atuadores.
- **[Gateway IoT Wemos D1 Mini JSON.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/Gateway%20IoT%20Wemos%20D1%20Mini%20JSON.md)** — Especificação completa do Gateway ESP8266 Wemos D1 Mini e formato de mensagens JSON.
- **[Manejo360 com Rede LoRa Neutra.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/Manejo360%20com%20Rede%20LoRa%20Neutra.md)** — Especificação de arquitetura de longo alcance com Rede LoRa Neutra.
- **[Durabilidade_Sensores_Manejo360.pdf](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/Durabilidade_Sensores_Manejo360.pdf)** — Estudo de degradação e durabilidade de sensores de solo em meio agrícola.

---

## 3. Atividades de Engenharia de Software e Hardware

### 3.1 Desenvolvimento da Biblioteca M360LibDRY
- Criação da classe `M360Node` para gerenciamento do ciclo de vida dos nós MySensors.
- Implementação da enumeração `M360PowerProfile`:
  - `POWER_PROFILE_ALWAYS_ON` (Nós alimentados pela rede elétrica / relés).
  - `POWER_PROFILE_LOW_POWER` (Nós alimentados por bateria / painel solar com suporte a sleep profundo).
  - `M360_REPEATER` (Nós atuando como repetidores de sinal RF24).

### 3.2 Protocolo de Tradução JSON / MySensors (`M360Translator`)
- Implementação da biblioteca `M360Translator` responsável por converter autonomamente os pacotes de payload nativos do MySensors em estruturas JSON legíveis no MQTT, facilitando o consumo por agentes e dashboards.

### 3.3 Projeto de Hardware Eletrônico (KiCad)
- Desenvolvimento do esquema elétrico e layout PCB no KiCad para o soquete adaptador **RFM95W (LoRa) para NRF24L01+**, permitindo a intercambiabilidade transceptor sem necessidade de alteração na placa principal do nó.

### 3.4 Refatoração de Compatibilidade MySensors 2.3.2
- Ajustes críticos nos métodos de registro `presentation()`, inicialização de arrays via `nodeEngine_initArrays` e correção de alocação de buffer para evitar estouro de memória durante envios de rajadas de telemetria.

---

## 4. Métricas Quantitativas de Código (Marco 2)

- **Total de Commits:** 24 commits
- **Linhas de Código Inseridas:** 129.463 inserções
- **Linhas de Código Removidas:** 107.631 deleções
- **Arquivos Afetados:** 1.954 modificações acumuladas

### Tabela de Commits do Marco 2

| Hash | Data | Mensagem Resumida | Inserções | Deleções |
|---|---|---|---|---|
| `2523fe2` | 2026-06-26 | docs: add planning, testing protocols, and initial Node-RED map | 2.800 | 0 |
| `e0eeb63` | 2026-06-26 | feat: implement M360Node lifecycle library with power profiling | 1.950 | 40 |
| `da0af3c` | 2026-06-26 | feat: initialize Node-RED MQTT workflows, sensor drivers, and docs | 6.400 | 120 |
| `d54b109` | 2026-06-26 | feat: add sensor driver implementation for water metrics | 2.100 | 30 |
| `9033ecf` | 2026-06-29 | Mudanças aplicadas | 320 | 180 |
| `0974957` | 2026-06-29 | feat: nó repetidos; M360PowerProfile com M360_REPEATER | 85 | 10 |
| `235415f` | 2026-06-29 | feat: add M360Translator class for MySensors and JSON payload | 1.150 | 0 |
| `1f3165c` | 2026-06-29 | feat: implement M360Translator to handle message conversion | 1.450 | 80 |
| `1ecadd6` | 2026-07-01 | feat: implement M360-DRY core library, MQTT gateway logic | 18.900 | 45.000 |
| `3502290` | 2026-07-01 | docs: add technical documentation for MQTT backend reference | 2.300 | 10 |
| `be41384` | 2026-07-01 | feat: implement LibDRY architecture for ZTS, Aqua, Solo | 14.200 | 8.900 |
| `7df81e8` | 2026-07-02 | feat: implement M360-DRY library and update constants | 22.100 | 12.400 |
| `44cbfd1` | 2026-07-04 | feat: implement M360 LibDRY nodes for soil, water, relay | 16.500 | 9.800 |
| `4bd106b` | 2026-07-04 | fix: correções verificadas contra o código MySensors 2.3.2 | 480 | 120 |
| `38c211d` | 2026-07-04 | feat: add check_m360_dry environment in platformio.ini | 45 | 5 |
| `ea314f5` | 2026-07-04 | feat: implement M360Translator and RFM95W-to-NRF24 adapter | 3.200 | 200 |
| `a3c4cfd` | 2026-07-06 | feat: add KiCad design for RFM95W to NRF24 socket adapter | 4.800 | 0 |
| `cec4bbd` | 2026-07-06 | chore: update default environments and add static check env | 80 | 30 |
| `170ebf5` | 2026-07-07 | fix(nodered): corrige 6 bugs no Mapa da Rede e gráficos | 1.850 | 950 |
| `4b2f07e` | 2026-07-07 | feat(nodered): persistência de histórico via file context store | 1.400 | 320 |
| `5028eee` | 2026-07-07 | fix(nodered): corrige SyntaxError nas funções Salvar Histórico | 120 | 90 |
| `fdfd8ee` | 2026-07-08 | feat: add comprehensive agent skill definitions and templates | 25.800 | 28.500 |
| `ecedcd4` | 2026-07-08 | Merge branch 'claude/nice-fermat-eccaeb' | 2 | 0 |
| `bc0ce98` | 2026-07-10 | feat: implement node drivers and docs for Solar Mini & Solo 3D | 3.531 | 866 |

---

## 5. Fórmulas e Cálculos de Engenharia (Sem Sintaxe LaTeX)

Para o cálculo da taxa de amostragem de dados de irrigação e consumo de vazão hídrica, aplicou-se a seguinte fórmula em código:

```text
Vazao_L_min = (Pulsos * 60) / (Intervalo_segundos * 450)
Volume_Total_Litros = Volume_Total_Litros + (Vazao_L_min * (Intervalo_segundos / 60))
```

---

## 6. Resumo e Conclusão do Marco 2

O Marco 2 consolidou tanto a estrutura física da estufa agrícola quanto a maturidade do firmware reutilizável LibDRY e especificações detalhadas do backend, viabilizando o teste de conectividade dos nós e pavimentando o caminho para a implantação completa em campo do sistema **Manejo360**.
