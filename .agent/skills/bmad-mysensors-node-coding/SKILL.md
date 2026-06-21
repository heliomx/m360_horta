---
name: bmad-mysensors-node-coding
description: Padrões obrigatórios de codificação DRY para nós MySensors e gateway MQTT no projeto M360 Horta
---

# MySensors Node Coding Standards — M360 Horta

> [!IMPORTANT]
> **Single Source of Truth (SSoT):** Este arquivo é a ÚNICA referência autoritativa. Padronizado com base no código real em `src/DRY/`.

---

## 1. Arquitetura e Motor DRY

Toda lógica presente em dois ou mais nós pertence à biblioteca `lib/M360-DRY`.
O diretório `shared/` existe apenas para manutenção dos nós legados.

### Estrutura de Diretórios
```
src/DRY/
├── nos/
│   ├── shared/              ← Legado: node_engine, config, powerProfile
│   └── nomeDoNo/            ← Camada 2: sensorDrivers.h/cpp + noDesejado.cpp
└── gateway/                 ← ESP8266 + ngm/ (módulos de infraestrutura)
```

---

## 2. Padrões de Implementação (Nós)

### 2.1 Buffers dos nós M360-DRY
```cpp
uint16_t updateInterval = DEFAULT_INTERVAL;
float    lastValues[NODE_ITEMS_COUNT]; 
uint8_t  nNoUpdates[NODE_ITEMS_COUNT]; 
MyMessage messages[NODE_ITEMS_COUNT + 2]; // intervalo + bateria
```

### 2.2 Ciclo de Vida (Setup)
Sempre inicializar nesta ordem:
1. `nodeEngine_setupPins()`: Configura pinagem de `NODE_ITEMS`.
2. `initSensors()`: Driver específico do nó.
3. `updateInterval = nodeEngine_loadInterval()`: Recupera da EEPROM.
4. `nodeEngine_initArrays(...)`: Zera `lastValues` e `nNoUpdates`.

### 2.3 Loop de Reporte (Regra de Mudança Significativa)
```cpp
if (abs(val - lastValues[i]) > 0.5 || nNoUpdates[i] >= 10) {
    lastValues[i] = val;
    nNoUpdates[i] = 0;
    send(messages[i].set(val, 1));
} else {
    nNoUpdates[i]++;
}
```

---

## 3. Comandos e Eventos Customizados (V_CUSTOM / V_VAR1)

O código implementa padrões específicos para comunicação avançada:

- **FORCE_UPDATE (V_CUSTOM):** O gateway força a leitura com `CMD_FORCE_UPDATE` (`"FORCE_UPDATE"`).
  - Nó deve setar `nNoUpdates[i] = 255` para todos os sensores.
- **RESET (V_VAR1):** Usado em nós de vazão para zerar acumuladores.
  - Implementar via `strcmp(message.getString(), "RESET") == 0`.
- **Intervalo (CHILD_ID_INTERVAL = 254):** Sempre tratar via `NODE_ENGINE_HANDLE_INTERVAL(message)`.

---

## 4. Gateway — Padrões do `newGatewayMqtt.cpp`

- **Node Tracking:** O gateway rastreia nós ativos via `updateNodeStatus()`.
- **JSON Envelope:** Mensagens do rádio para MQTT seguem o formato:
  ```json
  { "nodeId": X, "sensorId": Y, "command": Z, "type": W, "payload": "...", "timestamp": T }
  ```
- **Events:** Eventos de transporte (reconexão, nó perdido) são publicados em `{topic}/events`.
- **Config:** A configuração do gateway (WiFi/MQTT) fica no setor 512+ da EEPROM via `saveConfig()`.

---

## 5. Checklist Anti-Padrões (Verificar antes de Commit)

