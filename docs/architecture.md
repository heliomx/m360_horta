# Technical Architecture (Solution Design) — M360 Horta

## 1. Visão Geral do Sistema
O sistema M360 Horta utiliza uma arquitetura em camadas baseada no protocolo **MySensors** para a rede de rádio e **MQTT** para a camada de integração e controle. O diferencial técnico reside na aplicação estrita do princípio **DRY** no firmware dos nós e do gateway.

## 2. Pilha Tecnológica
- **MCU (Nodes):** ATMega328P (Arduino Nano / Pro Mini).
- **MCU (Gateway):** ESP8266 (D1 Mini). Não é uma escolha aberta: `M360Registry.h`
  tem `#error` se compilado fora do ESP8266.
- **Transporte de Rádio:** NRF24L01+ (2.4 GHz), canal padrão do MySensors (76),
  `RF24_250KBPS`, `RF24_PA_HIGH`.
- **Protocolo de Integração:** MQTT via WiFi.
- **Formato de Dados:** ver §5.1 — o build atual usa **tópico nativo**, não JSON.

> RS485 / Modbus RTU saiu da pilha junto com o nó ZTS (13), removido do projeto.

## 3. Padrões de Projeto (Firmware DRY)

### 3.1 Biblioteca M360-DRY
A arquitetura ativa concentra o ciclo de vida em `lib/M360-DRY`:
- **`M360Node`**: apresenta itens, processa leituras, comandos, bateria e perfis de energia.
- **`M360Gateway`**: orquestra WiFi, MQTT, webserver, heartbeat e registro de nós.
- **`M360Translator`**: implementa o contrato JSON/MySensors bidirecional.
- **`M360Registry`**: rastreia nós vivos e declara `node_lost` — ver §5.3.

O diretório `src/DRY/horta/nos/shared/node_engine.*` é legado e não deve ser usado
em novos nós.

### 3.2 Isolamento de Hardware (`sensorDrivers`)
Cada nó separa a lógica de aplicação da implementação física dos drivers. O motor
conhece apenas dois callbacks, registrados em `setup()`:

```cpp
node.onRead (float (*)(uint8_t nodeIndex));
node.onWrite(void  (*)(uint8_t nodeIndex, bool state));
```

> ⚠️ **O parâmetro é o índice em `NODE_ITEMS[]`, não o childId.** `M360Node` chama
> `_readCb(i)` / `_writeCb(i, state)` com o índice do laço. Tratar esse valor como
> childId indexa o array errado — o nó aciona outro atuador, ou lê fora dos limites,
> sem nenhum erro de compilação. Para chegar ao childId, use `NODE_ITEMS[i].childId`.

O **nome** da função de driver é escolha de cada nó, e eles divergem de propósito:

| Nó | Callback registrado | Observação |
|---|---|---|
| 4 | `readNodeItem(uint8_t nodeIndex)` | registrado direto no motor |
| 99 | `readItem(uint8_t index)` | adaptador que despacha por `NODE_ITEMS[index].childId` e só então chama `readNodeItem(uint8_t pin)` — que, nesse nó, recebe **pino** |

Ao ler o driver de um nó, confirme o que o parâmetro significa **naquele arquivo**
antes de copiar o padrão para outro.

## 4. Gestão de Energia e Proteção de Sensores

### 4.1 Ciclo de Vida do Sono
Nó `M360_LOW_POWER` — hoje só o Nó 4 — segue a sequência. Não é deep sleep de ESP:
é `smartSleep()`, que dorme por watchdog no AVR e entrega a fila de mensagens ao
acordar.
1. `M360::powerUp()` (ativa pinos de alimentação).
2. Leitura de Sensores.
3. Transmissão.
4. Janela de Escuta (`MIN_AWAKE_TIME_MS`).
5. `M360::powerDown()` -> `smartSleep()`.

