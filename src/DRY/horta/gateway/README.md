# Gateway MySensors ↔ MQTT da M360 Horta

ESP8266 D1 mini que faz a ponte entre a rede de rádio MySensors e o broker MQTT. O sketch
[`libDryGatewayMqtt.cpp`](libDryGatewayMqtt.cpp) é só a composição: toda a infraestrutura vive em
`lib/M360-DRY/`. Hardware em [`esquema_eletrico.md`](esquema_eletrico.md) e
[`diagrama_blocos.svg`](diagrama_blocos.svg); a placa definitiva é a **M360-GW**
(`hardware/m360-gw/`, `ARQUITETURA_FAMILIA.md` §6).

## Módulos (`lib/M360-DRY/src/`)

| Módulo | Responsabilidade |
|---|---|
| `M360Gateway` | Orquestra o `loop()`: reconexão WiFi e MQTT, webserver, LEDs, heartbeat, timeouts de nós |
| `M360WiFi` | Modo STA com a rede configurada, ou AP de configuração `M360-Config` (`192.168.4.1`) |
| `M360Webserver` | Portal com as rotas `/` e `/save` |
| `M360MQTT` | Conexão ao broker, tópicos `buildTopicOut()` / `buildTopicIn()`, assinatura de `in/#` |
| `M360Translator` | Serialização MySensors ↔ MQTT (modo nativo e envelope JSON), validação |
| `M360Registry` | Nós ativos, `node_discovered`, `node_lost` (15 min sem mensagem), `node_reconnected` |
| `M360Config` | `M360DeviceConfig` na EEPROM a partir de 521, com CRC, gravado campo a campo |
| `M360Leds` | Três LEDs de status (pinos abaixo) |

## Hardware e envs

| Env | Placa | Rádio | LEDs (todos ativos em HIGH) | Reed / botão |
|---|---|---|---|---|
| `d1_mini_gateway` | Protótipo em bancada (COM5), hardware nRF24 | nRF24: CE D4 (`MY_RF24_CE_PIN=2`), CSN D8. Rádio fixo em nRF24 (`[proto_rf24]`), fora do `radio_flags` | vermelho D2, amarelo D0, verde D1 | A0 com pull-up de 10 kΩ |
| `d1_mini_gw` | D1 mini no carrier **M360-GW** (`-D M360_BOARD_GW`) | RFM95W: DIO0 D1, NSS D8, sem RESET. nRF24 no header J4: CE D1 (`MY_RF24_CE_PIN=5`), CSN D8 | vermelho D2, amarelo D0, **verde D4** pelo 2N7002 Q1 (o D1 virou DIO0) | reed SW1 em A0, ímã pela parede |

No `d1_mini_gw` o rádio vem de `[common] radio_flags` em `../platformio.ini` (hoje RFM95W); o `d1_mini_gateway` é fixo em nRF24. Os envs só definem pinos.
`initLEDs(LED_RED, LED_YELLOW, LED_GREEN)` segue a ordem do protótipo `(red, yellow, green)`.

Na M360-GW o verde fica aceso durante reset e gravação, porque o GPIO2 é o TXD do boot ROM e só
cai no `initLEDs()`. Esperado, não é defeito.

## Ciclo de vida

1. **`before()`** — `initLEDs()`, EEPROM, `loadConfig()`. Configuração inválida ou A0 em GND no boot → modo AP; senão → WiFi STA. Só WiFi aqui: MQTT e webserver são de `setup()`.
2. **`setup()`** — webserver, MQTT (só em STA), `gateway.begin(...)`; apresenta o gateway como nó 0, `Manejo360 Gateway MQTT 2.0.0`.
3. **`loop()`** — `M360Gateway::loop()`: tudo não bloqueante, `wait()` curto para o rádio.

## MQTT

Modo nativo (`M360_NATIVE_MQTT=1` no env): os campos vão no tópico e o payload é o valor bruto.

```
subida:  m360/{UF}/{CAR}/out/{nodeId}/{childId}/{command}/{ack}/{type}   payload: "23.5"
descida: m360/{UF}/{CAR}/in/{nodeId}/{childId}/{command}/{ack}/{type}    payload: "1"
eventos: m360/{UF}/{CAR}/out/events                                       JSON
```

Na descida o `Translator` também aceita o envelope JSON (`nodeId`, `sensorId`, `command`, `type`,
`payload`) e a forma simplificada `{"action": "..."}`. Para `V_STATUS` o payload do nó é lido com
`atoi()`: só `"0"` e `"1"` valem, qualquer outra coisa **desliga**.

Todo envio ao rádio passa por `dispatchCommand()`, que valida, envia com `send(msg, false)` e
publica `publishTransportAck()` ou `command_send_failed`. Esse ACK prova só o auto-ACK do próximo
salto; a confirmação real é o eco do nó. Detalhes e armadilhas em [`CLAUDE.md`](../../../../CLAUDE.md),
seção "Armadilhas Críticas".

Heartbeat a cada 60 s em `.../out/events` com uptime, RSSI do WiFi, IP e versão.

## EEPROM

| Região | Dono |
|---|---|
| 0–511 | MySensors (IDs, roteamento). Intocável |
| 512–520 | `M360NodeConfig` (reservado) |
| 521+ | `M360DeviceConfig`: SSID, senha, broker, usuário, UF, CAR, CRC16 |

## Modo AP e factory reset

- Ímã no reed (ou botão) com A0 em GND **durante o boot** → AP `M360-Config`, portal em `192.168.4.1`.
- O A0 é lido uma única vez, no `before()`. **Factory reset não está implementado**, nem pelo portal nem pelo A0 por 3 s; pendência em `hardware/ARQUITETURA_FAMILIA.md` §9. Para voltar ao padrão, regravar com a EEPROM limpa ou salvar uma configuração nova pelo portal.
- Sem o pull-up de 10 kΩ no A0 o pino flutua perto de 0 V e o gateway sobe em AP sozinho.

## Antes de compilar

```bash
cp include/M360Credentials.h.example include/M360Credentials.h
```

Preencher AP, STA, MQTT, UF e CAR. O arquivo é ignorado pelo Git.

```bash
pio run -e d1_mini_gw -t upload && pio device monitor -e d1_mini_gw
```
