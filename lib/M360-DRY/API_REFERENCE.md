# 📖 Referência da API — M360-DRY

Este documento contém a descrição técnica de structs, enums e métodos da biblioteca LibDRY.

---

## 1. Structs e Enums Core

### `M360::M360ItemDef`
Define as propriedades de um sensor ou atuador no nó.

| Campo | Tipo | Descrição |
| :--- | :--- | :--- |
| `childId` | `uint8_t` | ID único do sensor no nó (0-253). |
| `kind` | `M360ItemKind` | `M360_SENSOR` ou `M360_ACTUATOR`. |
| `presentationType` | `uint8_t` | Tipo MySensors (ex: `S_TEMP`, `S_HUM`). |
| `valueType` | `uint8_t` | Tipo de dado MySensors (ex: `V_TEMP`, `V_STATUS`). |
| `pin` | `int` | Pino físico associado (-1 se não usado). |
| `reportIntervalMin` | `uint16_t` | Intervalo mínimo de reporte em minutos (0 = sempre reporta). |
| `readSamples` | `uint8_t` | Amostras para média aritmética antes do envio. |
| `label` | `const __FlashStringHelper*` | Nome exibido no Controller (ex: "Bomba 1"). Fica na **Flash**: declare um array `PROGMEM` e passe-o com `M360_LABEL()` (ver abaixo). |
| `wakeOnRadio` | `bool` | Se `true`, solicita estado ao gateway logo após acordar. |
| `flags` | `uint8_t` | Bit 0: Multiplica valor por 100 antes de enviar (Útil p/ Volts). |

#### Rótulos na Flash (2.0.0+)

```cpp
static const char L_BOMBA[] PROGMEM = "Bomba 1";
static const M360::M360ItemDef NODE_ITEMS[] = {
	{ 31, M360::M360_ACTUATOR, S_BINARY, V_STATUS, 5, 0, 1, M360_LABEL(L_BOMBA), false, 0 },
};
```

- Um literal `"Bomba 1"` direto no campo **não compila** — de propósito: até a 1.x cada rótulo ocupava SRAM.
- `M360_LABEL()` só pode receber um array `PROGMEM`. Aplicado a uma string em RAM, compila e envia lixo como rótulo.

---

## 2. Classe `M360::M360Node`

A peça central que coordena a vida do nó.

### Construtor
```cpp
M360Node(const M360ItemDef* items, uint8_t count, float* lastValues, uint8_t* nNoUpdates, M360PowerProfile profile = M360_LOW_POWER);
```
- **Frequência de Uso:** No início do arquivo do nó antes do `setup()`.
- **Buffers:** `lastValues` e `nNoUpdates` com tamanho `count`. Não há buffer de mensagens: desde a 2.0.0 a lib monta cada envio — inclusive intervalo (254), bateria (255) e debug (253) — num único `MyMessage` interno.

### `M360_PASSIVE` (O Sentinela Reativo)
- **Comportamento:** Acorda no intervalo -> Faz Check-in -> Volta a Dormir.
- **Lógica:** Ele respeita o timer de intervalo para acordar e verificar mensagens via `smartSleep()`, mas **ignora a rotina de leitura automática**. O ciclo de leitura/envio só acontece se o gateway enviar um comando (ex: `V_CUSTOM` ou `V_STATUS`) durante a janela de acordado.
- **Uso:** Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.
- `handleMessage(msg)`: Executa no `receive()`. Processa comandos de intervalo, força atualização e atuadores.
- `onRead(callback)`: Registra sua função de leitura.
- `onWrite(callback)`: Registra sua função de atuação para relés e válvulas.
- `setupPins()`: Configura automaticamente os pinos definidos na struct `M360ItemDef` como INPUT ou OUTPUT.

### Métodos Principais
- `begin(F(name), F(version))`: Registra o nó no MySensors. Nome e versão ficam na Flash — um literal sem `F()` não compila. Adiciona sufixo ao nome (`[LP]`, `[ON]`, `[PAS]`, `[REP]`). Inicializa `lastValues[]` com `NAN` e `nNoUpdates[]` com `0`.
- `process()`: Executa no `loop()`. Cuida de leituras, envios e gerenciamento de sono/millis.
- `handleMessage(msg)`: Executa no `receive()`. Processa: intervalo (`V_VAR1`/`V_VAR5` em childId 254), atuadores (`V_STATUS`), e comandos `V_CUSTOM` (ver tabela abaixo).
- `onRead(callback)`: Registra sua função de leitura.
- `onWrite(callback)`: Registra sua função de atuação para relés e válvulas.
- `setupPins()`: Configura automaticamente os pinos definidos na struct `M360ItemDef` como INPUT ou OUTPUT.
- `getInterval()`: Retorna o intervalo atual em minutos.
- `setInterval(minutes)`: Altera o intervalo de ciclo **sem gravar na EEPROM**. Cancela qualquer override em ms. Ignora valores fora de `[M360_MIN_INTERVAL, M360_MAX_INTERVAL]`. Imprime `INT:<valor>` no Serial. Um `C_SET V_VAR1` recebido via rede tem a mesma semântica, mas persiste na EEPROM.
- `setIntervalMs(ms)`: Define o intervalo em milissegundos brutos (sem validação de faixa, sem gravar EEPROM). Passa `0` para cancelar o override e retomar o intervalo em minutos. Uso principal: **warm-up pós-boot** (ver §4 abaixo).
- `sendDebug(text)`: Envia string arbitrária pelo child `CHILD_ID_DEBUG` (253, `V_TEXT`). Chega ao MQTT/Node-RED via gateway sem precisar de acesso Serial ao nó — canal de debug remoto. Payload truncado em 24 chars (limite `MAX_PAYLOAD_SIZE`).
- `sendItemState(index, state)`: Envia o estado (`bool`) do item `NODE_ITEMS[index]`, com o child e o tipo dele. Para uso do nó fora do fluxo da lib — ex.: o failsafe que desliga um relé sozinho, ou o eco do canal MUX preemptado (Nó 99).

