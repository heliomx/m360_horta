# PCB — Gateway MQTT M360 (rev A)

Placa-mãe (*carrier*) para o **Gateway Manejo360**, derivada diretamente do firmware
[`src/DRY/gateway/libDryGatewayMqtt.cpp`](../../../src/DRY/gateway/libDryGatewayMqtt.cpp)
e das definições em [`lib/M360-DRY/src/M360Leds.h`](../../../lib/M360-DRY/src/M360Leds.h)
e no `platformio.ini` (env `d1_mini_gateway`).

O ESP8266 e o rádio **não** são soldados: ambos entram em soquetes fêmea, permitindo
troca em campo sem ferro de solda.

| Arquivo | Conteúdo |
|---|---|
| `m360_gateway.kicad_pro` / `.kicad_sch` / `.kicad_pcb` | Projeto KiCad (formato nativo S-expression) |
| `m360_gateway_esquematico.pdf` | Esquemático em PDF |
| `m360_gateway_bom.csv` | Lista de materiais |

**Estado:** ERC 0 violações · DRC 0 erros (ver *Pendências* no fim).

---

## Requisitos extraídos do firmware

| Sinal | Pino D1 mini | GPIO | Origem no código |
|---|---|---|---|
| RF24 CE | D4 | 2 | `platformio.ini` → `-D MY_RF24_CE_PIN=2` |
| RF24 CSN | D8 | 15 | `platformio.ini` → `-D MY_RF24_CSN_PIN=15` |
| RF24 SCK / MISO / MOSI | D5 / D6 / D7 | 14 / 12 / 13 | HW SPI do ESP8266 (MySensors) |
| LED vermelho (ERRO) | D2 | 4 | `M360Leds.h` → `LED_RED` |
| LED verde (OK) | D1 | 5 | `M360Leds.h` → `LED_GREEN` |
| LED amarelo (TX/AP) | D0 | 16 | `M360Leds.h` → `LED_YELLOW` |
| Entrada de configuração | A0 | ADC | `M360Leds.h` → `RESET_PIN`; `isA0Low()` em `libDryGatewayMqtt.cpp:37` |

Os LEDs são **ativos em nível alto** (`digitalWrite(pin, HIGH)` acende — ver
`M360Leds.cpp:47`), logo o ânodo vai ao GPIO e o cátodo ao GND via resistor.

---

## Componentes

| Ref | Valor | Footprint | Função |
|---|---|---|---|
| **U1** | WEMOS D1 mini | `RF_Module:WEMOS_D1_mini_light` | ESP8266 — soldar **duas barras fêmea 1x8** no lugar do módulo |
| **U2** | nRF24L01+ | `PinSocket_2x04_P2.54mm_Vertical` | Soquete do rádio |
| **U3** | AMS1117-3.3 | SOT-223 | LDO **dedicado ao rádio** |
| **J1** | 5V IN | Bornier 1x02 P5.00 | Alimentação 5 V |
| **J2** | EXPANSÃO | Pin header 1x06 | 5V · 3V3 · GND · TX · RX · D3 |
| **J3** | IRQ SEL | Pin header 1x02 | Jumper opcional IRQ(nRF24) → D3 |
| **F1** | PTC 500 mA | Fuse Bourns MF-RG500 | Proteção de sobrecorrente |
| **D1** | 1N5819 | DO-41 | Proteção contra inversão de polaridade |
| **D2 / D4 / D3** | LED verm. / verde / amar. | LED 3 mm | ERRO / OK / TX-AP |
| **R1 / R3 / R2** | 470 Ω | Axial 1/4 W | Limitação dos LEDs |
| **R4** | 10 kΩ | Axial 1/4 W | **Pull-up de A0** (crítico — ver abaixo) |
| **R5** | 10 kΩ | Axial 1/4 W | **Pull-down de GPIO15/CSN** (boot) |
| **C1** | 470 µF / 16 V | Radial D8 | Reservatório da entrada 5 V |
| **C2** | 100 nF | Disco 5 mm | Entrada do LDO |
| **C5 / C3** | 10 µF / 100 µF | Radial D8 | Saída do LDO (3V3_RF) |
| **C4** | 100 nF | Disco 5 mm | Desacoplamento junto ao rádio |
| **SW1** | CFG / AP | Push 6 mm | Curto de A0 ao GND → modo configuração |
| **H1–H4** | — | Furo M3 (3,2 mm) | Fixação |

---

## Três decisões de projeto que valem explicação

### 1. R4 (10 k de A0 para 3V3) não é opcional

`isA0Low()` lê A0 duas vezes e considera "modo manutenção" se a média ficar
abaixo de 400 (de 1023). No D1 mini o pino A0 do header entra por um divisor
220 k / 100 k até o ADC do ESP. **Com A0 flutuando o ADC lê perto de zero**, o
firmware entenderia isso como pedido de modo AP e a placa **nunca sairia do
portal de configuração**.

Com R4 = 10 k para 3V3, o ADC fica em ≈1,0 V (≈1023) em repouso e vai a 0 quando
SW1 é pressionado. É o que dá sentido ao caminho `else if (isA0Low(RESET_PIN))`
em `libDryGatewayMqtt.cpp:251`.

