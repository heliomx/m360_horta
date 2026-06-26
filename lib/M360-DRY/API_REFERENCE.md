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
| `label` | `const char*` | Nome exibido no Controller (ex: "Bomba 1"). |
| `wakeOnRadio` | `bool` | Se `true`, solicita estado ao gateway logo após acordar. |
| `flags` | `uint8_t` | Bit 0: Multiplica valor por 100 antes de enviar (Útil p/ Volts). |

---

## 2. Classe `M360::M360Node`

A peça central que coordena a vida do nó.

### Construtor
```cpp
M360Node(const M360ItemDef* items, uint8_t count, MyMessage* messages, float* lastValues, uint8_t* nNoUpdates, M360PowerProfile profile = M360_LOW_POWER);
```
- **Frequência de Uso:** No início do arquivo do nó antes do `setup()`.
- **Dica:** O buffer `messages` deve ter tamanho `count + 2` para acomodar baterias e intervalo.

### `M360_PASSIVE` (O Sentinela Reativo)
- **Comportamento:** Acorda no intervalo -> Faz Check-in -> Volta a Dormir.
- **Lógica:** Ele respeita o timer de intervalo para acordar e verificar mensagens via `smartSleep()`, mas **ignora a rotina de leitura automática**. O ciclo de leitura/envio só acontece se o gateway enviar um comando (ex: `V_CUSTOM` ou `V_STATUS`) durante a janela de acordado.
- **Uso:** Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.
- `handleMessage(msg)`: Executa no `receive()`. Processa comandos de intervalo, força atualização e atuadores.
- `onRead(callback)`: Registra sua função de leitura.
- `onWrite(callback)`: Registra sua função de atuação para relés e válvulas.
- `setupPins()`: Configura automaticamente os pinos definidos na struct `M360ItemDef` como INPUT ou OUTPUT.

### Métodos Principais
- `begin(name, version)`: Registra o nó no MySensors. Adiciona sufixo ao nome (`[LP]`, `[ON]`, `[PAS]`). Inicializa `lastValues[]` com `NAN` e `nNoUpdates[]` com `0`.
- `process()`: Executa no `loop()`. Cuida de leituras, envios e gerenciamento de sono/millis.
- `handleMessage(msg)`: Executa no `receive()`. Processa: intervalo (`V_VAR1`/`V_VAR5` em childId 254), atuadores (`V_STATUS`), e comandos `V_CUSTOM` (ver tabela abaixo).
- `onRead(callback)`: Registra sua função de leitura.
- `onWrite(callback)`: Registra sua função de atuação para relés e válvulas.
- `setupPins()`: Configura automaticamente os pinos definidos na struct `M360ItemDef` como INPUT ou OUTPUT.
- `getInterval()`: Retorna o intervalo atual em minutos.

### Comandos `V_CUSTOM` processados por `handleMessage()`

Todos os payloads são strings. Constantes definidas em `M360Constants.h`.

| Constante | Payload | Efeito |
|-----------|---------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | Dispara `_readAndSendAll()` imediatamente; em `M360_PASSIVE` liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | Dispara `_rePresent()`: re-executa `present()` para todos os children sem resetar `lastValues[]`; imprime `REPRES:OK` no Serial |
| `CMD_RESET` | `"RESET"` | Não processado pelo motor — disponível para uso customizado no nó |

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
*Retornar ao [README.md](README.md)*
