# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projeto
Sistema IoT de monitoramento agrícola M360 Horta.  
**Plataforma:** PlatformIO · Arduino/AVR · MySensors RF24 · ESP8266  
**Arquitetura:** Gateway MQTT (`src/DRY/gateway/`) + Nós sensores/atuadores (`src/DRY/nos/`)

---

## Comandos PlatformIO

```bash
# Build de um ambiente específico
pio run -e <env>

# Flash (build + upload)
pio run -e <env> -t upload

# Monitor serial
pio device monitor -e <env>

# Build + upload + monitor em sequência
pio run -e <env> -t upload && pio device monitor -e <env>

# Análise estática da lib M360-DRY (deve ser executado antes de todo commit na lib)
pio check -e check_m360_dry

# Build de todos os envs padrão
pio run
```

### Ambientes disponíveis

| `env`                        | Hardware               | Porta  | Nó  |
|------------------------------|------------------------|--------|-----|
| `d1_mini_libdry`             | ESP8266 D1 Mini        | COM5   | 0 (Gateway) |
| `nano_01nodeSolo3d`          | Arduino Nano (5V)      | —      | 1  |
| `ProMini_04nodeClima`        | Arduino Pro Mini (3.3V/8MHz) | — | 4  |
| `nano_99_reles_nano`         | Arduino Nano (5V)      | COM4   | 99 |
| `nano_13nodeZTS_UmidadeHall` | Arduino Nano (5V)      | —      | 13 |
| `nano_80nodeAqua`            | Arduino Nano (5V)      | —      | 80 |

---

## Bootstrap de Credenciais (Gateway)

Antes de compilar o gateway, copiar e preencher:

```bash
cp include/M360Credentials.h.example include/M360Credentials.h
```

O arquivo `include/M360Credentials.h` define AP SSID/senha, WiFi STA, MQTT server/usuário/senha, UF e CAR. Está no `.gitignore`.

---

## Skill Obrigatória — MySensors Node Coding & Gateway

Ao trabalhar em qualquer arquivo dentro de `src/DRY/` (nós **ou** gateway), aplicar **obrigatoriamente** os padrões definidos em:

- **Referência consolidada (SSoT):** `.agent/skills/bmad-mysensors-node-coding/SKILL.md`

### Resumo das regras críticas para nós legados (`src/DRY/nos/shared/`)

**Nunca escrever manualmente** o que já existe como macro no `node_engine.h`:

| Padrão proibido | Substituição obrigatória |
|---|---|
| Corpo manual de `presentation()` | `NODE_ENGINE_PRESENTATION(name, ver)` |
| 6 declarações de globais repetidas | `NODE_ENGINE_DEFINE_GLOBALS()` |
| Bloco validação/save/send de intervalo | `NODE_ENGINE_HANDLE_INTERVAL(msg)` |
| Loop V_STATUS para atuadores | `NODE_ENGINE_HANDLE_ACTUATORS(msg)` |
| Bloco diagnóstico de rede | `NODE_ENGINE_CHECK_TRANSPORT()` |
| Bloco teste de conectividade | `NODE_ENGINE_TEST_CONNECTIVITY()` |
| Ciclo static battCycle manual | `NODE_ENGINE_PROCESS_BATTERY(N)` |
| Loop `pinMode`/`digitalWrite` | `nodeEngine_setupPins()` |
| Loop `lastValues=NAN` / `nNoUpdates=0` | `nodeEngine_initArrays(lv, nu, cnt)` |
| String `"teste_do_gateway"` hardcoded | `CMD_FORCE_UPDATE` (de `config.h`) |
| `MyMessage messages[N]` fixo | `messages[NODE_ITEMS_COUNT + 2]` |
| Código após `sleep()` | mover `request()`/`wait()` para **antes** |

### Regras críticas do Gateway (`src/DRY/gateway/`)

