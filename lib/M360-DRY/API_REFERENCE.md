# 📖 Referência da API — M360-DRY

Este documento contém a descrição técnica de structs, enums e métodos da biblioteca LibDRY.

---

## 1. Structs e Enums Core

### `M360::M360ItemDef`
Define as propriedades de um sensor ou atuador no nó.

| Campo | Tipo | Descrição |
| :--- | :--- | :--- |
| `childId` | `uint8_t` | ID único do sensor no nó (1–252). O 0 é `CHILD_ID_STATUS` e 253–255 são reservados (debug, intervalo, bateria). Use as faixas normativas de `M360Constants.h`: solo 1–10, clima 11–20, hidrometria 21–30, atuação 31–40. |
| `kind` | `M360ItemKind` | `M360_SENSOR` ou `M360_ACTUATOR`. |
| `presentationType` | `uint8_t` | Tipo MySensors (ex: `S_TEMP`, `S_HUM`). |
| `valueType` | `uint8_t` | Tipo de dado MySensors (ex: `V_TEMP`, `V_STATUS`). |
| `pin` | `int` | Pino físico associado (-1 se não usado). |
| `reportIntervalMin` | `uint16_t` | **Não lido pela lib** (campo reservado). O filtro de envio é fixo: o valor sai quando muda mais de 0,05 ou após 10 ciclos sem mudança. Preencher com `0`. |
| `readSamples` | `uint8_t` | **Não lido pela lib** (campo reservado): o callback de leitura é chamado uma vez por item e por ciclo. Se precisar de média, faça-a no driver. Preencher com `1`. |
| `label` | `const __FlashStringHelper*` | Nome exibido no Controller (ex: "Bomba 1"). Fica na **Flash**: declare um array `PROGMEM` e passe-o com `M360_LABEL()` (ver abaixo). |
| `wakeOnRadio` | `bool` | Só para `M360_ACTUATOR` no perfil `M360_LOW_POWER`: depois da leitura e antes de dormir, faz `request()` do estado ao controlador. Ignorado nos demais casos. |
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

### `M360_REPEATER`
- Igual ao `M360_ALWAYS_ON` (timer por `millis()`, nunca dorme), mas exige `-D MY_REPEATER_FEATURE` no env: o MySensors encaminha mensagens de nós fora do alcance do gateway. Sufixo `[REP]`. Usado pelo Nó 99 nos envs `_rep`.

### Métodos Principais
- `begin(F(name), F(version))`: Registra o nó no MySensors. Nome e versão ficam na Flash — um literal sem `F()` não compila. Adiciona sufixo ao nome (`[LP]`, `[ON]`, `[PAS]`, `[REP]`). Inicializa `lastValues[]` com `NAN` e `nNoUpdates[]` com `0`.
- `process()`: Executa no `loop()`. Cuida de leituras, envios e gerenciamento de sono/millis.
- `handleMessage(msg)`: Executa no `receive()`. Processa: intervalo (`V_VAR1`/`V_VAR5` em childId 254), atuadores (`V_STATUS`), e comandos `V_CUSTOM` (ver tabela abaixo).
- `onRead(callback)`: Registra sua função de leitura.
- `onWrite(callback)`: Registra sua função de atuação para relés e válvulas.
- `setupPins()`: Configura automaticamente os pinos definidos na struct `M360ItemDef` como INPUT ou OUTPUT.
- `getInterval()`: Retorna o intervalo atual em minutos.
- `setInterval(minutes)`: Altera o intervalo em minutos **sem gravar na EEPROM**, cancela qualquer override em ms e ecoa o valor vigente no child 254. Devolve `false` (log `INT_LOC:REJ:<valor>`) fora de `[M360_MIN_INTERVAL, M360_MAX_INTERVAL]`. Log `INT_LOC:<valor>`; o caminho da rede (`C_SET V_VAR1` em `handleMessage()`) imprime `INT:<valor>` e persiste. Só tem efeito depois de `begin()`, que carrega o valor da EEPROM.
- `setIntervalMs(ms)`: Override temporário do intervalo em milissegundos (sem gravar EEPROM), lido por `process()` nos perfis `M360_LOW_POWER` e `M360_PASSIVE`. `0` cancela o override e ecoa o intervalo em minutos no child 254. Devolve `false` nos perfis `M360_ALWAYS_ON`/`M360_REPEATER` (`INT_MS:IGN`) e fora de `[M360_MIN_INTERVAL_MS, M360_MAX_INTERVAL × 60 000]` (`INT_MS:REJ`). Uso principal: **warm-up pós-boot** (ver §4 abaixo).
- `getIntervalMs()`: Override em ms vigente (`0` = inativo).
- `sendDebug(text)`: Envia string arbitrária pelo child `CHILD_ID_DEBUG` (253, `V_TEXT`). Chega ao MQTT/Node-RED via gateway sem precisar de acesso Serial ao nó — canal de debug remoto. Payload truncado em 24 chars (limite `MAX_PAYLOAD_SIZE`).
- `sendItemState(index, state)`: Envia o estado (`bool`) do item `NODE_ITEMS[index]`, com o child e o tipo dele. Para uso do nó fora do fluxo da lib — ex.: o failsafe que desliga um relé sozinho, ou o eco do canal MUX preemptado (Nó 99).

