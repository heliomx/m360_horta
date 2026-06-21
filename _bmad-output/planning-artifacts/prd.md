---
stepsCompleted: ["engenharia-reversa-autonoma"]
inputDocuments: ["docs/prd.md", "docs/epics.md", "docs/architecture.md", "codebase"]
workflowType: 'prd'
classification:
  projectType: iot_embedded
  domain: agrotech
  complexity: high
  projectContext: brownfield
---

# Product Requirements Document - M360 Horta
**Autor:** John (BMAD PM Agent)
**Data:** 2026-06-21

## 1. Visão do Produto e Objetivos
O projeto **M360 Horta** é um ecossistema IoT avançado de telemetria e automação agrícola. Ele tem como objetivo primário otimizar o uso de recursos hídricos e monitorar a saúde do cultivo de forma hiper-localizada, integrando nós sensores de baixo consumo e atuadores de grande porte (bombas, válvulas) gerenciados por uma arquitetura local robusta.

**Objetivos de Negócio:**
- **Zero Desperdício Hídrico:** Irrigação baseada em dados reais de umidade do solo e medição exata de vazão, não em temporizadores cegos.
- **Resiliência Local:** A automação primária e coleta de dados devem operar em rede de rádio local (MySensors) imune a quedas de internet.
- **Governança de Dados via Nuvem:** Integração transparente de todos os sensores e atuadores para uma plataforma central via MQTT.

## 2. Abordagem Arquitetural (DRY Core)
A base tecnológica do projeto é orientada à eliminação de código redundante e estabilidade operacional.
- **Motor Central M360-DRY:** Nós implementam uma biblioteca `lib/M360-DRY` que orquestra setup, sleep mode, gerenciamento de bateria e envio/recepção MySensors (`M360Node`). O código em nível de nó concentra-se puramente em "qual sensor estou lendo" ou "que relé estou acionando" via `sensorDrivers`.
- **Master Gateway:** O cérebro da rede local (rodando em ESP8266). Realiza a ponte entre a malha MySensors de 2.4GHz e a infraestrutura MQTT/IP. Empacota e desempacota payloads JSON para minimizar o tráfego de rádio e otimizar a interação com o App/Nuvem.
- **Gestão de Energia Estrita:** A frota é dividida em perfis operacionais: `ALWAYS_ON` (atuadores) e `LOW_POWER` (sensores de bateria).

## 3. Atores do Sistema (A Frota de Dispositivos)

A rede é composta por "Monitores" (Sensoriamento) e "Atuadores" (Ação).

