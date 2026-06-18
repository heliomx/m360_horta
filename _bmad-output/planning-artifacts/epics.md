---
stepsCompleted: ["step-01-validate-prerequisites", "step-02-design-epics"]
inputDocuments: ["_bmad-output/planning-artifacts/prd.md", "src/DRY/nos/nodeUmidadeTemperatura/diagrama", "src/DRY/nos/shared/node_engine.h"]
---

# m360_horta - Epic Breakdown

## Overview
Este documento desdobra os requisitos do PRD para o nó de monitoramento de horta em epics e stories implementáveis.

## Requirements Inventory

### Functional Requirements
FR1: Ler Temperatura e Umidade do Ar via DHT11 (Pino D2).
FR2: Ler Temperatura de Solo/Líquido via DS18B20 (Pino D3).
FR3: Ler Umidade de Solo 1 (Pino A0) e Solo 2 (Pino A1).
FR4: Integrar mensagens ao node_engine (V_TEMP, V_HUM).
FR5: Reportar nível de bateria periodicamente.

### NonFunctional Requirements
NFR1: Modularidade: O código deve estar separado em sensorDrivers e lógica de nó.
NFR2: Eficiência: Uso obrigatório de Deep Sleep entre leituras.
NFR3: Portabilidade: Seguir estritamente as definições de config.h e node_engine.h.

### FR Coverage Map
| Epic | Stories | Requisitos Cobertos |
| :--- | :--- | :--- |
| Epic 1: Drivers | 1.1, 1.2, 1.3 | FR1, FR2, FR3, NFR1 |
| Epic 2: Node Logic | 2.1, 2.2 | FR4, FR5, NFR2, NFR3 |

## Epic List

1. **Epic 1: Drivers de Sensores**: Implementação da camada de abstração de hardware (`sensorDrivers.h/cpp`).
2. **Epic 2: Lógica do Nó e Integração**: Montagem do arquivo `nodeUmidadeTemperatura.cpp` integrado ao motor DRY.

---

## Epic 1: Drivers de Sensores
O objetivo é garantir leituras limpas e isoladas do hardware.

### Story 1.1: Driver DHT11 e Solo Analógico
As a Developer,
I want to implement the reading logic for DHT11 and Moisture sensors,
So that I can get air and soil data reliably.

**Acceptance Criteria:**
- Given Pino D2 high, When readDHT is called, Then return Temp and Humidity.
- Given Pino A0/A1, When readSoil is called, Then return raw analog values.

### Story 1.2: Driver DS18B20 (OneWire)
As a Developer,
I want to implement the DS18B20 driver,
So that I can monitor liquid/ground temperature via OneWire bus.

---

## Epic 2: Lógica do Nó e Integração
O objetivo é rodar o ciclo MySensors + Deep Sleep.

### Story 2.1: Implementação nodeUmidadeTemperatura.cpp
As a System,
I want to coordinate all sensors and send data to the gateway,
So that the dashboard receives updated info.

### Story 2.2: Gestão de Energia e Bateria
As a System,
I want to enter Deep Sleep between cycles,
So that the battery lasts for months.