### Comandos `V_CUSTOM` processados por `handleMessage()`

Todos os payloads são strings. Constantes definidas em `M360Constants.h`.

| Constante | Payload | Efeito |
|-----------|---------|--------|
| `CMD_FORCE_UPDATE` | `"FORCE_UPDATE"` | Dispara `_readAndSendAll(true)` imediatamente: lê e **sempre envia** o valor atual de cada sensor, ignorando o filtro de "mudou mais que 0.05" e o contador de 10 ciclos sem atualização; em `M360_PASSIVE` liga/desliga periféricos |
| `CMD_REPRESENT` | `"REPRESENT"` | Dispara `_rePresent()`: re-executa `present()` para todos os children sem resetar `lastValues[]`; imprime `REPRES:OK` no Serial |
| `CMD_DEBUG_NET` | `"DEBUG_NET"` | Envia diagnóstico de rede (`parent`/`dist`/`transportReady`) via `sendDebug()` — equivalente remoto de `_printNetDiag()` |

Os gateways compilam com `M360_NATIVE_MQTT=1`: os campos vão no **tópico** (`in/<nodeId>/<childId>/<command>/<ack>/<type>`) e o payload é a string bruta. Um envelope JSON publicado no tópico raiz `.../in` é rejeitado (`reportCommandRejected`).

**Diagnóstico remoto do Nó 4** (`type 48` = `V_CUSTOM`):
```
tópico:  m360/{UF}/{CAR}/in/4/0/1/0/48
payload: DEBUG_NET
```
A resposta chega em `m360/{UF}/{CAR}/out/4/253/1/0/47` (`V_TEXT`) com payload como `P:0 D:1 R:S`.

**Reapresentação do Nó 1:** mesmo formato, tópico `m360/{UF}/{CAR}/in/1/0/1/0/48`, payload `REPRESENT`.

---

## 3. Gestão de Energia (`M360Power`)

Funções de utilidade para economia de bateria e persistência.

### Funções Globais
- `readBatteryVoltage()`: Símbolo **fraco** (`__attribute__((weak))`). O padrão mede o VCC contra o bandgap de 1,1 V, o que só é a tensão da bateria quando ela alimenta o MCU direto (Nó 4). O nó pode sobrescrever dentro do `namespace M360` — a M360-LP o faz em `src/DRY/horta/nos/lp/drivers/battery.cpp`, lendo o A6 corrigido pelo bandgap.
- `readBatteryPercent()`: Converte voltagem em porcentagem (0-100%).
- `loadInterval()`: Recupera o intervalo de reporte da EEPROM (robusto contra resets).
- `saveInterval(uint16_t)`: Salva o novo intervalo vindo do gateway na EEPROM.

### Constantes de Configuração (`M360Config.h`)
- `M360_MIN_VOLTAGE`: Tensão mínima, 0 % (padrão **3,0 V**).
- `M360_MAX_VOLTAGE`: Tensão máxima, 100 % (padrão **4,2 V**).
- `M360_DEFAULT_INTERVAL`: Padrão: **1 minuto** (`M360Config.h`). O env do nó pode redefinir com `-D M360_DEFAULT_INTERVAL=<min>`; só vale com EEPROM virgem ou inválida.
- `M360_MIN_INTERVAL_MS`: Piso do override de `setIntervalMs()`. Padrão: 5000 ms.

