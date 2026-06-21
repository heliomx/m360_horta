# M360 Horta — Contexto Permanente para Codex

## Projeto
Sistema IoT de monitoramento agrícola M360 Horta.  
**Plataforma:** PlatformIO · Arduino/AVR · MySensors RF24 · ESP8266  
**Arquitetura:** Gateway MQTT (`src/DRY/gateway/`) + Nós sensores/atuadores (`src/DRY/nos/`)

---

## Skill Obrigatória — MySensors Node Coding & Gateway

Ao trabalhar em qualquer arquivo dentro de `src/DRY/` (nós **ou** gateway), aplicar **obrigatoriamente** os padrões definidos em:

- **Referência consolidada (SSoT):** `.agent/skills/bmad-mysensors-node-coding/SKILL.md`

### Resumo das regras críticas

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

**Antes de criar qualquer novo nó**, ler o template em `SKILLmySensors.md` seção 3.

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

## Estrutura Relevante

```
src/DRY/
├── gateway/              ESP8266 + MQTT bridge
│   ├── newGatewayMqtt.cpp
│   └── ngm/              wifi_utils, mqtt_utils, webserver, leds, config_utils
└── nos/
    ├── shared/           Motor DRY — node_engine, config, powerProfile
    ├── nodeDHT11/        Sensor temperatura/umidade (DHT11 mock)
    ├── nodePump/         Atuador bomba
    ├── nodeUmidadeTemperatura/  Multi-sensor (DHT11 + DS18B20 + solo)
    ├── nodeSelenoieVazao/       Válvulas + medidores de vazão YF-S201
    └── nodeZTS_UmidadeHall/     Solo Modbus RS485 (ZTS-3002) + Hall
```

---

## Convenções de Código

- **Linguagem dos comentários e logs Serial:** português
- **IDs reservados:** 254 = Intervalo (`V_VAR1`), 255 = Bateria (`V_VOLTAGE`)
- **EEPROM nós:** sempre via `nodeEngine_saveInterval()`, nunca `EEPROM.put()` direto
- **EEPROM gateway:** região 0–511 = MySensors, região 512+ = `DeviceConfig` com CRC
- **Perfil de energia:** definir `POWER_PROFILE_LOW_POWER` **ou** `POWER_PROFILE_ALWAYS_ON` — nunca os dois
- **Precisão float:** 1 casa decimal (`set(val, 1)`)
- **Solo:** escala 0 (seco) → 100 (água)
- **JSON gateway:** `DynamicJsonDocument(512)` para mensagens, `(384)` para heartbeat, `(256)` para eventos
