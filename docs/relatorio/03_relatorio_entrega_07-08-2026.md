# Capítulo 3 — Relatório Técnico de Entrega (07/08/2026)

**Referência de Emissão:** Nota Fiscal 3 — 07/08/2026  
**Escopo do Marco:** Comissionamento e Instalação em Campo (Mês de Julho/2026) do Gateway ESP8266, Instalação dos 4 Nós Sensores/Atuadores (`01nodeSolo3dNano`, `02nodeSolo3dNano`, `04noodeSolarMini`, `99nodeReles`), Integração dos 12 Sensores Agrícolas, Orquestração do Agent Reporter no Node-RED, Motor de Inferências/MCP e Padronização do Motor DRY (`node_engine.h`).

---

## 1. Instalação e Comissionamento em Campo (Mês de Julho/2026)

Durante o mês de **Julho/2026**, a equipe técnica realizou a instalação física, montagem de bancada final, cabeamento e testes de operação contínua de 24/7 de todo o ecossistema IoT **Manejo360** na estufa.

### 1.1 Implantação do Gateway e dos 4 Nós de Campo

| Nó / Equipamento | Identificador / Caminho | Função Operacional em Campo | Sensores / Atuadores Conectados | Perfil de Energia |
|---|---|---|---|---|
| **Gateway Central** | `src/DRY/horta/gateway` | Ponte RF24 para MQTT bridge no broker Mosquitto. | Módulo NRF24L01+ com antena PA/LNA, ESP8266. | Always-On (5V DC) |
| **Nó 01 (Solo 3D Canteiro A)** | `01nodeSolo3dNano` | Monitoramento da frente de molhamento em 3 profundidades. | 3x Sensores de umidade do solo (ADC bruto 0 a 1023 em 10cm, 20cm, 30cm) + 1x DS18B20 Temp Solo. | Always-On / Low-Power |
| **Nó 02 (Solo 3D Canteiro M)** | `02nodeSolo3dNano` | Monitoramento da umidade em profundidade no Canteiro M. | 3x Sensores de umidade do solo (ADC bruto 0 a 1023 em 10cm, 20cm, 30cm) + 1x DS18B20 Temp Solo. | Always-On / Low-Power |
| **Nó 04 (Estação Solar Mini)** | `04noodeSolarMini` | Coleta microclimática aérea e monitoramento de bateria. | 1x DHT11 (Temp/Umidade do Ar) + 1x Sensor de pH do solo + Leitura de Tensão V_VOLTAGE (Pino 255). | Low-Power (Painel Solar) |
| **Nó 99 (Controlador Relés)** | `99nodeReles` | Atuação e controle automatizado de irrigação na estufa. | 4x Solenoides de irrigação multiplexadas via MUX + 1x Medidor de Vazão YF-S201. | Always-On (12V DC) |

---

## 2. Documentação Técnica Entregue neste Marco

Para a fase de automação avançada, regras de inferência e integração com Inteligência Artificial no Node-RED, foram entregues os seguintes documentos técnicos:

