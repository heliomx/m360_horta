# Adversarial Code Review: nodeSelenoieVazao

## 🎯 Escopo da Revisão
- **Componente:** `nodeSelenoieVazao` (Válvulas e Fluxo)
- **Tecnologia:** MySensors, Arduino Pro Mini (AVR 8-bit)
- **Arquitetura:** DRY Node Engine

---

## 🚩 Findings Summary (Triage)

| ID | Severidade | Categoria | Descrição | Status |
| :--- | :--- | :--- | :--- | :--- |
| P-01 | **CRITICAL** | Race Condition | Leitura não-atômica de `pulseCount` (32-bit) em MCU 8-bit. | ✅ Fixed |
| P-02 | **HIGH** | Fragility | Acoplamento temporal no cálculo de `flowRate` (Assume ordem de leitura 0 -> 1). | ✅ Fixed |
| P-03 | **MEDIUM** | UX/Reliability | Monitoramento de vazão apenas por intervalo fixo (atraso na detecção de vazamentos). | ✅ Fixed |
| P-04 | **LOW** | Efficiency | Uso de `String()` no `receive` para comando "RESET" (Risco de Heap Fragmentation). | ✅ Fixed |
| P-05 | **LOW** | Consistency | `NODE_ITEMS` e `lastValues` desalinhados em tamanho/lógica de loop. | ✅ Fixed |

---

## 🔍 Detailed Analysis

### [P-01] Race Condition: Corrupção de Pulsos (Critical)
**Local:** `sensorDrivers.cpp:109`, `sensorDrivers.cpp:128`
**Descrição:** O `pulseCount` é um `uint32_t` (4 bytes). No ATmega328P, a leitura desse valor leva 4 ciclos de instrução. Se uma interrupção ocorrer no meio dessa leitura, o valor resultante será corrompido.
**Impacto:** Leituras de vazão e volume com picos erráticos ou valores impossíveis.

### [P-02] Acoplamento Temporal: Cálculo de Vazão (High)
**Local:** `sensorDrivers.cpp:115-122`
**Descrição:** O driver atualiza `oldTime` e `oldPulseCount` apenas quando `sensorIndex == 1`.
**Risco:** Se o motor DRY decidir ler apenas o Sensor 1 (via comando do Gateway) ou se a ordem dos itens em `NODE_ITEMS` mudar, o cálculo do Sensor 2 usará um Delta de tempo acumulado errado.
**Correção:** Cada sensor deve ter seu próprio `lastReadTime` e `lastPulseCount`.

### [P-03] Gap de Monitoramento em Tempo Real (Medium)
**Local:** `noSelenoideVazao.cpp:240`
**Descrição:** A vazão é lida e enviada apenas via `updateInterval` (que pode ser de 10-60 min).
**Risco:** Em sistemas de irrigação, reportar vazão a cada 10 minutos é insuficiente para detectar vazamentos em tempo hábil. Nó de vazão deve ter um "Fast Sampling" ou reportar imediatamente se `flowRate > 0`.

### [P-04] Fragmentação de Heap (Low)
**Local:** `noSelenoideVazao.cpp:279`
**Descrição:** Uso de `String(message.getString()) == "RESET"`.
**Correção:** Usar `strcmp(message.getString(), "RESET") == 0`.

---

## 🛠️ Proposed Patches (Implementation Plan)

### Patch 01: Atomicity Helper
Envolver as leituras de pulsos em blocos `noInterrupts()` / `interrupts()`.

### Patch 02: De-coupled Flow Calculation
Substituir as globais compartilhadas por arrays mapeados ao index do sensor.

### Patch 03: Fast Reporting on Flow
Adicionar lógica no `loop` para reportar vazão a cada 2-5 segundos se o valor for superior a zero, ignorando o `updateInterval` longo.

---

## ❓ Questões para Marcelo
1. Este nó é alimentado por **Bateria** ou **Fonte Fixa (12V)**? 
   > **Resposta:** Fonte Fixa (12V).
2. Deseja aplicar os patches automaticamente (Batch-Apply)?
   > **Resposta:** Sim, implementado em V1.2.