### 3.1 Monitores (Sensoriamento - *Low Power*)
São alimentados por bateria ou painel solar. Possuem janelas de escuta muito curtas e passam a maior parte do tempo em *Deep Sleep*. Além disso, pulsam a alimentação física dos sensores apenas durante a leitura para evitar degradação galvânica no solo.
- **Nó 01 (Solo 3D):** Responsável por leituras de temperatura e umidade profundas.
- **Nó 13 (ZTS Umidade Hall):** Nó profissional (Modbus/RS485). Lê até 7 parâmetros químicos e físicos do solo (NPK, pH, Condutividade, Temp, Umid), exigindo aquecimento prévio de 12V via relé local.
- **Nó 80 (Monitoramento de Caixas d'Água):** Realiza telemetria focada no armazenamento e linhas de vazão (sensores YF-S201).
- Outros nós climáticos focados em aferição atmosférica (DHT11, luminosidade).

### 3.2 Atuadores (*Always On*)
Estão permanentemente conectados à energia e escutando a rede. Respondem instantaneamente a comandos.
- **Nó Bomba (`nodePump`):** Aciona o motor hídrico principal. Incorpora lógicas defensivas de "timeout" para jamais permitir que a bomba queime trabalhando a seco se a rede falhar.
- **Nó Solenoide / Relés (`node99Reles` / `nodeSelenoieVazao`):** Controla a liberação da água bombeada para setores específicos do plantio, medindo de forma síncrona o volume exato injetado na terra.

## 4. Requisitos Funcionais (FR)

### Camada de Aquisição e Ação (Nós)
| ID | Req Funcional | Ator Associado | Criticidade |
|---|---|---|---|
| **FR01** | **Atuação Hídrica com Failsafe:** A bomba principal e válvulas devem aceitar comandos remotos de ligar/desligar. Devem também implementar tempo máximo de funcionamento autônomo. | Atuadores | P0 |
| **FR02** | **Aferição de Vazão em Tempo Real:** Enquanto a válvula estiver aberta, o sistema deve contar e reportar o volume d'água percorrido em litros reais. | Atuadores (Vazão) | P1 |
| **FR03** | **Coleta de Solo Modbus:** O sistema deve suportar sensores de agricultura de precisão (ZTS) que necessitam relés de 12V e comunicação RS485 bidirecional para ler NPK, Salinidade e pH. | Nós Avançados (13) | P1 |
| **FR04** | **Leituras Ambientais e Solo Básico:** Aferir Temperatura do ar (DHT), umidade pontual no solo, resistividade e luminosidade externa, enviando como valores normalizados (0-100% para umidades de solo). | Nós Sensores | P1 |
| **FR05** | **Prevenção de Corrosão:** A voltagem para sensores que ficam enterrados na terra deve ser acionada eletricamente apenas nos milissegundos que precedem e duram a leitura da porta analógica. | Nós Sensores | P0 |

### Camada de Orquestração (Gateway)
| ID | Req Funcional | Ator Associado | Criticidade |
|---|---|---|---|
| **FR06** | **Tradução Bidirecional (Radio <-> MQTT):** Converter estados binários/analógicos da malha MySensors para JSONs compreensíveis (max 512 bytes), e traduzir JSONs que chegam da nuvem em comandos de rádio direcionados (`V_STATUS`, `V_VAR1`). | Gateway | P0 |
| **FR07** | **Provisionamento Inteligente de Rede:** O Gateway deve subir um Access Point nativo com formulário Web caso não encontre rede WiFi ou não tenha configuração MQTT válida armazenada em sua EEPROM. | Gateway | P1 |
| **FR08** | **Gestão de Saúde de Malha:** Emitir "Heartbeats" da rede para a nuvem a cada 60s, informando sinal WiFi e indicativos vitais da base. | Gateway | P2 |
| **FR09** | **Comandos Especiais Globalizados:** Roteamento de comandos de meta-dados, como `CMD_FORCE_UPDATE` ou `SET_INTERVAL`, garantindo que os nós reprogramem seus tempos de *sleep* no próximo despertar. | Gateway | P1 |

## 5. Requisitos Não Funcionais (NFR)

- **NFR01 (Padronização Código Base):** Qualquer nó da frota **deve** usar a biblioteca unificada `M360Node` e declarar sua lógica de interface física exclusivamente dentro do arquivo `sensorDrivers.cpp`. Nunca implementar loops NRF diretamente.
- **NFR02 (Concorrência e Perda de Pacotes):** Atuadores como medidores de fluxo de água usam contagem por interrupções físicas (ex: `PCINT1`). O código não deve bloquear estas interrupções (usar `wait()` do rádio, não `delay()`), caso contrário a aferição de litros será incorreta.
- **NFR03 (Eficiência de Payload):** Para economizar banda local, comandos e estados MySensors são otimizados. Os `Child IDs` `254` e `255` são estritamente reservados no projeto inteiro para `V_VAR1` (Intervalos) e `V_VOLTAGE` (Bateria).
- **NFR04 (Persistência com Proteção):** O Gateway usa a EEPROM para persistir WiFI/MQTT, utilizando a região > 512 bytes (fugindo do reservado pelo core do MySensors), armazenando um `CRC` seguro e garantindo gravação via `commit()`.

## 6. Casos de Uso e Protocolo de MVP (Irrigação Inteligente)
Para validar a eficiência e o valor do produto, o sistema está inserido em um MVP focado em cultivo de alface (Crespa e Roxa), estruturado em um teste A/B para comparar o **Manejo360 (Canteiro A)** contra o **Manejo Tradicional temporizado (Canteiro B)**.

### 6.1 Parâmetros de Automação e Foco no Produto (Canteiro A)
- **Posicionamento de Sensores:** 10 cm e 25 cm de profundidade (foco na zona radicular ativa).
- **Lógica de Gatilho (Trigger):** A bomba/solenoide do ecossistema liga de forma autônoma quando a tensão da água no solo atinge o limite crítico para a cultura (**20-30 kPa**) e desliga imediatamente ao atingir a capacidade de campo.

### 6.2 Indicadores de Sucesso (KPIs) e Métricas Proprietárias
O sucesso e viabilidade do produto serão medidos cruzando dados precisos de telemetria e biometria final, incluindo métricas exclusivas:
- **Eficiência Hídrica (kg/L) e Economia:** Comparativo de consumo real de água (via dados contínuos de fluxo) vs. produtividade (Massa Fresca Total).
- **Índice Manejo360:** Razão entre a Eficiência Hídrica (kg/L) do sistema inteligente e a do sistema tradicional. Funciona como a métrica de "estrela-guia" para comprovar a superioridade do produto.
- **Índice de Estabilidade Hídrica:** Mede a consistência da umidade no solo do Canteiro A. Menor oscilação gráfica significa menor estresse radicular.
- **Payback e Custo por Kg:** Extrapolação dos dados para atestar o retorno sobre o investimento do equipamento IoT no campo.

---
*Artefato atualizado com protocolos de experimentação do MVP.*
