# Família de PCBs M360 Horta — Arquitetura

**Status:** arquitetura fechada; esquemáticos ainda não iniciados.
**Data:** 2026-09-10 · **Branch:** `feat/hardware-familia-pcb`
**Ferramentas:** KiCad 10.0.4 (esquemático e footprints) → EasyEDA (roteamento) → PCBA JLCPCB.

> As análises de rádio que fundamentam este documento estão na branch
> `feat/firmware-generico-autoconfig` (commit `6a9eb54`):
> `_bmad-output/implementation-artifacts/analise_radio_rfm95w_915.md` e as análises adversariais.

---

## 1. Visão geral

Quatro placas. Só uma tem RF.

| Placa | Função | Nós atendidos | Alimentação | Domínio lógico |
|---|---|---|---|---|
| **M360-RF** (mezanino) | RFM95W 915 MHz + desacoplamento + SMA | todas | 3,3 V do carrier | 3,3 V |
| **M360-GW** | Gateway ESP8266 (D1 mini) | 0 (Horta e Kit Hélio) | 5 V externo | 3,3 V |
| **M360-LP** | Nó sensor | 1, 2, 4, 11 | 3,0–6 V em `J_VIN` | 3,3 V |
| **M360-ACT** | Nó atuador | 99 | 12 V | 3,3 V (MCU) + 5 V (relés, MUX) |

**Por que um mezanino de rádio comum:** nada de RF atravessa o conector — a antena sai do próprio
mezanino. A seção de RF (50 Ω, keep-out, costura de terra, reservatório para o pico de TX) é
desenhada e validada uma vez; um módulo ruim troca-se sem dessoldar o carrier (lição de 20/08/2026,
em que a falha de enlace era o rádio do gateway). O header carrega DIO1 e RESET, que o soquete
2×4 do nRF24 não comporta, e mantém a opção LoRaWAN aberta nos nós.

---

## 2. Decisões de projeto

| # | Decisão | Consequência |
|---|---|---|
| 1 | MCU = **Pro Mini 3,3 V / 8 MHz**, módulo soldado | LED de power removido à mão; 3,3 V do carrier no pino VCC, RAW desligado |
| 2 | Rádio em **mezanino comum** | Header 2×5 (§3) |
| 3 | Atuador com **MCU a 3,3 V** + 74HCT244 | Conversão de nível só nos sinais DC dos relés, fora do SPI |
| 4 | Carga solar por **módulo TP4056 + DW01 externo** | A LP não tem carregador; recebe a saída do módulo em `J_VIN` |
| 5 | **74HC4051** na LP | 8 canais de solo num pino analógico; I2C livre |
| 6 | **RJ45** dentro da caixa; par marrom = VCC/GND | Pinagem em §7; TVS em todas as linhas de campo |
| 7 | Nós 01/02 em **fonte fixa** | Mesma LP; 5 V em `J_VIN` |
| 8 | Placas novas a **8 MHz** | Envs novos `pro8MHzatmega328`; os envs atuais servem o hardware atual |
| 9 | **SMA de borda** no mezanino | Mezanino na borda do carrier; ver §8 |
| 10 | RS485 fora | — |
| 11 | KiCad 10 → EasyEDA | Símbolos e footprints com código LCSC |
| 12 | **PCBA** (placas montadas) | SMD do LCSC; Pro Mini, mezanino, RJ45 e SMA soldados à mão |
| 13 | Repareamento por **reed switch** | 1 pino por nó; na LP, em INT1, acorda o nó dormindo |

Frequência LoRa: provisionada por software na instalação (sem chave DIP) — ver
`analise_radio_rfm95w_915.md` §5 e a decisão de um gateway por propriedade.

---

## 3. M360-RF — mezanino de rádio

| Bloco | Conteúdo |
|---|---|
| Rádio | RFM95W-915S2. Footprint **stock do KiCad** `RF_Module:HOPERF_RFM9XW_SMD` (confere com o contorno do datasheet p. 120; pads avançam 1 mm além da borda, bom para solda manual). Símbolo `RF_Module:RFM95W-915S2` |
| Desacoplamento | 100 nF + 22 µF cerâmico nos pinos 13 (3.3V) e 10 (GND). O datasheet pede só 100 nF; o reservatório é para o pico de 87–120 mA sobre bateria e LDO |
| Antena | Trilha de RF curta do pino 9 (ANA) até um **SMA fêmea de borda**. A trilha é bem menor que λ/20 em 915 MHz, mas é desenhada como coplanar aterrada no stackup do JLC |
| Terra | Plano sólido, costura de vias em volta da trilha de RF e sob o módulo |
| Interface | Header 2×5, 2,54 mm |

