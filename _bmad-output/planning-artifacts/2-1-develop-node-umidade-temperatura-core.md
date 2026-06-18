# Story 2.1: Implementação nodeUmidadeTemperatura.cpp

As a System,
I want to coordinate DHT11, DS18B20 and Soil sensors with the Node Engine,
So that all environmental data is collected and sent to the MySensors Gateway.

**Acceptance Criteria:**

**Given** the hardware described in the diagram (D2, D3, A0, A1)
**When** the node starts
**Then** it must present 5 child IDs to the gateway (Air Temp, Air Hum, Soil 1, Soil 2, Soil Temp)
**And** it must execute a reliable reading cycle for each sensor
**And** it must use the `NodeItemDef` structure of the `node_engine`.

## Tasks/Subtasks
- [x] Implementar `sensorDrivers.h` com assinaturas das leituras.
- [x] Implementar `sensorDrivers.cpp` com lógica específica (DHT, OneWire, Analog).
- [x] Criar `nodeUmidadeTemperatura.cpp` configurando o array `NODE_ITEMS`.
- [x] Testar integração com MySensors (V_TEMP e V_HUM).

### Review Findings
- [x] [Review][Patch] Contradição de Hardware (Risco de Corrosão): Diagrama mostra VCC_3V3 enquanto código e README usam D4. [diagramaNodeUmidadeTemperatura.md]
- [x] [Review][Patch] Desperdício Crítico de Bateria (3s Wake Window): Nó fica acordado por 3000ms desnecessariamente. [nodeUmidadeTemperatura.cpp:314]
- [x] [Review][Patch] Violação de DRY (Lógica Duplicada): loop() e envio reimplementados manualmente em vez de usar node_engine. [nodeUmidadeTemperatura.cpp:273]
- [x] [Review][Patch] Leitura de Temperatura de Solo Não-Determinística: sensors.getTempCByIndex(0) assume apenas um sensor. [sensorDrivers.cpp:119]
- [x] [Review][Patch] Risco de Fragmentação de Heap (String no receive): Uso de String() para comparação de comandos. [nodeUmidadeTemperatura.cpp:333]
- [x] [Review][Patch] Bloqueio de Rádio por Delay Estabilização (1.2s): delay(1200) bloqueia interrupts de rádio. [sensorDrivers.cpp:155]
- [x] [Review][Patch] Rastro Frágil de Valores Anteriores: lastValues[5] depende da ordem de NODE_ITEMS. [nodeUmidadeTemperatura.cpp:223]
- [x] [Review][Defer] Calibração de Solo Linear em Sensor Não-Linear [sensorDrivers.cpp:132] — deferred, pre-existing

## Dev Agent Record
### Implementation Plan
O plano foi executado com sucesso: a camada de drivers abstrai o hardware e a lógica principal do nó orquestra as leituras e o envio via Node Engine.

### Debug Log
- [OK] DHT11 no pino 2
- [OK] OneWire no pino 3
- [OK] Analógicos em A0/A1

### Completion Notes
Arquivos criados na pasta `src/DRY/nos/nodeUmidadeTemperatura`:
1. `sensorDrivers.h`: Definição de pins e assinaturas.
2. `sensorDrivers.cpp`: Implementação Real (DHT, DallasTemp).
3. `nodeUmidadeTemperatura.cpp`: Integração completa com o motor DRY.

## File List
- `src/DRY/nos/nodeUmidadeTemperatura/sensorDrivers.h`
- `src/DRY/nos/nodeUmidadeTemperatura/sensorDrivers.cpp`
- `src/DRY/nos/nodeUmidadeTemperatura/nodeUmidadeTemperatura.cpp`

## Change Log
- Criada estrutura de arquivos do Nó de Horta.
- Implementada lógica de sensores do Ar e Solo.

## Status: done
