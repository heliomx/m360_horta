# M360 Horta — Referência MQTT para Desenvolvedor Backend

Modo de operação ativo: **MySensors nativo** (`M360_NATIVE_MQTT=1`).  
O gateway publica e consome mensagens no formato de tópico plano do protocolo MySensors — sem envelope JSON.

---

## 1. Tópicos MQTT

| Direção | Tópico | Quem publica | Quem consome |
|---|---|---|---|
| Rede → Backend | `m360/DF/0000/out/{nodeId}/{sensorId}/{command}/{ack}/{type}` | Gateway ESP8266 | Backend / Node-RED |
| Backend → Rede | `m360/DF/0000/in/{nodeId}/{sensorId}/{command}/{ack}/{type}` | Backend / Node-RED | Gateway ESP8266 |
| Heartbeat | `m360/DF/0000/gateway/status` | Gateway ESP8266 | Backend |
| Eventos | `m360/DF/0000/out/events` | Gateway ESP8266 | Backend |

> **Wildcard de subscrição:** `m360/DF/0000/out/#` captura leituras, ACKs e eventos em um único subscribe.

---

## 2. Estrutura do Tópico

```
m360 / DF / 0000 / out / {nodeId} / {sensorId} / {command} / {ack} / {type}
  ^      ^     ^     ^       ^           ^            ^          ^       ^
  |      |     |     |    ID do nó    ID do sensor  Comando   ACK bit  Tipo de variável
  |      |     |  direção                                    (0 ou 1)
  |      |   ID da fazenda
  |    estado (UF)
prefixo
```

- **Payload**: string bruta com o valor da leitura (ex: `"23.5"`, `"1"`, `"REPRESENT"`)
- O **ack bit** no tópico indica ACK de transporte RF24: `1` = o nó confirmou recebimento, `0` = envio normal

---

## 3. Campos Numéricos

### command

| Valor | Constante | Significado |
|---|---|---|
| `0` | `C_PRESENTATION` | Apresentação de sensor/atuador — nó enviando sua configuração |
| `1` | `C_SET` | Definir valor — leitura de sensor **ou** comando para atuador |
| `2` | `C_REQ` | Requisitar valor atual — backend pergunta, nó responde |
| `3` | `C_INTERNAL` | Mensagem interna do protocolo (heartbeat, nome do sketch, etc.) |

### type — variáveis de sensor/atuador (command = 1 ou 2)

| Valor | Constante | Unidade | Uso neste projeto |
|---|---|---|---|
| `0` | `V_TEMP` | °C | Temperatura (nós 04, 13, 80) |
| `1` | `V_HUM` | % | Umidade do ar |
| `2` | `V_STATUS` | `0`/`1` | Estado de relé/atuador (nó 99) |
| `3` | `V_PERCENTAGE` | % | Umidade de solo (nós 01 e 13, após remapeamento) |
| `13` | `V_DISTANCE` | cm | Ultrassônico (nó 80) |
| `34` | `V_FLOW` | L/min | Vazão YF-S201 (nó 80) |
| `35` | `V_VOLUME` | L | Volume acumulado (nó 80) |
| `37` | `V_LEVEL` | % | **Umidade de solo nos nós 01 e 13** ⚠ ver nota abaixo |
| `38` | `V_VOLTAGE` | V | Tensão de bateria |
| `48` | `V_CUSTOM` | — | Comandos administrativos (`REPRESENT`, `teste_do_gateway`) |
| `51` | `V_PH` | — | pH da solução (nó 80) |
| `53` | `V_EC` | mS/cm | Condutividade elétrica (nó 80) |
| `24` | `V_VAR1` | minutos | Intervalo de envio (sensorId = 254) |

### type — mensagens internas (command = 3)

| Valor | Constante | Significado |
|---|---|---|
| `11` | `I_SKETCH_NAME` | Nome do firmware do nó |
| `12` | `I_SKETCH_VERSION` | Versão do firmware |
| `14` | `I_GATEWAY_READY` | Gateway pronto |
| `17` | `I_PRESENTATION` | Solicitar/responder apresentação |
| `19` | `I_PRESENTATION` | Broadcast de reapresentação |
| `22` | `I_HEARTBEAT_RESPONSE` | Heartbeat periódico do nó |

---

## 4. IDs Reservados (sensorId)

| sensorId | Uso |
|---|---|
| `254` | Intervalo de envio — `V_VAR1` (minutos) |
| `255` | Bateria — `V_VOLTAGE` (Volts); também usado como broadcast |