### Comandos `V_CUSTOM` processados por `handleMessage()`

Todos os payloads são strings. Constantes definidas em `M360Constants.h`.

| Constante | Payload | Efeito |
|-----------|---------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | Dispara `_readAndSendAll(true)` imediatamente: lê e **sempre envia** o valor atual de cada sensor, ignorando o filtro de "mudou mais que 0.05" e o contador de 10 ciclos sem atualização; em `M360_PASSIVE` liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | Dispara `_rePresent()`: re-executa `present()` para todos os children sem resetar `lastValues[]`; imprime `REPRES:OK` no Serial |
| `CMD_DEBUG_NET` | `"DEBUG_NET"` | Envia diagnóstico de rede (`parent`/`dist`/`transportReady`) via `sendDebug()` — equivalente remoto de `_printNetDiag()` |
| `CMD_RESET` | `"RESET"` | Não processado pelo motor — disponível para uso customizado no nó |

**Exemplo MQTT para solicitar diagnóstico remoto do nó 13:**
```json
{ "nodeId": 13, "sensorId": 0, "command": 1, "type": 48, "payload": "DEBUG_NET" }
```
A resposta chega em `m360/{UF}/{CAR}/out` com `sensorId: 253` e o payload `"P:0 D:1 R:S"`.

**Exemplo MQTT para acionar reapresentação:**
```json
{ "nodeId": 1, "sensorId": 0, "command": 1, "type": 48, "payload": "REPRESENT" }
```

---

## 3. Gestão de Energia (`M360Power`)

Funções de utilidade para economia de bateria e persistência.

### Funções Globais
- `readBatteryVoltage()`: Lê a voltagem de entrada (VCC) usando a referência interna de 1.1V.
- `readBatteryPercent()`: Converte voltagem em porcentagem (0-100%).
- `loadInterval()`: Recupera o intervalo de reporte da EEPROM (robusto contra resets).
- `saveInterval(uint16_t)`: Salva o novo intervalo vindo do gateway na EEPROM.

### Constantes de Configuração (`M360Config.h`)
- `M360_MIN_VOLTAGE`: Tensão mínima (ex: 2.7V para baterias de lítio).
- `M360_MAX_VOLTAGE`: Tensão máxima (ex: 4.2V).
- `M360_DEFAULT_INTERVAL`: Padrão: 30 minutos.

---

## 4. Padrão Warm-Up Pós-Boot (nós `M360_LOW_POWER`)

Após um reboot (queda de energia, reset manual ou watchdog), o nó demora até 1 intervalo completo para enviar a primeira leitura — o que pode significar 30 min sem dados no MQTT. O padrão **warm-up** resolve isso:

- **Fase 1 (0 – N min):** intervalo curto em ms via `setIntervalMs()` — garante dados rápidos logo após o boot e confirma a reconexão com o gateway.
- **Fase 2 (após N min):** transição automática para o intervalo nominal via `setInterval()`, que cancela o override em ms.

### Comportamento com comandos externos durante o warm-up

| Evento | Resultado |
|---|---|
| Node-RED envia `C_SET V_VAR1` (child 254) durante o warm-up | `handleMessage()` cancela `_intervalMs`, adota e persiste o novo intervalo imediatamente |
| `millis()` atinge o limiar de duração | `setInterval(NOMINAL)` cancela `_intervalMs`, nó entra em regime normal |
| Reboot/reset | `_intervalMs` volta para 0 no constructor; `setup()` do nó precisa rearmá-lo |

> ⚠️ O campo `_intervalMs` **não é persistido na EEPROM** — é sempre zerado no constructor. O nó deve rearmar o warm-up em `setup()` a cada boot.

### Exemplo de implementação (Nó 4 — SolarMini)

```cpp
// Constantes de warm-up (definir no .cpp do nó)
#define WARMUP_INTERVAL_MS   30000UL          // 30 s
#define WARMUP_DURATION_MS   (5UL * 60000UL)  // 5 min
#define NOMINAL_INTERVAL_MIN 30               // 30 min

static bool warmupDone = false;

void setup() {
    node.onRead(readNodeItem);
    node.setIntervalMs(WARMUP_INTERVAL_MS);   // fase 1: dorme 30 s
    Serial.println(F("WARMUP:30s"));
}

void loop() {
    // millis() acumula tempo de uptime real (smartSleep para o clock)
    if (!warmupDone && millis() >= WARMUP_DURATION_MS) {
        warmupDone = true;
        node.setInterval(NOMINAL_INTERVAL_MIN); // fase 2: cancela ms, adota 30 min
        Serial.println(F("WARMUP:done"));
    }
    node.process();
}
```

> **Nota sobre `millis()` e LOW_POWER:** no perfil `M360_LOW_POWER` o `smartSleep()` **para** o `millis()` durante o sono — o tempo dorme não conta. A janela de 5 min é medida em **tempo de uptime cumulativo** (soma dos períodos acordado). Com WARMUP de 30 s, ~10 ciclos acordados completam os 5 min.

---
*Retornar ao [README.md](README.md)*