**Header 2×5 (vista do mezanino):**

| Pino | Sinal | Pino | Sinal |
|---:|---|---:|---|
| 1 | 3V3 | 2 | GND |
| 3 | SCK | 4 | GND |
| 5 | MOSI | 6 | MISO |
| 7 | NSS | 8 | DIO0 |
| 9 | DIO1 | 10 | RESET |

> ⛔ **Sem pull-up no NSS dentro do mezanino.** No gateway o NSS vai ao GPIO15, que tem de estar
> em LOW no boot do ESP8266. Quando o carrier precisa de pull-up no NSS (nós AVR, para o rádio não
> ser selecionado durante a gravação), ele fica no carrier.

---

## 4. M360-LP — nó sensor

### 4.1 Alimentação

```
Painel ─►|─ [módulo TP4056+DW01] ─ OUT+/OUT− ─┐
   (externo, fora desta placa)                ├─► J_VIN (3,0–6 V) ─► LDO 3,3 V ─► Pro Mini VCC + mezanino
Fonte 5 V (variante fixa, Nós 01/02) ─────────┘              │
                                                              └─► 2 MΩ / 1 MΩ + 100 nF ─► A6
```

| Item | Especificação | Motivo |
|---|---|---|
| LDO 3,3 V | Iq ≤ 5 µA; dropout ≤ ~150 mV @ 100 mA; Vin máx ≥ 6 V. Candidato: MCP1700-3302 (confirmar estoque no JLC) | Sono do nó; rádio a +17 dBm com a bateria em 3,4 V; fonte de 5 V na variante fixa |
| Divisor de bateria | **2 MΩ / 1 MΩ permanente, 100 nF no A6** | A6 nunca passa de 2 V, nem com 6 V na entrada. Drena ~1,4 µA com a bateria a 4,2 V. O capacitor fornece a carga do sample-and-hold (a fonte de 667 kΩ sozinha seria alta demais para o ADC). Um divisor comutado pela perna de baixo deixaria o A6 na tensão da bateria — acima do VCC — quando desligado |
| Bateria no VCC? | **Não** | O Nó 04 atual (nRF24) roda o MCU direto na bateria; com RFM95W isso não pode: o SPI do MCU chegaria ao rádio a 4,2 V, e o máximo absoluto do SX1276 é 3,9 V |

A leitura de bateria pelo bandgap interno (padrão de `M360Power.cpp`) **não serve** nesta placa: com
VCC regulado ela mede 3,3 V. O nó sobrescreve `readBatteryVoltage()` lendo o A6.

### 4.2 Blocos

| Bloco | Conteúdo |
|---|---|
| MCU | Pro Mini 3,3 V / 8 MHz, LED de power removido |
| Trilho chaveado (VSW) | P-MOSFET high-side no 3,3 V, gate com pull-up (desligado no boot), comandado por D4. Alimenta sensores, pull-ups e o par marrom das portas |
| Multiplexador | 74HC4051 no VSW. **Regra de firmware:** levar S0–S2 a LOW antes de cortar o VSW, senão os pinos alimentam o CI pelos diodos de proteção |
| Sondas de solo | Pull-up de 10 kΩ para o VSW por canal (sonda resistiva, como os Nós 01/02); **DNI** para sonda capacitiva. RC 1 kΩ / 100 nF por canal |
| Digitais de campo | 1-Wire com 4,7 kΩ para o VSW; DHT com 10 kΩ para o VSW; I2C com 3,3 kΩ para o VSW |
| Reed switch | D3 (INT1) com pull-up interno; contato aberto em repouso, zero corrente no sono |
| Proteção | TVS em todas as linhas de campo e no VSW das portas |
| Programação | Header FTDI do próprio Pro Mini |

### 4.3 Pinos (Pro Mini)

