---
stepsCompleted: ["step-01-init", "step-02-discovery", "step-02b-vision", "step-02c-executive-summary", "step-03-success", "step-04-journeys", "step-05-requirements", "step-11-review", "step-12-complete"]
inputDocuments: ["docs/mysensors.pdf", "src/DRY/nos/nodeUmidadeTemperatura/diagrama", "src/DRY/nos/shared/node_engine.h"]
workflowType: 'prd'
classification:
  projectType: iot_embedded
  domain: agrotech
  complexity: medium
  projectContext: brownfield
---

# Product Requirements Document - m360_horta

**Author:** Marcelo
**Date:** 2026-04-01

## 1. Sumário Executivo

O projeto **m360_horta** foca no monitoramento ambiental e de solo de alta fidelidade, utilizando o ecossistema Arduino e o protocolo MySensors. O objetivo central é fornecer dados contínuos e padronizados sobre as condições de cultivo, permitindo uma gestão baseada em dados reais para otimizar a saúde das plantas.

### O Que Torna Este Projeto Especial
O diferencial é a arquitetura **DRY**, ancorada na biblioteca `lib/M360-DRY` e em `M360Node`. Isso permite a geração acelerada de nós complexos com padrões rigorosos de comunicação e baixo consumo.

## 2. Critérios de Sucesso

### Sucesso do Usuário
- O usuário visualiza dados em tempo real de temperatura (ar/solo) e umidade (ar/solo x2).
- Os sensores de solo operam sem corrosão prematura (via gestão de energia por código).

### Sucesso Técnico
- O nó consome corrente mínima em sleep mode conforme o perfil `M360_LOW_POWER`.
- Zero colisões de rádio no envio de mensagens do MySensors.

## 3. Requisitos Funcionais

| ID | Descrição | Prioridade |
| :--- | :--- | :--- |
| RF01 | Ler Temperatura e Umidade do Ar via DHT11 (Pino D2). | P1 |
| RF02 | Ler Temperatura de Solo/Líquido via DS18B20 (Pino D3). | P1 |
| RF03 | Ler Umidade de Solo 1 (Pino A0) e Solo 2 (Pino A1). | P1 |
| RF04 | Integrar mensagens ao `M360Node` (V_TEMP, V_HUM). | P1 |
| RF05 | Reportar nível de bateria periodicamente. | P2 |

## 4. Especificações de Hardware (Diagrama)

- **MCU:** Arduino Pro Mini 3.3V
- **Rádio:** NRF24L01 (CE:9, CSN:10)
- **DHT11:** D2
- **DS18B20:** D3 (One-Wire)
- **Soil Moisture 1:** A0
- **Soil Moisture 2:** A1

## 5. Requisitos Não-Funcionais

- **Modularidade:** O código deve estar separado em `sensorDrivers` e lógica de nó.
- **Eficiência:** Uso obrigatório de `Deep Sleep` entre leituras.
- **Portabilidade:** Seguir os contratos de `M360Config.h`, `M360Constants.h` e `M360Node`.

---
*PRD Finalizado e pronto para codificação.*