| Padrão proibido | Regra |
|---|---|
| Tópico MQTT como string literal | Sempre `buildTopicOut(config)` / `buildTopicIn(config)` |
| `EEPROM.put(&config)` direto | `saveConfig()` campo-a-campo com CRC |
| MQTT ou WebServer em `before()` | Exclusivos de `setup()` |
| `setupWiFi()` fora de `before()` | Exclusivo de `before()` |
| `mqttClient.loop()` em modo AP | Checar `WiFi.getMode() == WIFI_AP` primeiro |
| Lógica de infraestrutura em `newGatewayMqtt.cpp` | Módulo dedicado em `ngm/` |

---

## Biblioteca M360-DRY (`lib/M360-DRY/`)

A lib canônica para nós e gateway. **Novos nós usam esta lib**, não os arquivos legados em `src/DRY/nos/shared/`.

### Componentes principais

| Header | Classe/Struct | Responsabilidade |
|---|---|---|
| `<M360.h>` | — | Header centralizador (importa tudo) |
| `M360Node.h` | `M360::M360Node` | Motor de ciclo de vida do nó (AVR) |
| `M360ItemDef` | struct | Definição declarativa de sensor/atuador |
| `M360PowerProfile` | enum | `M360_LOW_POWER` / `M360_ALWAYS_ON` / `M360_PASSIVE` |
| `M360Gateway.h` | `M360::M360Gateway` | Orquestrador do loop do gateway (ESP8266) |
| `M360Translator.h` | `M360::Translator` | Serialização/deserialização JSON ↔ MyMessage |
| `M360Registry.h` | `M360::Registry` | Rastreamento de nós ativos e timeouts |
| `M360Config.h` | `M360::M360DeviceConfig` | Config EEPROM do gateway (offset 521+) |

### Padrão de nó com lib M360-DRY

Todo nó usa dois arquivos:

- **`withLibDRY/noX.cpp`** — **Puramente declarativo.** Define `NODE_ITEMS[]`, buffers estáticos, instancia `M360Node`, implementa os 5 hooks MySensors (`before`, `presentation`, `setup`, `loop`, `receive`). Não contém leitura de hardware.
- **`sensorDrivers.h/cpp`** — **Camada física.** Implementa as leituras brutas (ADC, I2C, Modbus etc.) e é conectada ao motor via callbacks `onRead` / `onWrite`.

Estrutura mínima de um nó:

```cpp
// noX.cpp
#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>
#include "sensorDrivers.h"

static const M360::M360ItemDef NODE_ITEMS[] = {
    { CHILD_ID, M360::M360_SENSOR, S_TEMP, V_TEMP, -1, 1, 3, "Label", false, 0 },
};
static const uint8_t NODE_ITEMS_COUNT = sizeof(NODE_ITEMS) / sizeof(NODE_ITEMS[0]);

static MyMessage messages[NODE_ITEMS_COUNT + 2];  // SEMPRE +2
static float     lastValues[NODE_ITEMS_COUNT];
static uint8_t   nNoUpdates[NODE_ITEMS_COUNT];

static M360::M360Node node(NODE_ITEMS, NODE_ITEMS_COUNT, messages, lastValues,
                           nNoUpdates, M360::M360_LOW_POWER);

namespace M360 { void powerUp() { ... } void powerDown() { ... } }

void before()       { initSensors(); }
void presentation() { node.begin("Nome", "1.0"); }
void setup()        { node.onRead(readNodeItem); node.onWrite(writeNodeItem); }
void loop()         { node.process(); }
void receive(const MyMessage& msg) { node.handleMessage(msg); }
```

### Perfis de energia

| Perfil | Modo | Uso |
|---|---|---|
| `M360_LOW_POWER` | `smartSleep()` — acorda por watchdog | Sensores de campo a bateria (Nós 01, 04, 13) |
| `M360_ALWAYS_ON` | Timer por `millis()`, sem sleep | Atuadores em fonte fixa (Nós 80, 99) |
| `M360_PASSIVE` | `smartSleep()` — lê só sob comando | Sensores pesados (Modbus RS485) |