| Pino | Função | Pino | Função |
|---|---|---|---|
| D2 | DIO0 (INT0) | A0 | saída do 74HC4051 |
| D3 | reed switch (INT1) | A1 | 1-Wire |
| D4 | habilita VSW | A2 | DHT |
| D5 / D6 / D7 | S0 / S1 / S2 do 4051 | A3 | livre |
| D8 | RESET do rádio | A4 / A5 | SDA / SCL |
| D9 | DIO1 | A6 | bateria (divisor) |
| D10–D13 | NSS / MOSI / MISO / SCK | **A7** | **livre — semente da assinatura** |

> ⛔ **A7 sem nada ligado.** Com a assinatura MySensors ligada,
> `MY_SIGNING_SOFT_RANDOMSEED_PIN=A7` lê ruído dele. O padrão da lib (7) faria `pinMode` no **D7**,
> que é o S2 do 4051 (`MyHwAVR.cpp:245`).

---

## 5. M360-ACT — nó atuador

### 5.1 Alimentação

12 V (borne) → buck 5 V (MUX, 74HCT244, opto dos relés, sensores) → LDO 3,3 V (Pro Mini e mezanino).
O regulador do próprio Pro Mini (~150 mA) não sustenta o pico do rádio somado ao resto.

### 5.2 Blocos

| Bloco | Conteúdo |
|---|---|
| MCU | Pro Mini 3,3 V / 8 MHz |
| Fronteira 3,3 → 5 V | **74HCT244** (VIH 2,0 V aceita 3,3 V direto): S0–S3, SIG, NFT, OXI. **/OE com pull-up para 5 V**, liberado pelo A5 depois do boot |
| MUX | CD74HC4067 a 5 V (EN no GND), 7 canais em uso e 9 de reserva |
| Vazão | Entrada D3 com divisor 10 kΩ / 20 kΩ + 1 nF + TVS |
| Reed switch | A4 (o nó nunca dorme; lido por polling) |

**Por que o /OE:** enquanto o Pro Mini inicia, os pinos dele ficam soltos. Sem o /OE, as saídas do
74HCT244 ficariam indefinidas e um relé poderia bater na partida. Com o /OE em alto, as saídas ficam
em alta impedância, a entrada do módulo de relé fica solta — e **entrada solta = relé desligado**,
a mesma regra de segurança do Nó 99 atual.

**YF-S201:** o datasheet do fabricante especifica nível alto > 4,5 V com 5 V de alimentação e borda de
0,04 µs — o sensor põe ~5 V no fio. O divisor leva isso a 3,3 V; os pulsos (≤ ~225 Hz a 30 L/min) passam
folgados pelo filtro de 1 nF.

### 5.3 Módulos de relé

Módulo real do quadro (`docs/modulo rele.jpeg`): 2 canais SRD-05VDC (bobina ~72 mA), optoacoplador
PC817, jumper JD-VCC/VCC/GND, entrada ativa em LOW.

| Ligação | Regra |
|---|---|
| Bobinas | **JD-VCC no 5 V do quadro**, jumper JD-VCC–VCC removido. A corrente das bobinas não passa pelo RJ45 |
| Opto (VCC do módulo) | Pelo **par marrom** do RJ45, ~2 mA por canal |
| ⛔ Mesmo 5 V | O VCC do opto tem de ser **o mesmo 5 V que alimenta o 74HCT244**. Com outro 5 V, o nível alto do buffer poderia não apagar o LED do opto e o relé ficaria preso ligado |

### 5.4 Pinos (Pro Mini)

| Pino | Função | Pino | Função |
|---|---|---|---|
| D2 | DIO0 (INT0) | A0 | Bomba NFT (via 74HCT244) |
| D3 | vazão (INT1) | A1 | Bomba Oxi (via 74HCT244) |
| D4–D7 | S0–S3 do MUX (via 74HCT244) | A2 | DIO1 |
| D8 | SIG do MUX (via 74HCT244) | A3 | RESET do rádio |
| D9 | DHT | A4 | reed switch |
| D10–D13 | NSS / MOSI / MISO / SCK | A5 | /OE do 74HCT244 |
| | | A6 | livre (analógico) |
| | | **A7** | **livre — semente da assinatura** |

---

## 6. M360-GW — gateway

