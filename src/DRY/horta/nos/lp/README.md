# Nós na placa M360-LP — firmware comum, configuração por nó

Um único fonte (`lpNode.cpp` + `lpDrivers.cpp` + `drivers/`) serve todos os nós montados na
**M360-LP** (Pro Mini 3,3 V / 8 MHz, `hardware/m360-lp/`). O que muda entre nós fica em
`config/<nó>/nodeConfig.h`, escolhido pelo `-I` do env em `src/DRY/horta/platformio.ini`.

Por que assim, e não um `.cpp` por nó nem um binário genérico configurado em campo:
ver a decisão em `hardware/ARQUITETURA_FAMILIA.md` §2 (decisão 8) e o workflow
`.agent/workflows/m360-node-factory.md`, Passo 0.

## Arquivos

| Arquivo | Papel |
|---|---|
| `lpPins.h` | Pinagem fixa da placa e códigos `LP_SRC_*` usados no campo `pin` dos itens |
| `lpDrivers.h/.cpp` | Interface e orquestração: VSW, ordem de ligar/desligar, despacho das leituras |
| `drivers/soilMux.cpp` | 74HC4051, 8 sondas resistivas em A0 (`LP_HAS_SOIL`) |
| `drivers/dht.cpp` | DHT11/22 em A2 (`LP_HAS_DHT`, tipo por `LP_DHT_TYPE`) |
| `drivers/onewire.cpp` | DS18B20 em A1 (`LP_HAS_ONEWIRE`) |
| `drivers/battery.cpp` | Bateria pelo A6 com correção por bandgap (`LP_POWER_BATTERY`) |
| `lpNode.cpp` | Nó declarativo: hooks MySensors, `M360Node`, warm-up e corte de bateria |
| `config/node01/nodeConfig.h` | Nó 1, Canteiro A — fonte fixa, 6 sondas |
| `config/node02/nodeConfig.h` | Nó 2, Canteiro B — fonte fixa, 6 sondas |

Cada driver é um arquivo inteiro atrás de `#if LP_HAS_*`: sem a flag, o driver vira uma unidade
vazia e a biblioteca dele, embora compilada pelo PlatformIO, não é linkada. Medido: `lp_01`
(só solo) ocupa 20 502 B de Flash e 657 B de RAM, contra 20 556 / 662 do mesmo nó no Nano.

## Pinagem que o firmware assume (netlist de 2026-10-14)

| Pino | Função | Regra |
|---|---|---|
| D4 | `VSW_EN` — liga o 3,3 V dos sensores | **LOW liga, HIGH desliga** (P-MOSFET high-side) |
| D5 / D6 / D7 | S0 / S1 / S2 do 74HC4051 | Em LOW antes de cortar o VSW |
| A0 | Saída do 74HC4051 | Nunca amostrar durante SPI com o rádio |
| A1 | 1-Wire (DS18B20) | `INPUT` puro com o VSW desligado |
| A2 | DHT | `INPUT` puro com o VSW desligado |
| A4 / A5 | SDA / SCL | Sem driver ainda; TWI desligado |
| A6 | Bateria, divisor 2 MΩ / 1 MΩ | VBAT = 3 × V(A6), VCC medido pelo bandgap |
| D3 | Reed switch para GND (INT1) | Nunca configurar como saída |
| D2 / D9 / D10 / D8 | DIO0 / DIO1 / NSS / RESET do RFM95W | Flags no env; nRF24 legado: CE=D2, CSN=D10 |

Pull-ups, RC e TVS das sondas estão na placa. A escala do ADC (0–1023, alto = seco) é a
mesma dos Nós 01/02 no Nano, porque pull-up e referência do ADC são o mesmo trilho.

## Como criar um nó novo nesta placa

O procedimento oficial é o workflow
[`.agent/workflows/m360-node-factory.md`](../../../../../.agent/workflows/m360-node-factory.md):
no **Passo 0** escolher o **caminho A (placa M360-LP)**, elicitar só o que varia (1.1 identificação,
1.3 alimentação, 1.7 sensores e child IDs, 1.8 warm-up se for a bateria) e fechar com o checklist
do Passo 8, itens 18 a 20. O workflow traz o modelo completo do `nodeConfig.h` e do env. Os
passos abaixo são o resumo do que ele produz:

1. Copiar `config/node01/` para `config/nodeXX/` e editar `nodeConfig.h`: nome, versão,
   `LP_POWER_FIXED_5V` **ou** `LP_POWER_BATTERY`, flags `LP_HAS_*`, rótulos PROGMEM e
   `NODE_ITEMS[]` com `pin` = `LP_SRC_SOIL(ch)`, `LP_SRC_DHT_TEMP`, `LP_SRC_DHT_HUM` ou
   `LP_SRC_DS18B20`.
2. Acrescentar `[env:lp_XX]` em `src/DRY/horta/platformio.ini` estendendo `lp_base`, com
   `MY_NODE_ID` e `-Isrc/DRY/horta/nos/lp/config/nodeXX`.
3. Atualizar `src/DRY/horta/inventario.md` (§1 e seção do nó) na mesma entrega.
4. `pio run -e lp_XX`.

I2C: a placa tem SDA/SCL com pull-ups para o VSW, mas nenhum sensor I2C foi escolhido ainda.
Quando houver, o driver entra em `drivers/` atrás de `LP_HAS_I2C_<sensor>`, com `Wire.end()`
e pinos em `INPUT` no `powerDown`, como os outros.