- [ ] ❌ **Definir MY_NODE_ID após MySensors.h** → Deve vir ANTES.
- [ ] ❌ **EEPROM.put() direto no nó** → Use `nodeEngine_saveInterval()`.
- [ ] ❌ **Hardcoding de strings de comando** → Use `CMD_FORCE_UPDATE` de `config.h`.
- [ ] ❌ **Dois perfis de energia** → Escolha apenas `POWER_PROFILE_LOW_POWER` ou `ALWAYS_ON`.
- [ ] ❌ **Mensagens sem dimensionamento correto** → Use `messages[NODE_ITEMS_COUNT + X]`.

---
---

## 6. Referência Rápida MySensors API (v2.x)

Este guia resume os elementos core da biblioteca MySensors necessários para o desenvolvimento de nós.

### 6.1 Ciclo de Vida do Nó
- **`before()`**: Chamado antes de `begin()`. Útil para configurar pinos e inicializar sensores que precisam estar prontos para o rádio.
- **`presentation()`**: Onde o nó se apresenta ao gateway e registra seus sensores (`present()`).
- **`setup()`**: Inicialização padrão do Arduino após o MySensors estar pronto.
- **`loop()`**: Loop principal. Evite `delay()` longos para não bloquear o transporte de rádio. Use `wait()` se necessário.
- **`receive(const MyMessage &message)`**: Callback acionada ao receber mensagens (C_SET ou C_REQ).

### 6.2 Funções Core da API
- **`present(uint8_t childId, uint8_t type, [const char *alias])`**: Registra um sensor (S_...).
- **`send(MyMessage &msg, [bool ack])`**: Envia uma mensagem. `ack=true` solicita confirmação.
- **`request(uint8_t childId, uint8_t type, [uint8_t destination])`**: Solicita um valor ao controller.
- **`wait(uint32_t ms)`**: Pausa o loop mas continua processando rádio e mensagens.
- **`sleep(uint32_t ms / uint8_t interrupt, uint8_t mode, uint32_t ms)`**: Coloca o rádio e a MCU em baixo consumo.

### 6.3 Tipos Comuns (S_..., V_..., I_...)
| Categoria | Tipo (Exemplos) | Uso |
| :--- | :--- | :--- |
| **Sensores (S_)** | `S_TEMP`, `S_HUM`, `S_BINARY`, `S_WATER`, `S_CUSTOM` | Usado em `present()`. |
| **Variáveis (V_)** | `V_TEMP`, `V_HUM`, `V_STATUS`, `V_FLOW`, `V_CUSTOM` | Usado em `send()` / `request()`. |
| **Internas (I_)** | `I_BATTERY_LEVEL`, `I_TIME`, `I_SKETCH_NAME` | Configuração e status interno. |

### 6.4 Macros de Configuração (Colocar ANTES de #include <MySensors.h>)
- **Radio**: `MY_RADIO_RF24`, `MY_RADIO_RFM69`, `MY_RADIO_NRF5_ESB`.
- **Node**: `MY_NODE_ID 254`, `MY_REPEATER_FEATURE`.
- **Gateway**: `MY_GATEWAY_ESP8266`, `MY_GATEWAY_MQTT_CLIENT`.
- **Debug**: `MY_DEBUG`.

---

## 6. Padrões de Qualidade e Ferramentas (Core MySensors)

Para alinhar o desenvolvimento com o core do MySensors, os seguintes padrões devem ser observados (baseados no `style.cfg` oficial):

- **Indentação**: Sempre use **Tabs**.
- **Chaves**: Sempre use chaves `{}`, mesmo em declarações de uma única linha (padrão `1tbs`).
- **Nomenclatura**: Use `camelCase` para funções e variáveis. Evite notação húngara (ex: `iCount`).
- **Largura de Linha**: Máximo de 100 caracteres.
- **Macros**: Use parênteses extras e identação em definições multi-linha.
- **Análise Estática**: O código deve ser compatível com `cppcheck` (evitar warnings de estilo e portabilidade).

---
*Referência baseada na análise de: `nodeSelenoieVazao`, `nodeZTS_UmidadeHall`, `nodeUmidadeTemperatura` e padrões oficiais do MySensors Core.*