| Sinal | Pino do D1 mini | Observação |
|---|---|---|
| SCK / MOSI / MISO | D5 / D7 / D6 | SPI de hardware |
| NSS | D8 (GPIO15) | O D1 mini tem pull-down; o mezanino não pode ter pull-up (§3) |
| DIO0 | D1 (GPIO5) | Padrão da lib (`RFM95.h:96`) |
| DIO1, RESET | **não ligados** | Não há GPIO livre seguro no boot (GPIO0/2/15 são pinos de boot). Um gateway LoRaWAN de verdade usa concentrador SX1302, não RFM95 — a opção LoRaWAN só importa nos nós |
| LED vermelho / amarelo | D2 / D0 | Sem mudança |
| LED verde | **D4 (GPIO2), ativo em LOW** | Sai do D1, que agora é o DIO0. GPIO2 é pino de boot: o LED liga do 3,3 V ao pino, para não puxá-lo para baixo na partida. **Exige inverter a polaridade em `M360Leds.cpp`** |
| Botão AP / reset | A0 | Sem mudança |

---

## 7. Portas RJ45 (chicote PCB → sensores e atuadores)

Pares T568B: laranja 1/2, verde 3/6, azul 4/5, **marrom 7/8 = VCC/GND** em todas as portas.

**M360-LP** — VCC = 3,3 V chaveado (VSW):

| Porta | Laranja 1/2 | Verde 3/6 | Azul 4/5 |
|---|---|---|---|
| **J1 SOLO** | 4051 ch0 / ch1 — estaca de 1 m (10 / 30 cm) | ch2 / ch3 — estaca de 3 m | ch4 / ch5 — estaca de 5 m |
| **J2 CLIMA/AUX** | 1-Wire (DS18B20) / DHT | SDA / SCL | 4051 ch6 / ch7 (reserva) |

**M360-ACT** — VCC = 5 V:

| Porta | Laranja 1/2 | Verde 3/6 | Azul 4/5 |
|---|---|---|---|
| **J3 RELÉS-A** | Sol. A (ch0) / Sol. B (ch1) | Sol. C (ch2) / Perist. A (ch3) | Perist. B (ch4) / pH+ (ch5) |
| **J4 RELÉS-B** | pH− (ch6) / Bomba NFT | Bomba Oxi / MUX ch7 | MUX ch8 / ch9 (reserva) |
| **J5 SENSORES** | Vazão / DHT | A6 (analógico) / livre | livres |

Todas as linhas de sinal levam TVS no conector. SDA e SCL compartilham um par: aceitável até ~2 m a
100 kHz; acima disso, reavaliar.

---

## 8. Montagem e fabricação

| Item | Como entra |
|---|---|
| SMD (LDO, MOSFETs, 74HC4051, 74HCT244, CD74HC4067, buck, passivos, TVS) | PCBA JLCPCB, com peças escolhidas pela disponibilidade no LCSC |
| Pro Mini, mezanino RF, RJ45, SMA, bornes | Solda manual após a PCBA |
| Antena | SMA de borda no mezanino: ou o conector atravessa a parede da caixa (furo e vedação), ou a antena fica dentro dela — funciona em 915 MHz se a caixa for plástica |

---

## 9. Pendências

| Item | O que falta |
|---|---|
| Banda do RFM95W | Validar em bancada (RTL-SDR em 916,8 MHz ou RSSI contra referência) antes de comprar em quantidade |
| Seleção de peças | LDO, P-MOSFET do VSW, TVS, buck, SMA de borda — pelo estoque no JLC |
| DHT11 no atuador | Alimentado a 5 V pelo RJ45, com pull-up para 3,3 V: confirmar que a saída é dreno aberto |
| Firmware das placas novas | Envs `pro8MHzatmega328`; pinos novos; `readBatteryVoltage()` pelo A6 na LP; LED verde ativo em LOW no gateway. Atualizar `inventario.md` e os `esquema_eletrico.md` na mesma entrega |
| Caixa | Modelo IP e dimensões internas; posição do SMA |

---

## 10. Ordem de trabalho

1. **M360-RF** — a menor placa e a única comum às outras três.
2. **M360-LP** — cobre quatro nós.
3. **M360-ACT**.
4. **M360-GW** — o mais simples: D1 mini + mezanino + LEDs.
