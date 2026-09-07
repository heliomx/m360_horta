# SU-xxT — Sensores de Solo e Lisímetro Cerâmico 🌱

Biblioteca para a família **SU-xxT** (SU-10, SU-10T, SU-20, SU-20T, SU-30, SU-30T):
umidade de solo em duas profundidades, temperatura da rizosfera, e pH / EC / nível
da solução extraída por um lisímetro cerâmico.

Adota o padrão de API da **`DallasTemperature` / `OneWire`** — disparo global de
conversão, acesso por índice, constantes de erro — orquestrando sob o capô o ADC
I2C **ADS1115**, o multiplexador **74HC4051**, o barramento 1-Wire do **DS18B20**,
a excitação AC das células de EC e nível, e as compensações agronômicas.

> [!IMPORTANT]
> **Status: implementada, nunca executada em hardware.**
> O código existe e compila nas três plataformas declaradas. **Nenhuma leitura
> real foi feita** — os defaults de calibração são valores teóricos de partida,
> não medições, e nenhum tempo de acomodação foi caracterizado. Tudo que está
> marcado *a definir por medição* segue por definir.
>
> O hardware está especificado em [`hardware/SU-xxT/README.md`](../../hardware/SU-xxT/README.md).

---

## 💡 A Analogia: "O Laboratório Enterrado"

Um sensor de umidade comum mede o solo. A família SU mede também **a água que o
solo entrega à planta** — e são coisas diferentes.

Uma vela cerâmica microporosa enterrada a 30–40 cm funciona como raiz artificial:
suga a solução do solo por diferença de potencial matricial e a acumula numa
câmara, onde eletrodos medem pH e condutividade sem o atrito de pedras e areia.

Disso decorre quase tudo que é peculiar nesta biblioteca:

- **A câmara pode estar vazia.** Solo seco não entrega solução. pH e EC passam a
  não existir — não a valer zero.
- **Encher não é estar pronto.** A lâmina nova precisa deslocar a reserva antiga
  antes que a medição valha.
- **O nível é dado agronômico.** Câmara vazia significa solo abaixo da faixa de
  extração da cerâmica — indicador de déficit, não só telemetria interna.

---

## 🧬 A Família

| Modelo | Umidade 10/30 cm | EC | pH | Temp. solo | Nível |
|:---|:---:|:---:|:---:|:---:|:---:|
| **SU-10** | ✅ | — | — | — | — |
| **SU-10T** | ✅ | — | — | ✅ | — |
| **SU-20** | ✅ | ⚠️ | — | — | ✅ |
| **SU-20T** | ✅ | ✅ | — | ✅ | ✅ |
| **SU-30** | ✅ | ⚠️ | ✅ | — | ✅ |
| **SU-30T** | ✅ | ✅ | ✅ | ✅ | ✅ |

O sufixo **'T'** indica a presença do DS18B20.

> ⚠️ **EC sem termometria é indicativa, não quantitativa.** A condutividade tem
> coeficiente térmico de 1,91 %/°C. Sem `T_solo` real a compensação cai numa
> temperatura assumida, o que dá **~19 % de erro a cada 10 °C de desvio** — e solo
> de campo varia bem mais que isso ao longo do ano. O pH tolera melhor a mesma
> aproximação (~0,10 pH). **Quem dosa fertirrigação pela EC precisa de modelo 'T'.**

---

## 🏗️ Arquitetura de Desacoplamento

A `SU-xxT` é **camada física**. Ela não conhece MySensors, não conhece child IDs e
não conhece o Node-RED — entrega grandeza, rótulo e unidade.

```
noSU30T.cpp          Declarativo: NODE_ITEMS[], IDX_MAP[], hooks MySensors
      |                (é aqui que vivem os child IDs)
      v
sensorDrivers.cpp    Instancia SU_Device, implementa onRead/onWrite
      |
      v
lib/SU-xxT           Física: ADS1115, 74HC4051, 1-Wire, excitação AC, compensações
```

O contrato MySensors — child IDs, tipos `S_*` / `V_*` — pertence ao
`NODE_ITEMS[]` do nó, e sua fonte única é
[`src/DRY/horta/inventario.md`](../../src/DRY/horta/inventario.md). Um `childId`
"sugerido" dentro do driver criaria uma segunda fonte de verdade, e a divergência
seria silenciosa: `M360Node::handleMessage()` casa `childId` exato e ignora IDs
desconhecidos, sem erro de compilação e sem log.

---

## 🚀 Como Começar (Guia Rápido)

### 1. Instancie o dispositivo

```cpp
#include <SU_xxT.h>

static SU_Device su(/* MUX A/B/C */ 5, 6, 7,
                    /* excitação AC */ 3,
                    /* MOSFET front-ends */ 4,
                    /* 1-Wire */ 8,
                    SU_MODEL_30T);
```