---

## Arquitetura do Gateway

**Fluxo de inicialização (`before()` → `setup()` → `loop()`):**

1. `before()`: EEPROM init → `loadConfig()` → WiFi STA (config válida) **ou** WiFi AP (A0 em GND ou config inválida)
2. `setup()`: `setupWebServer()` → `setupMQTT()` (apenas modo STA) → `gateway.begin(...)`
3. `loop()`: delega tudo para `M360::M360Gateway::loop()` — WiFi reconnect, MQTT reconnect, `server.handleClient()`, LEDs, heartbeat, node timeouts

**MQTT topics:**
- Upstream (rádio → broker): `m360/{UF}/{CAR}/out` (JSON com `nodeId`, `sensorId`, `command`, `type`, `payload`)
- Downstream (broker → rádio): `m360/{UF}/{CAR}/in`
- Eventos: `m360/{UF}/{CAR}/out/events`

**Modo AP / Factory Reset:**
- A0 em GND durante o boot → modo configuração (SSID: `M360-Config`, portal em `192.168.4.1`)
- Factory reset via web portal ou mantendo A0 em GND por 3 s nos primeiros 30 s

**LEDs de status:** Verde = WiFi+MQTT OK | Amarelo piscando = WiFi OK, sem MQTT | Vermelho piscando = sem WiFi

---

## Estrutura Relevante

```
src/DRY/
├── gateway/
│   ├── ngm/              config_utils, wifi_utils, mqtt_utils, webserver, leds
│   └── withLibDRY/       libDryGatewayMqtt.cpp (usa M360::M360Gateway)
└── nos/
    ├── shared/           Motor legado (node_engine.h macros) — não usar em novos nós
    ├── 01nodeSolo3d/     Solo 3D resistivo — 18 canais via MUX CD74HC4067 (Pro Mini 3.3V, LP)
    ├── 13nodeZTS_UmidadeHall/ Umidade capacitiva ZTS-3002 Modbus RS485 + Hall (Nano, LP)
    ├── 80nodeAqua/       pH, EC, DS18B20, ultrassônico, 4x vazão YF-S201 (Nano, ON)
    └── 99nodeReles/      9 atuadores: 7 via MUX CD74HC4067 + 2 nativos + DHT11 (Nano, ON)

lib/M360-DRY/             Biblioteca canônica (M360Node, M360Gateway, M360Translator…)
include/
    M360Credentials.h     Credenciais locais (gitignored)
    M360Credentials.h.example  Template de credenciais
```

---

## Convenções de Código

- **Linguagem dos comentários e logs Serial:** português
- **IDs reservados:** 254 = Intervalo (`V_VAR1`), 255 = Bateria (`V_VOLTAGE`)
- **EEPROM nós (AVR):** endereços 512–515 via `M360Config` / `nodeEngine_saveInterval()` — nunca `EEPROM.put()` direto
- **EEPROM gateway (ESP):** região 0–511 = MySensors (reservado), 512–520 = M360NodeConfig, 521+ = `DeviceConfig`/`M360DeviceConfig` com CRC
- **Perfil de energia:** definir exatamente um dos dois — nunca ambos simultaneamente
- **Precisão float:** 1 casa decimal (`set(val, 1)`)
- **Solo:** escala 0 (seco) → 100 (água)
- **JSON gateway:** `DynamicJsonDocument(512)` para mensagens, `(384)` para heartbeat, `(256)` para eventos
- **Macros MY_\*:** definidas exclusivamente no `platformio.ini` — nunca no `.cpp` após `#include <MySensors.h>`
- **Pinos virtuais MUX (Nó 99):** `pin = MUX_CHANNEL_OFFSET + canal` (100–115) — `setupPins()` deve ser omitido; pinos gerenciados em `initSensors()`