### 2. LDO dedicado (U3) para o rádio

O `platformio.ini` usa `-D MY_RF24_PA_LEVEL=RF24_PA_HIGH`. Com um nRF24L01+
"nu" o consumo é modesto (~13 mA), mas com a variante **PA+LNA** — a esperada
num gateway central — o pico de TX passa de 100 mA e o regulador embarcado do
D1 mini fica no limite, provocando resets e perda de pacotes. U3 alimenta apenas
o rádio, com C3/C5/C4 no ponto de uso. A rede `+3V3_RF` é separada da `+3V3`
(que só alimenta R4 e o pino 3V3 do header de expansão); os GNDs são comuns.

### 3. R5 (10 k de CSN para GND)

GPIO15 precisa estar em nível baixo no boot do ESP8266. Como o CSN do nRF24
fica em alto-Z antes do firmware assumir o pino, R5 garante o *strap* correto.

**O que foi deliberadamente omitido:** pull-up em GPIO2/CE (o D1 mini já tem, via
o LED onboard) e botão de reset + pull-up de RST (o próprio módulo tem os dois, e
com o D1 mini em soquete o botão fica acessível por cima). O pino RST de U1 está
marcado como *no-connect* no esquemático.

---

## Layout

Placa de **100 × 70 mm**, 2 camadas, planos de GND em ambas as faces.

```
        ┌──────────────────────────────────────────────────┐
  y=14  │ J1 → F1 → D1 → C1 → C2 → U3 → C5 → C3            │  cadeia de energia
        │                                                  │
  y=26  │            R5      ┌───── U2 (nRF24) ─────►      │  antena p/ direita
  y=32  │  ┌──── U1 ────┐    │ SPI curto (4 trilhas)       │
  y=44  │  │  D1 mini   │ R4 │                    SW1      │
  y=58  │  └────────────┘    │                             │
  y=66  │  R1  R3  R2        │            J2 (expansão)    │
  y=74  │  D2  D4  D3        │                       J3    │
        └──────────────────────────────────────────────────┘
```

- **U1 girado 180°** para que a coluna de pinos D5–D8 (SPI) fique voltada ao rádio.
  Isso reduz as quatro trilhas SPI de ~50 mm para ~25 mm cada. Consequência: o
  conector USB do D1 mini aponta para a borda inferior (marcado na serigrafia).
- A camada superior concentra energia (5 V, 3V3, 3V3_RF) e a inferior os sinais SPI.
- O footprint do D1 mini traz uma **zona de exclusão de cobre** (região da antena do
  ESP-12, x 15,75–41,35 / y 51,4–58,2). O roteamento a contorna — se for mover peças,
  respeite essa área.
- Os planos de GND têm um **recorte em x 84–99,5 / y 19–41**, sob a ponta do módulo
  de rádio, para não degradar a antena.

### Regras de projeto

| Parâmetro | Valor |
|---|---|
| Isolação mínima | 0,25 mm |
| Trilha padrão / sinal | 0,35 mm |
| Trilha de 3V3 | 0,5 mm |
| Trilha de 5 V / 3V3_RF | 0,8 mm |
| Via | 0,8 mm (furo 0,4 mm) |
| Vias de costura GND | 0,6 mm (furo 0,3 mm) |

Compatível com o processo mais barato de qualquer fab (JLCPCB/PCBWay classe 2).

---

## Montagem e uso

1. Alimentar J1 com **5 V regulados** (fonte de 5 V/1 A). D1 protege contra inversão;
   F1 abre em curto.
2. O D1 mini entra nas barras fêmea com o **USB voltado para a borda inferior**.
3. O nRF24 entra em U2 com a **antena apontando para a borda direita** da placa.
4. `SW1` pressionado durante o boot (ou config inválida) → modo AP `M360-Config`
   em `192.168.4.1`.
5. LEDs, conforme `updateLEDStatus()`:
   - verde aceso = WiFi + MQTT OK
   - amarelo piscando 400 ms = WiFi OK, sem MQTT
   - amarelo piscando 200 ms = modo AP
   - vermelho piscando 300 ms = sem WiFi
6. `J3` fechado liga o IRQ do rádio ao D3 (GPIO0). **Deixe aberto** — o firmware atual
   não usa interrupção e GPIO0 é pino de *strap* de boot.

---

## Pendências antes de fabricar

1. **Preencher as zonas**: abra o `.kicad_pcb` no KiCad e tecle `B`. O backend usado
   aqui não preenche zonas, então as 36 vias de costura aparecem como
   *via dangling* no DRC até o primeiro preenchimento.
2. **Serigrafia**: 3 avisos cosméticos de DRC (texto de referência próximo à borda /
   sobre máscara). Reposicione com *Autoposition Field* no KiCad.
3. **Conferir o módulo de rádio físico**: o footprint é apenas o soquete 2x4. Confirme
   que o corpo do seu módulo (≈15,3 × 29 mm) cabe na área reservada e que a antena
   fica sobre o recorte dos planos.
4. **Se usar nRF24 PA+LNA**: considere subir C3 para 220 µF e conferir se o AMS1117
   dissipa bem (5 V → 3,3 V a 120 mA ≈ 0,2 W, ok em SOT-223).