### 4.2 Alimentação Pulsada
Para evitar a degradação galvânica dos eletrodos de solo, a barra de pull-up é
energizada por um pino digital só durante a varredura e desligada logo depois — nos
nós 1 e 2 é o `PIN_POWER_SENSORS` (D3), com 20 ms de acomodação e uma leitura de ADC
descartada por canal. Detalhes e o que ainda não está implementado em
[`praticas_de_campo.md`](praticas_de_campo.md).

Isso vale mesmo em nó `ALWAYS_ON`: o pulso existe contra eletrólise, não para
economizar energia.

## 5. Gateway e Mensageria

### 5.1 Transformação Serial-MQTT
O gateway tem **dois modos**, selecionados em tempo de compilação por
`-D M360_NATIVE_MQTT` no `platformio.ini`. O build em produção usa o **nativo**:

| Modo | Upstream | Downstream |
|---|---|---|
| **Nativo** (ativo, `=1`) | tópico `{prefix}/out/{nodeId}/{sensorId}/{command}/{ack}/{type}` com o **valor cru** no payload | assina `{prefix}/in/#`; os campos vêm do tópico e o payload é só o valor |
| JSON (alternativo) | envelope M360 em `{prefix}/out`, `DynamicJsonDocument(512)` | comandos simplificados (Actions) ou frame MySensors completo |

> Trocar de modo exige ajustar o `Decodificador Nativo MySensors` do Node-RED, que
> aceita os dois formatos, e conferir todo consumidor que casa tópico por posição.

### 5.2 Resiliência
- **Heartbeat:** Verificação de saúde a cada 60 s.
- **Retry Logic:** O gateway gerencia as tentativas de reconexão WiFi e MQTT sem bloquear o tráfego do rádio.

### 5.3 Rastreamento de nós (`M360Registry`)
O gateway declara um nó perdido por limiar **aprendido por nó**, não por constante:

```
base    = max(intervalo declarado no child 254, cadência observada)
          — multiplicado por 10 se o sketch name traz [ON] ou [REP]
timeout = base + max(2 min, 50 % da base)          (teto de 2 h)
```

O multiplicador existe porque `M360Node` só reenvia um sensor sem variação a cada
10 ciclos: um nó sem sleep pode ficar ~11 intervalos calado sem defeito algum. Nó
`[LP]` não precisa dele — o `smartSleep()` emite notificação de sono todo ciclo.

Quando o gateway não conhece o sketch name de um nó (tipicamente após um reboot **do
gateway**), ele pede `I_PRESENTATION` e reenvia a cada 5 min até obter resposta.

A mesma fórmula existe no Node-RED e é contrato de dois lados — ver
[`funcionalidades_nodered.md` §9](../src/DRY/horta/nodered/funcionalidades_nodered.md).

## 6. Segurança e Manutenção
- **Reset de Fábrica:** Detecção de hardware via pino `A0` (GND) para limpar EEPROM e entrar em modo AP de configuração.
- **IDs Reservados:** Child IDs `253` (Debug remoto, `V_TEXT`), `254` (Intervalo,
  `V_VAR1`) e `255` (Bateria, `V_VOLTAGE`) são globais e imutáveis. `M360Node::begin()`
  escreve nas três posições, por isso `messages[]` exige `NODE_ITEMS_COUNT + 3`.
- **Credenciais:** `include/M360Credentials.h` contém os defaults locais e nunca é versionado. O arquivo `include/M360Credentials.h.example` documenta todas as constantes obrigatórias.
- **EEPROM:** Configuração de rede usa versão, CRC e strings limitadas; dados inválidos acionam provisionamento seguro.
- **Comando remoto:** o nó aceita três payloads `V_CUSTOM` — `FORCE_UPDATE` (leitura
  imediata de todos os sensores, ignorando o filtro de variação), `REPRESENT`
  (reapresenta os children) e `DEBUG_NET` (responde o diagnóstico de rede pelo child
  253). Só o `FORCE_UPDATE` força leitura; `FORCE_UPDATE` **não** relê atuadores,
  que não respondem a `C_REQ`.