---

## 5. Nós da Rede

| nodeId | Hardware | Perfil | Sensores / Atuadores |
|---|---|---|---|
| `0` | ESP8266 D1 Mini | Gateway | WiFi + MQTT + rádio RF24 |
| `1` | Pro Mini 3.3V/8MHz | LOW_POWER (sleep) | 18× umidade de solo — canteiros A/B (sensorId 0–17) |
| `4` | Pro Mini 3.3V/8MHz | LOW_POWER (sleep) | Clima — temperatura/umidade do ar |
| `13` | Arduino Nano | LOW_POWER (sleep) | ZTS-3002 Modbus: umidade/temp/CE/pH/N/P/K; Hall; LDR; relé |
| `80` | Arduino Nano | ALWAYS_ON | pH, EC, DS18B20, ultrassônico, 4× vazão YF-S201 |
| `99` | Arduino Nano | ALWAYS_ON | 9 relés (solenóides/bombas) + DHT11 |

---

## 6. Mapeamento de Sensores — Nó 01 (Umidade de Solo)

O nó 01 possui 18 sensores multiplexados (MUX CD74HC4067), dois canteiros (A e B), três profundidades (10, 20, 30 cm) e três distâncias do caule (1, 3, 5 m):

| sensorId | Nome | sensorId | Nome |
|---|---|---|---|
| `0` | A_1m_10cm | `9` | B_1m_10cm |
| `1` | A_1m_20cm | `10` | B_1m_20cm |
| `2` | A_1m_30cm | `11` | B_1m_30cm |
| `3` | A_3m_10cm | `12` | B_3m_10cm |
| `4` | A_3m_20cm | `13` | B_3m_20cm |
| `5` | A_3m_30cm | `14` | B_3m_30cm |
| `6` | A_5m_10cm | `15` | B_5m_10cm |
| `7` | A_5m_20cm | `16` | B_5m_20cm |
| `8` | A_5m_30cm | `17` | B_5m_30cm |

**Tipo da variável:** `V_LEVEL` (37) — escala 0 (solo seco) → 100 (saturado com água).

> ⚠ **Atenção:** o firmware usa `V_LEVEL` (37) para umidade de solo por decisão histórica, não `V_PERCENTAGE` (3). O backend deve tratar `V_LEVEL` dos nós `1` e `13` como percentual de umidade.

**Exemplo de mensagem recebida:**
```
Tópico : m360/DF/0000/out/1/3/1/0/37
Payload: 68.5
```
→ Canteiro A, 3m do caule, 10cm de profundidade: **68.5 % de umidade**

---

## 7. Mapeamento de Atuadores — Nó 99 (Relés)

| sensorId | Nome | Tipo de atuação |
|---|---|---|
| `0` | Solenóide A — Canteiro A | MUX canal 0 |
| `1` | Solenóide B — Canteiro B | MUX canal 1 |
| `2` | Solenóide C — Canteiro C | MUX canal 2 |
| `3–6` | Reservados | MUX canais 3–6 |
| `7–15` | Perístulas / Suplementares | MUX canais 7–15 |
| `16` | Bomba NFT Hidroponia | Pino nativo D2 |
| `17` | Bomba de Oxigenação | Pino nativo D8 |

Payload de estado: `"1"` = LIGADO, `"0"` = DESLIGADO.

---

## 8. Enviando Comandos para a Rede

### 8.1 Ligar / Desligar um relé (Nó 99)

```
Tópico : m360/DF/0000/in/99/{sensorId}/1/0/2
Payload: 1   ← LIGAR
         0   ← DESLIGAR
```

Exemplo — ligar Solenóide A:
```
Tópico : m360/DF/0000/in/99/0/1/0/2
Payload: 1
```

O nó responde enviando de volta o estado atual:
```
Tópico : m360/DF/0000/out/99/0/1/0/2
Payload: 1
```

### 8.2 Forçar leitura / reapresentação de um nó

Válido para qualquer nó. Para nós em **sleep** (`LOW_POWER`), o comando será processado na próxima vez que o nó acordar.

```
Tópico : m360/DF/0000/in/{nodeId}/0/1/0/48
Payload: REPRESENT
```

Exemplos:
```
m360/DF/0000/in/1/0/1/0/48   REPRESENT   ← nó 01: re-envia 18 leituras de umidade
m360/DF/0000/in/99/0/1/0/48  REPRESENT   ← nó 99: re-envia estado de todos os relés
```

