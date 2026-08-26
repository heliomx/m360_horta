# Inventário de Nós e Child IDs — M360 Horta

Relação de todos os nós da rede MySensors e de seus child IDs, um a um.

> **Fonte:** extraído dos arrays `NODE_ITEMS[]` / `nodeItems[]` de cada nó, dos
> `#define CHILD_ID_*` dos respectivos `sensorDrivers.h` e dos `#define MY_NODE_ID`
> em `src/DRY/horta/platformio.ini`. Ao alterar qualquer child ID no firmware,
> atualizar esta tabela **e** o `flows.json` — ver
> [Contrato com o Node-RED](#8-contrato-com-o-node-red).
>
> **Regra do projeto:** qualquer alteração de código em `src/DRY/horta/` deve ser
> refletida neste arquivo na **mesma** entrega — ver `CLAUDE.md`, `AGENTS.md`,
> §4.1 do SKILL `bmad-mysensors-node-coding` e R12 do workflow `m360-node-factory`.

---

## 1. Visão geral da rede

| Nó | Env PlatformIO | Placa | Perfil | Children | Consumido pelo Node-RED |
|---:|---|---|---|---|---|
| **0** | `d1_mini_gateway` | ESP8266 D1 Mini | — | — | Gateway MQTT (não tem children) |
| **1** | `nano_01nodeSolo3d` · `ProMini_01nodeSolo3d` | Nano 5V · Pro Mini 16MHz | `LOW_POWER` | 1–6 | Sim — Filtro Umidade Solo Canteiro A |
| **2** | `nano_02nodeSolo3d` | Nano 5V | `LOW_POWER` | 1–6 | Sim — Filtro Canteiro B + Motor de Regras |
| **4** | `ProMini_04noodeSolarMini` | Pro Mini 16MHz | `LOW_POWER` | 1, 11, 12 | Sim — Filtro Clima SolarMini |
| **99** | `nano_99reles` · `nano_99reles_rep` | Nano 5V | `ALWAYS_ON` | 11, 12, 21, 31–39 | Sim — comandos, clima e vazão |

**Total: 27 children declarados** (6 + 6 + 3 + 12), mais os 3 reservados por nó.  
*Nota de Hardware:* Todos os módulos nRF24L01+ de todos os nós e gateways possuem capacitor de desacoplamento instalado (premissa de hardware confirmada).

---

## 2. Como ler os atributos

Cada item é um `M360::M360ItemDef` ([`lib/M360-DRY/src/M360Node.h`](../../../lib/M360-DRY/src/M360Node.h)),
com os campos abaixo na ordem em que aparecem no código:

| Campo | Significado |
|---|---|
| `childId` | Identificador do sensor na rede MySensors (1–252) |
| `kind` | `M360_SENSOR` (só lê) ou `M360_ACTUATOR` (lê e escreve) |
| `presentationType` | Tipo `S_*` anunciado no `present()` — define o ícone/classe no controlador |
| `valueType` | Tipo `V_*` do payload — define como o valor é interpretado |
| `pin` | Pino físico, pino virtual MUX (`100 + canal`) ou `-1` se resolvido em software |
| `reportIntervalMin` | Ciclos mínimos entre envios; `0` = envia sempre que o valor mudar |
| `readSamples` | **Não implementado** — a lib chama o callback de leitura uma única vez por ciclo |
| `label` | Nome enviado no `present()`; é o que o Node-RED exibe e usa em `child.de` |
| `wakeOnRadio` | `true` = solicita o estado ao gateway após acordar |
| `flags` | bit 0 = multiplica o valor por 100 e envia como inteiro |

### Tipo do payload por `valueType`

Determinado por como `M360Node` serializa ([`M360Node.cpp`](../../../lib/M360-DRY/src/M360Node.cpp)):

| `valueType` | Nº | Tipo do payload | Como é enviado |
|---|---:|---|---|
| `V_TEMP`, `V_HUM`, `V_LEVEL`, `V_FLOW` | 0, 1, 37, 34 | **float**, 1 casa decimal | `set(valor, 1)` |
| `V_STATUS` | 2 | **bool** (`0` / `1`) | `set(estado)` |
| `V_VOLTAGE` | 38 | **float**, 1 casa decimal | `set(tensao, 1)` |
| `V_VAR1` | 24 | **uint16** (minutos) | `set(intervalo)` |
| `V_TEXT` | 47 | **string** (máx. 24 chars) | `set(texto)` |
| qualquer, com `flags` bit 0 | — | **int32** (valor × 100) | `set((int32_t)(v * 100))` |

> Nenhum item declarado hoje usa `flags` bit 0 — **todos** os sensores enviam
> float com 1 casa decimal.

---

## 3. Child IDs reservados pela biblioteca

Presentes em **todos** os nós que usam `M360::M360Node`, criados automaticamente
por `begin()`. Nunca declarar em `NODE_ITEMS[]`.

| Child | `valueType` | Payload | Direção | Uso |
|---:|---|---|---|---|
| **253** | `V_TEXT` (47) | string ≤ 24 chars | nó → gateway | Debug remoto (`M360Node::sendDebug()`); responde a `DEBUG_NET` com `P:<parent> D:<dist> R:<S/N>` |
| **254** | `V_VAR1` (24) | uint16 (minutos) | bidirecional | Intervalo de reporte; aceita `C_SET` e confirma reenviando o valor vigente |
| **255** | `V_VOLTAGE` (38) | float, 1 casa | nó → gateway | Tensão da bateria |

> Por isso `messages[]` exige **`NODE_ITEMS_COUNT + 3`**: `begin()` escreve
> incondicionalmente nas três posições reservadas.

### Faixas normativas

Definidas em [`lib/M360-DRY/src/M360Constants.h`](../../../lib/M360-DRY/src/M360Constants.h).

| Faixa | Domínio | Constantes |
|---|---|---|
| 1 – 10 | Solo | `CHILD_ID_SOLO_MIN` / `_MAX` |
| 11 – 20 | Clima | `CHILD_ID_CLIMA_MIN` / `_MAX` |
| 21 – 30 | Hidrometria | `CHILD_ID_FLOW_MIN` / `_MAX` |
| 31 – 40 | Atuação | `CHILD_ID_ACTUATOR_MIN` / `_MAX` |

---

## 4. Nó 1 — `01nodeSolo3dNano` (Canteiro A)

**Env:** `nano_01nodeSolo3d` (Nano 5V) · `ProMini_01nodeSolo3d` (Pro Mini 16MHz)
**Perfil:** `M360_LOW_POWER` — `smartSleep()` entre ciclos
**Hardware:** 6 eletrodos resistivos ligados direto às portas analógicas nativas

| Child | Label | `kind` | `S_*` | `V_*` | Payload | Pino | `reportIntervalMin` | `wakeOnRadio` | `flags` |
|---:|---|---|---|---|---|---|:---:|:---:|:---:|
| **1** | `A_1m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A0 | 0 | false | 0 |
| **2** | `A_1m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A1 | 0 | false | 0 |
| **3** | `A_3m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A2 | 0 | false | 0 |
| **4** | `A_3m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A3 | 0 | false | 0 |
| **5** | `A_5m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A4 | 0 | false | 0 |
| **6** | `A_5m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A5 | 0 | false | 0 |

Convenção do label: `<canteiro>_<distância>_<profundidade>`.
**Escala:** ADC bruto 0–1023 — valor alto = solo seco. Ver [§9](#9-escalas-e-unidades).

---

## 5. Nó 2 — `02nodeSolo3dNano` (Canteiro B)

**Env:** `nano_02nodeSolo3d` (Nano 5V)
**Perfil:** `M360_LOW_POWER`
**Hardware:** idêntico ao nó 1, instalado no canteiro B. Alimenta o **Motor de Regras Canteiro B**.

| Child | Label | `kind` | `S_*` | `V_*` | Payload | Pino | `reportIntervalMin` | `wakeOnRadio` | `flags` |
|---:|---|---|---|---|---|---|:---:|:---:|:---:|
| **1** | `B_1m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A0 | 0 | false | 0 |
| **2** | `B_1m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A1 | 0 | false | 0 |
| **3** | `B_3m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A2 | 0 | false | 0 |
| **4** | `B_3m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A3 | 0 | false | 0 |
| **5** | `B_5m_10cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A4 | 0 | false | 0 |
| **6** | `B_5m_30cm` | `M360_SENSOR` | `S_MOISTURE` | `V_LEVEL` (37) | float, 1 casa | A5 | 0 | false | 0 |

---

## 6. Nó 4 — `04noodeSolarMini` (clima + solo, alimentação solar)

**Env:** `ProMini_04noodeSolarMini` (Pro Mini 5V/16MHz)
**Perfil:** `M360_LOW_POWER`
**Hardware:** DHT11 (ar) + DS18B20 (solo), painel solar com bateria

| Child | Constante | Label | `kind` | `S_*` | `V_*` | Payload | Sensor | Pino | `reportIntervalMin` | `wakeOnRadio` | `flags` |
|---:|---|---|---|---|---|---|---|---|:---:|:---:|:---:|
| **1** | `CHILD_ID_SOIL_TEMP` | `Temperatura Solo` | `M360_SENSOR` | `S_TEMP` | `V_TEMP` (0) | float, 1 casa | DS18B20 | −1 | 60 | false | 0 |
| **11** | `CHILD_ID_TEMP` | `Temperatura Ar` | `M360_SENSOR` | `S_TEMP` | `V_TEMP` (0) | float, 1 casa | DHT11 | −1 | 60 | false | 0 |
| **12** | `CHILD_ID_HUM` | `Umidade Ar` | `M360_SENSOR` | `S_HUM` | `V_HUM` (1) | float, 1 casa | DHT11 | −1 | 60 | false | 0 |

`pin = -1` porque a leitura é resolvida por `childId` em `sensorDrivers.cpp`, não
por porta.

> **A coluna `reportIntervalMin` é documentação, não configuração.** O campo
> `M360ItemDef.reportIntervalMin` não é lido por nenhum ponto de `M360Node` — a
> cadência real do nó é `_interval`, carregado de `loadInterval()` (EEPROM 512–515)
> e alterável só pelo child 254. A coluna registra a cadência **pretendida**.
>
> Consequência para o Nó 4: `-D M360_DEFAULT_INTERVAL=60` no `platformio.ini` só
> vale para EEPROM virgem. O nó já em campo gravou `magic + 1` no primeiro boot da
> versão anterior, e **regravar o firmware mantém o ciclo de 1 minuto**. Para
> passar a 60 min de fato, depois de gravar dispare **⏱️ Definir Intervalo = 60**
> no dashboard (aba de comandos), com o nó 4 selecionado. O nó é `M360_LOW_POWER` e
> fica acordado ~3 s por ciclo, mas não é preciso acertar a janela na mão: a
> **Caixa Postal** do Node-RED retém o comando e o despacha no próximo despertar
> (ver `nodered/funcionalidades_nodered.md` §5.5). A confirmação é o eco do child 254
> em `.../out` — é ele que ensina o timeout dinâmico ao gateway, e o Telegram avisa
> com *"PARÂMETRO CONFIRMADO: Intervalo de Envio"*.

> Este nó **sobrescreve** `M360::readBatteryVoltage()` (divisor 100k/100k em
> `PIN_BATTERY_ADC`) em vez de usar o bandgap interno de 1,1 V da lib. O child 255
> continua reportando float com 1 casa.

---

## 7. Nó 99 — `99nodeReles` (atuação central da estufa)

**Env:** `nano_99reles` · `nano_99reles_rep` (mesmo hardware, com `MY_REPEATER_FEATURE`)
**Perfil:** `M360_ALWAYS_ON` — fonte fixa, timer por `millis()`, nunca dorme
**Hardware:** MUX CD74HC4067 + 9 relés + DHT11 + sensor de vazão YF-S201

### 7.1 Sensores

| Child | Constante | Label | `kind` | `S_*` | `V_*` | Payload | Pino | `reportIntervalMin` | `wakeOnRadio` | `flags` |
|---:|---|---|---|---|---|---|---|:---:|:---:|:---:|
| **11** | `CHILD_ID_DHT_TEMP` | `Temperatura do Ar` | `M360_SENSOR` | `S_TEMP` | `V_TEMP` (0) | float, 1 casa | D2 | 0 | false | 0 |
| **12** | `CHILD_ID_DHT_HUM` | `Umidade do Ar` | `M360_SENSOR` | `S_HUM` | `V_HUM` (1) | float, 1 casa | D2 | 0 | false | 0 |
| **21** | `CHILD_ID_FLOW` | `Vazao.Irrig` | `M360_SENSOR` | `S_WATER` | `V_FLOW` (34) | float, 1 casa | D3 (INT1) | 0 | false | 0 |

O child 21 declara `pin = -1`: a leitura é despachada por `childId` para
`readFlowLpm()`, que consome o contador de pulsos da ISR ligada a D3/INT1.

### 7.2 Atuadores

| Child | Constante | Label | `kind` | `S_*` | `V_*` | Payload | Campo `pin` | Pino/canal | `reportIntervalMin` | `wakeOnRadio` | `flags` | Carga |
|---:|---|---|---|---|---|---|---|---|:---:|:---:|:---:|---|
| **31** | `CHILD_ID_SOL_A` | `Sol.CanteiroA` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 0` = 100 | MUX C0 | 0 | false | 0 | Solenóide gotejamento Canteiro A, 12V |
| **32** | `CHILD_ID_SOL_B` | `Sol.CanteiroB` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 1` = 101 | MUX C1 | 0 | false | 0 | Solenóide gotejamento Canteiro B, 12V |
| **33** | `CHILD_ID_SOL_C` | `Sol.CanteiroC` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 2` = 102 | MUX C2 | 0 | false | 0 | Solenóide gotejamento Canteiro C, 12V |
| **34** | `CHILD_ID_PERIST_A` | `Perist.SuplA` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 3` = 103 | MUX C3 | 0 | false | 0 | Peristáltica suplemento A, 12V/0,5A |
| **35** | `CHILD_ID_PERIST_B` | `Perist.SuplB` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 4` = 104 | MUX C4 | 0 | false | 0 | Peristáltica suplemento B, 12V/0,5A |
| **36** | `CHILD_ID_PH_PLUS` | `Perist.pH+` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 5` = 105 | MUX C5 | 0 | false | 0 | Peristáltica pH+, 12V/0,5A |
| **37** | `CHILD_ID_PH_MINUS` | `Perist.pH-` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `MUX_CHANNEL_OFFSET + 6` = 106 | MUX C6 | 0 | false | 0 | Peristáltica pH−, 12V/0,5A |
| **38** | `CHILD_ID_NFT_PUMP` | `BombaNFT` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `PIN_NFT_PUMP` = A0 | Nativo A0 | 0 | false | 0 | Bomba circulação NFT, 220V AC |
| **39** | `CHILD_ID_NFT_OXI` | `BombaOxi` | `M360_ACTUATOR` | `S_BINARY` | `V_STATUS` (2) | bool (`0`/`1`) | `PIN_NFT_OXI` = A1 | Nativo A1 | 0 | false | 0 | Bomba oxigenação NFT, 220V AC |

**Concorrência:** apenas **um** canal MUX (31–37) pode estar ligado por vez — o
driver desliga o anterior antes de comutar. Os nativos (38, 39) são livres.

Ao comutar, o nó **ecoa `V_STATUS=0` do child preemptado** antes do eco do child
comandado (`writeNodeItem()` devolve o canal desligado; ver
`reportPreemptedMuxChannel()`). O Node-RED recebe, nessa ordem, `31→0` e `32→1`.
Sem esse eco, ligar a Solenóide B durante a rega da A cortaria a irrigação da A
sem nenhum sinal — o dashboard continuaria mostrando as duas ligadas.

**Lógica dos relés:** Active-LOW (`LOW` = ligado), com ligação em NA/COM para que
uma falha ou reset deixe as cargas desligadas.

**Pinagem completa:** MUX `SIG=D8`, `S0–S3=D4–D7` · DHT11 `D2` · Vazão `D3` ·
Bombas `A0`/`A1` · NRF24 `D9–D13`.

### 7.3 Failsafe de atuação

Cada atuador tem um **tempo máximo ligado**. Ao estourar, o nó desliga a carga
sozinho e envia `V_STATUS=0` ao gateway — o Node-RED recebe isso como uma
leitura normal (`command=1`, `type=2`, `ack=0`) e reconcilia o estado.

| Child | Carga | Tempo máximo | Motivo |
|---:|---|---:|---|
| 31, 32, 33 | Solenóides de gotejamento | **600 s** | A irrigação real usa no máximo 300 s (cron do Canteiro A) |
| 34, 35, 36, 37 | Peristálticas (suplemento, pH) | **120 s** | Dosagem é sempre curta |
| 38, 39 | Bombas NFT (circulação, oxigenação) | **sem limite** | Regime contínuo por projeto — um timeout interromperia a hidroponia |

Implementado em `99nodeReles.cpp` (`maxOnSecondsFor()`, `armFailsafe()`,
`checkActuatorFailsafe()`), avaliado a cada volta do `loop()`.

**Por que existe:** nenhuma camada acima do nó garante o comando OFF. O
desligamento da irrigação é agendado por `setTimeout()` dentro de um function
node do Node-RED, que morre em qualquer redeploy ou restart; e o gateway publica
o ACK assim que o rádio confirma o salto (`send(outMsg, false)` +
`publishTransportAck()`), então o Sincronizador dá o comando por confirmado e
nunca reenvia. Este failsafe é a única proteção que sobrevive à queda do WiFi,
do MQTT, do Node-RED e do próprio gateway.

> Ao alterar um tempo aqui, conferir se o fluxo do Node-RED não pede uma janela
> maior — uma rega mais longa que o limite seria cortada pela metade, e o único
> sinal seria o `V_STATUS=0` inesperado.

Ao ligar um canal MUX, o failsafe desarma o prazo dos demais canais MUX — o
hardware já os desligou, e disparar o failsafe sobre um relé desligado geraria
log e tráfego de rádio inúteis. O eco de `V_STATUS=0` do child preemptado é
responsabilidade do `writeItem()` (ver §7.2).

---

## 8. Contrato com o Node-RED

Os child IDs abaixo são **endereçados por número fixo** em
[`nodered/flows.json`](nodered/flows.json). Alterá-los no firmware sem atualizar
o flow faz os comandos serem **descartados em silêncio**: `handleMessage()` casa
`childId` exato e não responde a IDs desconhecidos — o sintoma é timeout no
Sincronizador ACK, não erro de log.

| Nó / Child | Onde aparece no flow | Uso |
|---|---|---|
| 99 / 31 | `Controlador Canteiro A`, botões, `mqtt out` | Liga/desliga Solenóide A |
| 99 / 32 | `Motor de Regras Canteiro B`, botões, `mqtt out` | Liga/desliga Solenóide B |
| 99 / 33 | Botões do dashboard | Liga/desliga Solenóide C |
| 99 / 38 | Botões do dashboard | Liga/desliga Bomba NFT |
| 99 / 39 | Botões do dashboard | Liga/desliga Bomba Oxi |
| 99 / 11, 12 | `Coletor Telemetria Canteiro B & Clima` | Clima do Motor de Regras |
| 99 / 21 | `Filtro Vazão e Volume` (casa por `type=34`) | Vazão e acúmulo de volume |
| 4 / 1, 11, 12 | `Filtro Clima SolarMini` | Gráficos de clima |
| 1 / 1–6 | `Filtro Umidade Solo Canteiro A` (`type=37`) | Gráficos de solo |
| 2 / 1–6 | `Filtro Canteiro B` + `Coletor Telemetria` | Gráficos + irrigação |
| qualquer / 254 | `Seletor de Ação` / `Intervalo (min)` | Configuração de intervalo de reporte (1–1440 min via `C_SET`/`V_VAR1`) |
| qualquer / 255 | `Filtro Bateria` | Gráfico de bateria |

Comando MQTT em formato nativo (`M360_NATIVE_MQTT=1`) — ligar a Solenóide A:

```
tópico:  m360/DF/0000/in/99/31/1/0/2
payload: 1
```

`command 1` = `C_SET` · `type 2` = `V_STATUS`.

### 8.1 Validação no gateway

O gateway **rejeita** o comando, em vez de repassar algo degradado, quando:

| Rejeição | Motivo |
|---|---|
| Tópico sem exatamente 8 barras | Truncado, ou níveis extras que a assinatura `in/#` também entrega |
| Segmento não numérico | `atoi("xx")` valia 0 e o comando virava `sensorId=0`, descartado pelo nó sem rastro |
| `command` fora de {1, 2, 3} | Só `C_SET`, `C_REQ` e `C_INTERNAL` fazem sentido do gateway para o nó |
| `nodeId`/`sensorId`/`type` fora de 0–255 | — |
| Payload maior que `MAX_PAYLOAD_SIZE` (25) | `MyMessage::set()` truncava em silêncio; nenhum comando legítimo chega perto |
| **`C_SET`/`V_STATUS` com payload diferente de `"0"` ou `"1"`** | O nó resolve com `getBool()`, que faz `atoi()`: `"true"`, `"ON"` ou lixo virariam **0** e **desligariam** o relé, indistinguíveis de um OFF legítimo |

Toda rejeição publica em `m360/{UF}/{CAR}/out/events`:

```json
{"event":"command_rejected","nodeId":99,"details":"payload de V_STATUS deve ser 0 ou 1"}
```

E quando o rádio falha no envio, o evento é `command_send_failed`. Antes, os dois
casos morriam num `println` no Serial e o Node-RED só percebia pelos ~35 s de
timeout do Sincronizador, sem nunca saber a causa. **Vale assinar esse tópico no
Node-RED** — é o diagnóstico mais direto para comando que "não faz nada".

> Após renumerar children e regravar o nó, disparar **REPRESENT** pelo dashboard:
> o nó `Mapeia nós` só limpa os children antigos do `mys_nodes` nessa mensagem,
> senão os IDs anteriores permanecem como fantasmas.

---

## 9. Escalas e unidades

- **Umidade de solo** (nós 1 e 2): `V_LEVEL` (37) em **ADC bruto 0–1023**, não em
  percentual. Valor alto = solo seco (sensor resistivo). O Motor de Regras do
  Node-RED opera nessa escala:
  - $< 350\text{ ADC}$: Solo saturado / Capacidade de campo (Standby).
  - $350\text{ a }850\text{ ADC}$: Escala contínua de déficit hídrico mapeada em lâmina de reposição líquida ($0\text{ a }3.5\text{ mm}$ com cap de $4.5\text{ mm}$).
  - $\ge 700\text{ ADC}$: Limiar de estresse hídrico crítico (sobrepõe adiamento por chuva).
- **Motor Agroclimático do Canteiro B (Open-Meteo + IoT):** Integração de $ET_0$ FAO-56 Penman-Monteith, VPD, radiação solar, vento e previsão de chuva com corte mínimo de duração $> 3\text{ minutos}$ ($180\text{ s}$) — ver documentação detalhada em [`docs/motor_agroclimatico_manejo360.md`](../../../docs/motor_agroclimatico_manejo360.md).
- **Temperatura** (`V_TEMP`): °C, 1 casa decimal.
- **Umidade do ar** (`V_HUM`): % relativa, 1 casa decimal.
- **Vazão** (nó 99, child 21): `V_FLOW` (34) em **L/min**. Fator do YF-S201:
  `F(Hz) = 7,5 × Q(L/min)`, ou 450 pulsos por litro.
- **Bateria** (child 255): `V_VOLTAGE` (38) em volts, 1 casa decimal.

---

## Apêndice A — Sub-projeto Kit Hélio

Rede separada, com gateway próprio (`d1_mini_kit_helio_gateway`, nó 0).

| Nó | Env | Child | Constante | Label | `S_*` | `V_*` | Payload | Pino | `reportIntervalMin` |
|---:|---|---:|---|---|---|---|---|---|:---:|
| **11** | `pro16MHz_miniDHT` | 11 | `CHILD_ID_DHT_TEMP` | `Temperatura Ar` | `S_TEMP` | `V_TEMP` (0) | float, 1 casa | `PIN_DHT` | 1 |
| | | 12 | `CHILD_ID_DHT_HUM` | `Umidade Relativa` | `S_HUM` | `V_HUM` (1) | float, 1 casa | `PIN_DHT` | 1 |

---

## Apêndice B — Nós removidos do projeto

| Nó | Diretório | Motivo | Recuperação |
|---:|---|---|---|
| **5** | `src/DRY/horta/nos/05nodeSolo3dNanoMux/` | Removido a pedido: duplicava funcionalmente os nós 1 e 2 (mesmos canteiros, mesmos rótulos `A_*`/`B_*`) e não tinha consumidor no Node-RED | Histórico do git |
| **13** | `src/DRY/nos/13nodeZTS_UmidadeHall/` | ZTS-3002 Modbus RS485 + sensor Hall; apagado em `d169901` sem migração para o layout monorepo | `git checkout 0ccf66a -- src/DRY/nos/13nodeZTS_UmidadeHall` |
| **80** | `src/DRY/nos/80nodeAqua/` | pH, EC, DS18B20, ultrassônico, 4× vazão; mesma causa | `git checkout 0ccf66a -- src/DRY/nos/80nodeAqua` |