---

## 4. Padrão Warm-Up Pós-Boot (nós `M360_LOW_POWER`)

Após um reboot (queda de energia, reset manual ou watchdog), o nó demora até 1 intervalo completo para enviar a primeira leitura — com intervalo de 30 min, são 30 min sem dados no MQTT. O padrão **warm-up** resolve isso:

- **Fase 1 (N ciclos):** intervalo curto em ms via `setIntervalMs()` — dados rápidos logo após o boot e confirmação da reconexão com o gateway.
- **Fase 2 (após N ciclos):** `setIntervalMs(0)` encerra o override; o nó volta ao intervalo em minutos que `begin()` carregou da EEPROM (child 254). A lib ecoa o intervalo vigente no child 254 nessa transição.

### Por que contar ciclos, e não `millis()`

- `smartSleep()` **para** o `millis()` durante o sono: o HAL AVR do MySensors dorme por watchdog e não ajusta o contador. Uma janela de "5 min de `millis()`" mede só tempo acordado (~5 s por ciclo) e levaria ~60 ciclos — ~35 min de relógio transmitindo a cada 30 s.
- O `millis()` **corre** enquanto `_begin()` espera o gateway (`MY_TRANSPORT_WAIT_READY_MS` = 0 espera indefinidamente). Numa queda geral de energia em que o gateway demore mais que a janela, um warm-up por tempo já nasceria expirado em `setup()`.

No perfil `M360_LOW_POWER`, cada chamada de `process()` é exatamente um ciclo (lê → envia → dorme), então basta contar chamadas.

### Regras

| Situação | Comportamento |
|---|---|
| `C_SET V_VAR1` (child 254) chega durante o warm-up | `handleMessage()` zera `_intervalMs`, adota e persiste o novo intervalo; o `setIntervalMs(0)` posterior é inócuo |
| Warm-up termina | `setIntervalMs(0)` → intervalo da EEPROM; eco no child 254 |
| Reboot | `_intervalMs` volta a 0 no constructor; `setup()` rearma |
| Perfil `M360_ALWAYS_ON` / `M360_REPEATER` | `setIntervalMs()` **rejeita** (`INT_MS:IGN`): esses perfis usam o timer por `millis()` e enviam já no primeiro `process()` — não precisam de warm-up |
| `ms` fora de `[M360_MIN_INTERVAL_MS, M360_MAX_INTERVAL × 60 000]` | Rejeitado (`INT_MS:REJ`) — abaixo de 500 ms o MySensors nem dorme e o nó ficaria em loop acordado com o rádio ligado |

> O intervalo nominal **não** é uma constante do nó: é o valor da EEPROM. O padrão de fábrica vem de `M360_DEFAULT_INTERVAL` (1 min na lib; o env pode redefinir com `-D M360_DEFAULT_INTERVAL=30`) e só vale com EEPROM virgem ou inválida. Nunca chame `setInterval(<constante>)` na transição: isso descarta o intervalo que o operador persistiu.

### Exemplo de implementação (Nó 4 — SolarMini)

```cpp
#define WARMUP_INTERVAL_MS 30000UL   // 30 s entre ciclos
#define WARMUP_CYCLES      10        // 10 ciclos × 30 s ≈ 5 min de relógio

static uint8_t warmupCycles = 0;
static bool    warmupDone   = false;

void setup() {
    node.onRead(readNodeItem);
    if (node.setIntervalMs(WARMUP_INTERVAL_MS)) {
        Serial.println(F("WARMUP:30s"));
    }
}

void loop() {
    node.process();   // LOW_POWER: uma chamada = um ciclo

    if (!warmupDone && ++warmupCycles >= WARMUP_CYCLES) {
        warmupDone = true;
        if (node.getIntervalMs() > 0) {
            node.setIntervalMs(0);                      // volta ao intervalo da EEPROM
            Serial.println(F("WARMUP:done->eeprom"));
        } else {
            Serial.println(F("WARMUP:done->backend"));  // C_SET V_VAR1 já cancelou
        }
    }
}
```

**Custo:** 10 ciclos extras de ~5 s acordado por boot (~0,3 mAh no Nó 4) — ver `balanco_energetico.md` do nó.

---
*Retornar ao [README.md](README.md)*