### 8.3 Solicitar reapresentação de toda a rede (broadcast)

```
Tópico : m360/DF/0000/in/255/255/3/0/19
Payload: (vazio)
```

### 8.4 Alterar intervalo de envio de um nó

O intervalo é armazenado em EEPROM e persiste após reinicializações.

```
Tópico : m360/DF/0000/in/{nodeId}/254/1/0/24
Payload: {minutos}
```

Exemplo — intervalo de 10 minutos para o nó 01:
```
m360/DF/0000/in/1/254/1/0/24   10
```

### 8.5 Requisitar valor atual de um sensor (C_REQ)

```
Tópico : m360/DF/0000/in/{nodeId}/{sensorId}/2/0/{type}
Payload: (vazio)
```

Exemplo — requisitar temperatura do nó 80, sensor 0:
```
m360/DF/0000/in/80/0/2/0/0
```

---

## 9. Fluxo de ACK de Atuador

Para comandos de relé (`command=1, type=2`), o nó 99 confirma a execução enviando de volta o estado:

```
Backend → Gateway    m360/DF/0000/in/99/0/1/0/2    "1"
                               ↓ RF24
                           Nó 99 executa e responde
                               ↓ RF24
Gateway → Backend    m360/DF/0000/out/99/0/1/0/2   "1"
```

O campo `ack` no tópico de resposta do nó é `0` (não é ACK de transporte — é uma confirmação semântica via re-envio do estado). O ACK de transporte RF24 (`ack=1`) indica apenas que o pacote de rádio foi entregue ao nó, não que o atuador foi acionado.

---

## 10. Apresentação de Sensores (command = 0)

Quando um nó inicia ou é solicitado a se reapresentar, envia uma mensagem de apresentação para cada sensor:

```
Tópico : m360/DF/0000/out/{nodeId}/{sensorId}/0/0/{sensorType}
Payload: {alias do sensor}   ← ex: "A_1m_10cm", "Solenóide A", ""
```

| sensorType | Constante | Uso típico |
|---|---|---|
| `3` | `S_BINARY` | Relé / atuador on-off |
| `6` | `S_TEMP` | Sensor de temperatura |
| `7` | `S_HUM` | Umidade do ar |
| `35` | `S_MOISTURE` | Umidade de solo |
| `17` | `S_ARDUINO_NODE` | O próprio nó (sensorId = 255) |

---

## 11. Mensagens Internas Relevantes (command = 3)

```
Tópico : m360/DF/0000/out/{nodeId}/255/3/0/11
Payload: "M360 Node 99 Reles"          ← nome do sketch (I_SKETCH_NAME)

Tópico : m360/DF/0000/out/{nodeId}/255/3/0/12
Payload: "2.1"                         ← versão (I_SKETCH_VERSION)

Tópico : m360/DF/0000/out/{nodeId}/255/3/0/22
Payload: "1"                           ← heartbeat do nó (I_HEARTBEAT_RESPONSE)
```

---

## 12. Armadilhas Conhecidas

### V_LEVEL ≠ Nível de reservatório para os nós 01 e 13
Os nós 01 e 13 usam `type=37` (`V_LEVEL`) para umidade de solo por limitação histórica do firmware. O backend **deve** tratar como percentual (0–100 %) quando `nodeId` é `1` ou `13`. Os demais nós que usarem `V_LEVEL` referem-se ao nível do reservatório de água (nó 80, sensor ultrassônico).

### Nó 01 em sleep — latência de resposta
O nó 01 (`LOW_POWER`) dorme entre leituras (padrão: 10 min). Comandos enviados ao nó 01 só são processados quando ele acorda. O backend não deve esperar resposta imediata — implementar timeout de pelo menos 15 minutos antes de considerar falha.

### sensorId 255 — broadcast vs. bateria
`sensorId=255` em `command=3` e `type=38` (`V_VOLTAGE`) é a leitura de bateria do nó. Em contexto de comando (`command=1` ou `command=2`), `sensorId=255` é broadcast (todos os sensores do nó).

### Pinos virtuais MUX (Nó 99)
Os canais do MUX CD74HC4067 são mapeados internamente como `pin = 100 + canal`. O backend não precisa conhecer essa implementação — use apenas `sensorId` 0–15 para os canais MUX e `sensorId` 16–17 para os pinos nativos.
