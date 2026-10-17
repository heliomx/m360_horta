# Nós de campo da M360 Horta

Firmware dos nós sensores e atuadores, todos sobre a biblioteca
**[M360-DRY](../../../../lib/M360-DRY/)** (API em
[`API_REFERENCE.md`](../../../../lib/M360-DRY/API_REFERENCE.md)). O contrato de child IDs com o
Node-RED é o [`inventario.md`](../inventario.md).

## Nós existentes

| Pasta | Nó | Hardware | Envs |
|---|---:|---|---|
| `lp/` | 1, 2 | Placa **M360-LP** (Pro Mini 3,3 V / 8 MHz): firmware comum + `config/<nó>/nodeConfig.h` | `lp_01`, `lp_02` |
| `01nodeSolo3dNano/` | 1 | Nano 5 V, 6 eletrodos em A0–A5 (legado, em campo) | `nano_01nodeSolo3d`, `ProMini_01nodeSolo3d` |
| `02nodeSolo3dNano/` | 2 | Idem, Canteiro B | `nano_02nodeSolo3d` |
| `04noodeSolarMini/` | 4 | Pro Mini 16 MHz, DHT11 + DS18B20, solar | `ProMini_04noodeSolarMini` |
| `99nodeReles/` | 99 | Nano 5 V (com DHT11) **ou** placa **M360-ACT** (sem DHT11, flag `NODE99_BOARD_ACT`) | `nano_99reles`, `nano_99reles_rep`, `act_99`, `act_99_rep` |
| `01nodeSolo3dMini/` | — | Só documentação, defasada (projeto de 18 canais que não existe) | — |

Os nós 5, 13 e 80 foram removidos (inventário, Apêndice B).

---

## Como criar um nó

Seguir o workflow [`m360-node-factory`](../../../../.agent/workflows/m360-node-factory.md). O
Passo 0 decide o caminho:

| Caminho | Quando | O que se cria |
|---|---|---|
| **A — placa M360-LP** | Nó sensor da família: solo pelo 74HC4051, DS18B20, DHT, fonte 5 V ou bateria/solar | `lp/config/<nó>/nodeConfig.h` + `[env:lp_XX]` em `../platformio.ini`. Nenhum `.cpp` |
| **B — hardware próprio** | Qualquer outro caso | Pasta `<nomeNó>/` com `.cpp` declarativo, `sensorDrivers.h/.cpp`, `esquema_eletrico.md` e `diagrama_blocos.svg` |

Regras que não se negociam, nos dois caminhos: macros `MY_*` só no `platformio.ini`; ordem de
includes `Arduino.h` → `MySensors.h` → `M360.h` → driver; `Serial.begin()` e `initSensors()` em
`before()`; rótulos em `PROGMEM` com `M360_LABEL()`; `inventario.md` atualizado na mesma entrega.

### Estrutura de um nó em hardware próprio (caminho B)

```text
nomeDoNo/
├── nomeDoNo.cpp          nó declarativo: NODE_ITEMS[], M360Node, 5 hooks MySensors
├── sensorDrivers.h/.cpp  camada física (ADC, I2C, 1-Wire, relés)
├── esquema_eletrico.md   pinagem e ligações
└── diagrama_blocos.svg   diagrama do hardware
```

O subdiretório `withLibDRY/` de versões anteriores não existe mais: o sketch fica direto na pasta.

---

## Perfis de energia (`M360PowerProfile`)

| Perfil | Comportamento | Uso |
|---|---|---|
| `M360_ALWAYS_ON` | Rádio em escuta contínua, timer por `millis()`, nunca dorme. Resposta a comando imediata | Fonte fixa (5 V do quadro): Nós 1, 2, 99 |
| `M360_LOW_POWER` | **Proativo.** `smartSleep()` pelo intervalo (EEPROM, child 254); acorda, lê, envia, dorme. Warm-up obrigatório após o boot (R13 do workflow) | Bateria/solar: Nó 4; nós LP com `LP_POWER_BATTERY` |
| `M360_PASSIVE` | **Reativo.** Dorme sempre; só lê e envia sob `FORCE_UPDATE` do gateway | Sensores de alto consumo (Modbus) a bateria |
| `M360_REPEATER` | Igual ao `ALWAYS_ON`, com `MY_REPEATER_FEATURE` no env | Nó 99 em posição central da rede |

Um nó declara exatamente um perfil. Na M360-LP ele decorre da alimentação escolhida no
`nodeConfig.h`: `LP_POWER_FIXED_5V` → `ALWAYS_ON`, `LP_POWER_BATTERY` → `LOW_POWER` com leitura
de bateria pelo A6 e corte de transmissão abaixo de `LP_VBAT_CUTOFF`.

### Custo do modo `PASSIVE` em bateria

O `smartSleep()` do MySensors abre uma janela de escuta de 500 ms a cada acordada. Com nRF24L01+
em Pro Mini 3,3 V, a escuta consome ~18 mA, ou seja ~0,0025 mAh por acordada. Para uma célula de
2000 mAh:

| Intervalo | Latência máxima do comando | Autonomia estimada |
|---|---|---|
| 10 s | 10 s | ~3 meses |
| 1 min | 60 s | ~1,5 ano |
| 10 min | 10 min | mais de 10 anos |

Com RFM95W os números mudam (RX ~10–12 mA, TX a 17 dBm ~120 mA); medir antes de dimensionar.