### 2. Aquisição no `powerUp()`, leitura no `onRead()`

A `M360Node` chama o callback de leitura **um item por vez** e não tem gancho
"antes de todas as leituras". O único ponto pré-leitura do ciclo é
`M360::powerUp()` — é lá que a aquisição em lote acontece:

```cpp
namespace M360 {
    void powerUp()   { su.powerUp(); su.requestReadings(); }
    void powerDown() { su.powerDown(); }
}

static float readNodeItem(uint8_t i) { return su.getReadingByIndex(IDX_MAP[i]); }
```

> ⚠️ Sobrescreva **dentro do `namespace M360`**. Como função global, o linker trata
> os símbolos como distintos, a implementação vazia da lib continua valendo e o nó
> publica cache frio — sem erro de compilação.

### 3. Escolha o perfil de energia certo

| Perfil | Suportado | Por quê |
|---|:---:|---|
| `M360_LOW_POWER` | ✅ | Perfil de referência |
| `M360_PASSIVE` | ✅ | `powerUp()` roda no `FORCE_UPDATE` |
| `M360_ALWAYS_ON` | ❌ | A `M360Node` **nunca** chama `powerUp()` nesse perfil — `requestReadings()` não rodaria e o nó publicaria cache frio |

---

## 🚦 Valores Sentinela

Toda leitura inválida devolve uma sentinela **abaixo de −32767**, que é o piso que
`M360Node::_readAndSendAll()` descarta. O resultado esperado de uma falha é
**ausência de publicação**, não um valor estranho no dashboard.

| Constante | Significado |
|---|---|
| `SU_ERR_LEVEL_LOW` | Câmara seca — pH e EC não existem no momento |
| `SU_ERR_STALE_RECHARGE` | Câmara cheia, mas a reserva antiga ainda não foi trocada |
| `SU_ERR_ADC_FAULT` | Falha do ADS1115, ou índice fora de faixa |
| `SU_ERR_NOT_SAMPLED` | Cache frio: nenhum `requestReadings()` desde o `powerUp()` |
| `SU_DEVICE_DISCONNECTED` | DS18B20 ausente ou mudo |

Teste com `SU_isError(v)` — nunca compare float por igualdade.

---

## 🔬 Calibração

Persistida na EEPROM em **768–831**, dentro da região de aplicação
(`M360_EEPROM_APP_BASE`) declarada no mapa unificado de
[`M360Config.h`](../M360-DRY/src/M360Config.h). Protegida por magic, versão de
layout e CRC: layout novo lido como antigo produziria pH e EC plausíveis e
errados, que é o pior modo de falha possível aqui.

> ⚠️ **Ao calibrar o pH, registre a temperatura da solução** (`calibTempC`). O
> slope medido em campo já embute a temperatura da calibração; multiplicá-lo pelo
> fator de Nernst cru é dupla compensação. Ver [ARCHITECTURE.md](ARCHITECTURE.md).

> ⚠️ **A calibração não sobrevive a um OTA que reescreva a EEPROM inteira.** São
> pontos medidos com solução tampão — só se recuperam com nova visita ao campo.

---

## 📏 Custo medido

Compilação do exemplo `BasicReadings` — biblioteca + `Adafruit ADS1X15` +
`BusIO` + `OneWire` + `DallasTemperature`, **sem** MySensors, RF24 nem
`M360Node`:

| Alvo | RAM | Flash |
|---|---|---|
| `nanoatmega328` | 784 B / 2048 (38,3 %) | 14.644 B / 30.720 (47,7 %) |
| `d1_mini` (ESP8266) | 28.076 B (34,3 %) | 277.320 B (26,6 %) |
| `esp32dev` | 20.616 B (6,3 %) | 345.616 B (26,4 %) |

> **Isto não é o orçamento do nó.** O teto que importa — 1400 B de RAM e 27.600 B
> de flash no ATmega328P — só pode ser aferido no binário do nó real, com
> MySensors e RF24 linkados junto. Essa medição, como o tempo acordado por ciclo,
> pertence à construção do nó (workflow `m360-node-factory`), não a esta
> biblioteca.
>
> O número acima serve como **linha de base**: 784 B já comprometidos deixam
> ~616 B para MySensors, RF24 e o motor do nó, dentro do teto de 1400 B. É
> apertado, e é o dado que justifica medir cedo. A escada de contenção está em
> [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 📚 Documentação Detalhada

- [**Arquitetura e decisões de projeto**](ARCHITECTURE.md) — o porquê de cada
  escolha, e as armadilhas que elas evitam
- [**Referência da API**](API_REFERENCE.md) — assinaturas, structs e contratos
- [**Especificação de hardware**](../../hardware/SU-xxT/README.md) — PCB, lisímetro,
  pinagem e mapeamento MySensors