- **[03. Motor de Inferências e DSL de Regras - v1.0.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/03.%20Motor%20de%20Infer%C3%AAncias%20e%20DSL%20de%20Regras%20-%20v1.0.md)** — Definição do Motor de Inferências e Linguagem de Regras de Irrigação (DSL).
- **[04. Integração com Protocolo MCP - v1.1.md](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/04.%20Integra%C3%A7%C3%A3o%20com%20Protocolo%20MCP%20-%20v1.1.md)** — Integração do ecossistema Manejo360 com o protocolo MCP (Model Context Protocol) para os Agentes IA.
- **[MySensors](file:///d:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/ViridIoTech/Projetos/Manejo360/Documenta%C3%A7%C3%A3o/MySensors)** — Especificações complementares de integração MySensors e barramento sem fio.

---

## 3. Inventário Integrado dos 12 Sensores Instalados

1. **Sensor Umidade Solo 1 (10cm - Canteiro A):** Leitura capacitiva/resistiva analógica (0–1023).
2. **Sensor Umidade Solo 2 (20cm - Canteiro A):** Leitura capacitiva/resistiva analógica (0–1023).
3. **Sensor Umidade Solo 3 (30cm - Canteiro A):** Leitura capacitiva/resistiva analógica (0–1023).
4. **Sensor Umidade Solo 4 (10cm - Canteiro M):** Leitura capacitiva/resistiva analógica (0–1023).
5. **Sensor Umidade Solo 5 (20cm - Canteiro M):** Leitura capacitiva/resistiva analógica (0–1023).
6. **Sensor Umidade Solo 6 (30cm - Canteiro M):** Leitura capacitiva/resistiva analógica (0–1023).
7. **Sensor Temp Solo 1 (DS18B20 - Canteiro A):** Protocolo 1-Wire.
8. **Sensor Temp Solo 2 (DS18B20 - Canteiro M):** Protocolo 1-Wire.
9. **Sensor Temperatura Ar (DHT11 - Estação Solar):** Leitura digital.
10. **Sensor Umidade Ar (DHT11 - Estação Solar):** Leitura digital.
11. **Sensor pH Solo (Estação Solar):** Interface analógica / sonda de pH.
12. **Medidor de Vazão (YF-S201 - Nó 99):** Sensor Hall de pulso por litro.

---

## 4. Avanços de Software e Automação (Node-RED & Motor DRY)

### 4.1 Padronização do Motor DRY (`node_engine.h`)
- Implantação definitiva das macros padronizadas nos firmware dos nós:
  - `NODE_ENGINE_PRESENTATION(name, ver)` para auto-registro limpo.
  - `NODE_ENGINE_HANDLE_INTERVAL(msg)` para persistência de intervalo em EEPROM (Endereço 254).
  - `NODE_ENGINE_PROCESS_BATTERY(N)` para reporte de tensão da bateria (Endereço 255).
- Dimensionamento correto de buffers MyMessage: `messages[NODE_ITEMS_COUNT + 3]`.

### 4.2 Agente Repórter e Estabilidade do Node-RED
- Criação e orquestração do **Agent Reporter da Horta** no Node-RED.
- Solução técnica para estabilização de I/O: Migração do armazenamento de logs de eventos do sistema de arquivos para a **memória RAM** (File Context Store / RAM), eliminando falhas `ENOENT` e garantindo alta disponibilidade da IA Repórter.
- Implementação da rotina de limpeza estrutural e Global Catch para tratamento de exceções de MQTT.

### 4.3 Agregação Monorepo no PlatformIO
- Reorganização das configurações do PlatformIO via `extra_configs` em [platformio.ini](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/Manejo360/platformio.ini), permitindo a compilação paralela dos ambientes `d1_mini_gateway`, `ProMini_01nodeSolo3d`, `nano_02nodeSolo3d`, `nano_99reles`, `pro16MHz_miniDHT`, entre outros.

---

## 5. Métricas Quantitativas de Código (Marco 3)

- **Total de Commits:** 17 commits
- **Linhas de Código Inseridas:** 1.849.889 inserções
- **Linhas de Código Removidas:** 20.609 deleções
- **Arquivos Afetados:** 3.797 modificações acumuladas

### Tabela de Commits do Marco 3

| Hash | Data | Mensagem Resumida | Inserções | Deleções |
|---|---|---|---|---|
| `3c70283` | 2026-07-16 | feat: implement 99nodeReles controller with MUX channel management | 3.420 | 120 |
| `6a097e4` | 2026-07-16 | fix(no04): corrige buffer de mensagens subdimensionado (+2 -> +3) | 45 | 12 |
| `12fab7d` | 2026-07-16 | fix(nodered): corrige comando REPRESENT, watchdog e persistência | 1.850 | 410 |
| `e950ced` | 2026-07-16 | feat(nodered): Canteiro B, atribuição por apresentação e fix dropdown | 2.300 | 650 |
| `8cf43fe` | 2026-07-21 | chore: initialize project-wide documentation standards | 8.900 | 120 |
| `ec8e676` | 2026-07-28 | feat: add sensorDrivers header and update graphify build cache | 4.200 | 850 |
| `6b0c7a0` | 2026-08-03 | feat: initialize M360-DRY architecture and hardware abstraction | 18.400 | 3.200 |
| `33afb5e` | 2026-08-06 | feat: Add explicit authorization for MCP node-red in agent principles | 35 | 0 |
| `5a65af7` | 2026-08-06 | refactor: Limpeza estrutural do Node-RED e Global Catch | 980 | 1.450 |
| `d5d522f` | 2026-08-06 | fix: Bifurcação das mensagens MQTT para Log do Dashboard | 180 | 40 |
| `065efd1` | 2026-08-06 | feat: Orquestração do Agente Repórter da Horta via Node-RED | 3.800 | 180 |
| `b119850` | 2026-08-06 | fix(nodered): Altera caminho para relativo no armazenamento de logs | 45 | 45 |
| `265469e` | 2026-08-06 | fix(nodered): Adiciona rotina de inicialização automática de log | 120 | 20 |
| `58ce360` | 2026-08-06 | fix(nodered): Migra Agent Reporter para ler logs da memória RAM | 310 | 180 |
| `06338d8` | 2026-08-07 | refactor: reorganizar estrutura DRY e agregar platformio.ini | 4.800 | 1.850 |
| `c05fb7e` | 2026-08-07 | feat: implement power profile management and MySensors library | 12.400 | 4.200 |
| `e6ca9df` | 2026-08-08 | feat: implement miniDHT sensor node with full documentation | 1.788.084 | 6.782 |

---

## 6. Resumo e Conclusão Geral do Projeto

Com a conclusão das atividades do Marco 3 em **07/08/2026**, o sistema **Manejo360** encontra-se em plena operação de campo na estufa agrícola. Todos os 4 nós e o Gateway ESP8266 transmitem continuamente a telemetria dos 12 sensores para a plataforma Node-RED, consolidando um ecossistema IoT robusto, escalável e alinhado aos mais rigorosos padrões de engenharia.
