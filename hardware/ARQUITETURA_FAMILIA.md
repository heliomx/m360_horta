# Família de PCBs M360 Horta — Arquitetura

**Status:** esquemáticos da LP, da ACT e da GW **corrigidos após análise adversarial** (2026-09-13,
ERC 0). **As três placas estão defasadas desses esquemáticos e serão refeitas** (§9); só a M360-RF
está íntegra. Fabricação não gerada em nenhuma.
**Data:** 2026-09-13 · **Branch:** `feat/hardware-familia-pcb`
**Ferramentas:** KiCad 10.0.4 (esquemático, footprints e placa) → Freerouting 1.9.0 (roteamento, §11) → PCBA JLCPCB.

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
| **M360-ACT** | Nó atuador | 99 | 5 V do quadro (borne J1) | 3,3 V (MCU) + 5 V (relés, MUX) |

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
| 11 | **KiCad 10 do início ao fim** | Footprints do LCSC importados por `easyeda2kicad`, já com o código da peça; roteamento pelo Freerouting (§11). O EasyEDA saiu do fluxo |
| 12 | **PCBA** (placas montadas) nas quatro placas | SMD do LCSC. Soldados à mão só os módulos (Pro Mini, D1 mini, RFM95W) e os conectores de inserção (soquetes 2×5, RJ45, bornes, JST) — §8 |
| 13 | **Reed switch** (SW1, Littelfuse 59170 SMD, pela PCBA) **nas três placas** | 1 pino por placa: na LP, D3/INT1, acorda o nó dormindo; na ACT, A4; na GW, A0 (modo AP e factory reset). Acionado por ímã **através da parede**, sem abrir a caixa. O botão tátil que o substituiu em 2026-09-12 durou um dia |
| 14 | **GW em SMD**, montada pela JLCPCB (2026-09-13) | Mínimo de montagem manual ao menor custo: os passivos SMD são todos basic, os THT eram todos extended (§6.2) |

Frequência LoRa: provisionada por software na instalação (sem chave DIP) — ver
`analise_radio_rfm95w_915.md` §5 e a decisão de um gateway por propriedade.

---

## 3. M360-RF — mezanino de rádio

| Bloco | Conteúdo |
|---|---|
| Rádio | RFM95W-915S2. Footprint **stock do KiCad** `RF_Module:HOPERF_RFM9XW_SMD` (confere com o contorno do datasheet p. 120; pads avançam 1 mm além da borda, bom para solda manual). Símbolo `RF_Module:RFM95W-915S2` |
| Desacoplamento | 100 nF + 22 µF cerâmico nos pinos 13 (3.3V) e 10 (GND). O datasheet pede só 100 nF; o reservatório é para o pico de 87–120 mA sobre bateria e LDO |
| Antena | Trilha de RF curta do pino 9 (ANA) até um **SMA fêmea de borda** Samtec SMA-J-P-H-ST-EM1 (SMD, montado na PCBA). A trilha é bem menor que λ/20 em 915 MHz, mas é desenhada como coplanar aterrada no stackup do JLC |
| Espessura da placa | **1,6 mm (padrão do JLC) — nunca mais fina.** O Samtec abraça a borda com terra nas duas faces e é feito para 1,57 mm (0,062"); numa placa de 0,8 ou 1,0 mm ele fica frouxo e o terra não encosta |
| Terra | Plano sólido, costura de vias em volta da trilha de RF e sob o módulo |
| Interface | Header 2×5, 2,54 mm |

**Header 2×5 (vista do mezanino):**

| Pino | Sinal | Pino | Sinal |
|---:|---|---:|---|
| 1 | MISO | 2 | DIO1 |
| 3 | MOSI | 4 | DIO0 |
| 5 | SCK | 6 | 3V3 |
| 7 | NSS | 8 | GND |
| 9 | RESET | 10 | GND |

**Por que esta ordem:** a coluna ímpar fica junto ao módulo e segue a ordem dos pinos 2–6 do
RFM95W (MISO, MOSI, SCK, NSS, RESET), então o SPI sai em linha reta. DIO1, DIO0 e 3V3 estão na
coluna direita do módulo; contornam o rádio por cima e descem na coluna par na única ordem que não
cruza — tudo em F.Cu, com o plano de terra da face de baixo inteiro sob o rádio. A primeira
pinagem (3V3 no pino 1, SPI entre GNDs) obrigava cruzamentos.

**Layout e interface mecânica** (vista de cima, origem no canto superior esquerdo da placa):

| Item | Valor |
|---|---|
| Placa | 34,2 × 23,0 mm, 2 camadas, 1,6 mm |
| J1 (face de baixo) | Pino 1 (MISO) em (7,00; 6,42) mm; coluna par em x = 4,46 mm; linhas a cada 2,54 mm até y = 16,58 mm |
| SMA | Eixo em y = 18,50 mm; o conector avança ~10 mm além da borda direita |
| Trilha de RF | Coplanar com terra: 1,25 mm de largura, gap de 0,2 mm, ~5 mm de comprimento |
| Planos | GND nas duas faces, clearance de 0,2 mm; 17 vias de costura, **nenhuma sob o RFM95W** (via sob módulo soldado à mão só é segura se tampada) |
| Serigrafia | Referências de C1, C2 e J2 ocultas — não cabem no canto do SMA, e a PCBA usa o arquivo de posição |

O carrier vê o mezanino por baixo e, portanto, espelha as coordenadas do J1 no eixo X.

**Chaveamento mecânico — sem mudar placa.** O header 2×5 é simétrico e o mezanino encaixa girado
180°: o pino 6 (3V3 do rádio) cairia no SCK do carrier e os pinos 8/10 (GND) em MISO/MOSI, em curto
com um pino do AVR em nível alto. Na montagem, **cortar o pino 10 do header do mezanino e tampar o
furo 10 do soquete do carrier**. O pino 10 é GND duplicado (o 8 continua GND). Girado, a posição k
vai para 11 − k: o pino 1, presente, cai sobre o furo 10 tampado e o mezanino não entra.

> ⛔ **Sem pull-up no NSS dentro do mezanino.** No gateway o NSS vai ao GPIO15, que tem de estar
> em LOW no boot do ESP8266. Quando o carrier precisa de pull-up no NSS (nós AVR, para o rádio não
> ser selecionado durante a gravação), ele fica no carrier.

---

## 4. M360-LP — nó sensor

### 4.1 Alimentação

```
Painel ─►|─ [TP4056+DW01+8205] ─ OUT+/OUT− ─┐
   (módulo externo, 6 terminais)            ├─► J_VIN (3,0–6 V) ─► LDO 3,3 V ─► Pro Mini VCC + mezanino
Fonte 5 V (variante fixa, Nós 01/02) ───────┘              │
                                                            └─► 2 MΩ / 1 MΩ + 100 nF ─► A6
```

| Item | Especificação | Motivo |
|---|---|---|
| LDO 3,3 V | Iq ≤ 5 µA; dropout ≤ ~150 mV @ 100 mA; Vin máx ≥ 6 V. Candidato: MCP1700-3302 (confirmar estoque no JLC) | Sono do nó; rádio a +17 dBm com a bateria em 3,4 V; fonte de 5 V na variante fixa |
| Divisor de bateria | **2 MΩ / 1 MΩ permanente, 100 nF no A6** | A6 nunca passa de 2 V, nem com 6 V na entrada. Drena ~1,4 µA com a bateria a 4,2 V. O capacitor fornece a carga do sample-and-hold (a fonte de 667 kΩ sozinha seria alta demais para o ADC). Um divisor comutado pela perna de baixo deixaria o A6 na tensão da bateria — acima do VCC — quando desligado. **Custo aceito:** a fuga de entrada do A6 contra os 667 kΩ desloca a leitura (típico dezenas de mV na bateria a 25 °C; 1 µA no pior caso do datasheet daria ~2 V) e deriva com a temperatura. O firmware calibra em dois pontos e usa `analogReference(DEFAULT)` — com a referência interna de 1,1 V o A6 satura para qualquer bateria acima de 3,3 V |
| Proteção da célula | **Do módulo: TP4056 com DW01 + 8205 e saída OUT±** (6 terminais) | A LP não tem bloqueio de subtensão próprio. **Não servem:** o módulo de 4 terminais (IN±/BAT±) de `docs/tp4056.jpeg`, sem DW01 nem 8205 — a célula desceria até o MCU parar e continuaria drenando no sono —; e o "18650 Battery Shield V3" do BOM do Nó 04, cuja saída passa por boost, o que inviabiliza a leitura da bateria pelo A6. Além disso o firmware para de transmitir abaixo de um limiar de bateria |
| Bateria no VCC? | **Não** | O Nó 04 atual (nRF24) roda o MCU direto na bateria; com RFM95W isso não pode: o SPI do MCU chegaria ao rádio a 4,2 V, e o máximo absoluto do SX1276 é 3,9 V |
| Inversão de polaridade | **P-MOSFET AO3401A (Q1) em série no positivo**, gate no GND | Queda de ~5 mV a 100 mA; um Schottky perderia ~0,3 V, que faltam com a bateria em 3,4 V e o rádio transmitindo. Com a entrada invertida o FET não conduz e o LDO fica protegido. Vgs máx. ±12 V; a entrada nunca passa de 6 V |
| Capacitores do LDO | 1 µF na entrada; 1 µF + 10 µF na saída | O mezanino ainda soma 22 µF + 100 nF junto ao rádio |

A leitura de bateria pelo bandgap interno (padrão de `M360Power.cpp`) **não serve** nesta placa: com
VCC regulado ela mede 3,3 V. O nó sobrescreve `readBatteryVoltage()` lendo o A6.

### 4.2 Blocos

| Bloco | Conteúdo |
|---|---|
| MCU | Pro Mini 3,3 V / 8 MHz, LED de power removido |
| Trilho chaveado (VSW) | P-MOSFET AO3401A (Q2) high-side no 3,3 V, comandado por D4 (LOW = liga). Gate com 100 kΩ para o 3,3 V (desligado no boot) e partida suave de ~100 µs: 10 kΩ em série a partir do D4 e 10 nF entre gate e source. 1 µF no VSW. Alimenta sensores, pull-ups e o par marrom das portas |
| Multiplexador | 74HC4051 no VSW. **Regra de firmware:** levar S0–S2 a LOW antes de cortar o VSW, senão os pinos alimentam o CI pelos diodos de proteção |
| Sondas de solo | Pull-up de 10 kΩ para o VSW por canal (sonda resistiva, como os Nós 01/02); **DNI** para sonda capacitiva. RC 1 kΩ / 100 nF por canal |
| Digitais de campo | 1-Wire com 4,7 kΩ para o VSW; DHT com 10 kΩ para o VSW; I2C com 3,3 kΩ para o VSW |
| Reed switch (SW1) | D3 (INT1) com pull-up interno; contato normalmente aberto, zero corrente no sono. Littelfuse 59170, SMD, pela PCBA |
| Proteção | 3× **SRV05-4** (U3–U5, ≤ 1 pF por linha): U3 = solo ch0–3, U4 = ch4–7, U5 = 1-Wire, DHT, SDA, SCL. O pino VP de cada um vai ao VSW, então o zener interno protege também o par marrom das portas |
| Programação | Header FTDI do próprio Pro Mini. **O VCC do FTDI não pode entrar:** o header liga direto no trilho de 3,3 V, que alimenta o rádio. Um adaptador em 5 V poria 5 V no SX1276 (máximo absoluto 3,9 V); em 3,3 V, adaptadores FT232RL tiram o trilho do regulador interno de 50 mA e o boot entra em laço no primeiro TX. Gravar com `J_VIN` alimentado e o fio VCC do adaptador desligado (§8) |
| Mezanino (J4) | Soquete 2×5. **A numeração dos pinos no esquemático é a do mezanino (§3).** O `PinSocket_2x05` stock já encaixa pino a pino: nele a coluna par fica em x negativo, o mesmo lado em que ela fica no J1 do mezanino (header na face de baixo, vista de cima). Footprint `M360:M360-RF_Soquete_2x05` = o stock + contorno do mezanino e do SMA na F.Fab. NSS com 10 kΩ para o 3,3 V (R3) |

**Por que a partida suave do VSW:** ao ligar o trilho, a capacitância dos sensores é carregada a partir
do 3,3 V. Sem limitar a subida, a divisão de carga derruba o 3,3 V: com ~45 µF no 3,3 V e uma sonda
capacitiva de 10 µF no VSW, o trilho cairia a 3,3 × 45/55 ≈ 2,7 V — o brown-out do ATmega a 8 MHz.

**Regras de firmware que o hardware impõe:** com o VSW desligado, nenhum pino do MCU ligado ao campo
pode ficar em HIGH — S0–S2 do 4051, A1 (1-Wire), A2 (DHT) e A4/A5 (o TWI tem de estar desligado, sem
os pull-ups internos). Um pino alto alimenta os sensores pelos diodos de proteção.

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

O quadro já distribui energia (foto em `docs/quadro nó atuador.jpeg`): uma fonte slim 220 V → 12 V
embaixo da placa perfurada alimenta dois conversores DC-DC ajustáveis, e quatro barramentos de
latão levam **+12 V, GND, +5 V e +3,3 V** a todo o quadro. A placa entra apenas com **5 V**:

| Decisão | Motivo |
|---|---|
| **Nenhum 12 V na placa** | Os 12 V só alimentam as cargas, pelos contatos dos relés, que ficam no quadro. Nem 12 V nem 220 V encostam na PCB |
| **Sem buck a bordo** | O quadro já entrega 5 V; o conversor 12 → 5 V previsto antes deixa de existir |
| **3,3 V gerado na placa**, não tirado do barramento | O pico de TX do rádio (87–120 mA) num trilho compartilhado, por fios longos e vindo de um DC-DC ajustável, chega com queda e ruído. Com LDO local e desacoplamento junto ao mezanino, o rádio ganha trilho próprio. O regulador do próprio Pro Mini não sustenta esse pico |
| 5 V sai também pelo par marrom dos RJ45 | É o VCC do opto dos módulos de relé, e tem de ser **o mesmo 5 V** que alimenta o 74HCT244 (§5.3) |
| **Inversão no borne: P-MOSFET AO3401A (Q1)** em série no positivo, gate no GND | Mesma topologia do Q1 da LP. Fio invertido no J1 poria −5 V no regulador, no 74HCT244 e nos TVS. Vgs de −5 V, dentro do ±12 V |
| **Regulador: ME6211C33 (U1)**, SOT-23-5, 500 mA | Estável com cerâmico. Substitui o AMS1117, cujo datasheet pede 22 µF de tântalo na saída para garantir estabilidade. EN ligado ao 5 V; 10 µF + 100 nF na entrada e na saída, somados aos 22 µF + 100 nF do mezanino |

### 5.2 Blocos

| Bloco | Conteúdo |
|---|---|
| MCU | Pro Mini 3,3 V / 8 MHz |
| Fronteira 3,3 → 5 V | **74HCT244** (VIH 2,0 V aceita 3,3 V direto): S0–S3, SIG, NFT, OXI. **/OE com pull-up de 10 kΩ (R2) para o 3,3 V**, liberado pelo A5 depois do boot. Não para 5 V: com o A5 em alta impedância (boot, reset, gravação) o diodo de clamp do pino conduziria e ele ficaria a ~3,8 V, acima do máximo absoluto |
| Estado definido com o /OE alto | **7× 100 kΩ (R7–R13)**, do lado de 5 V do buffer: pull-down em MUX_S0–S3 e pull-up para 5 V em MUX_SIG, REL_NFT e REL_OXI. Sem eles, as entradas do CD74HC4067 flutuariam em meio nível durante boot, reset e gravação. Relé ativo em LOW: pull-up = desligado |
| MUX | CD74HC4067 a 5 V (EN no GND), **10 canais em uso** e 6 de reserva |
| Vazão | Entrada D3 com divisor 10 kΩ / 20 kΩ + 1 nF + TVS. **Firmware: `INPUT`, não `INPUT_PULLUP`** — o pull-up interno contra o divisor leva o nível baixo a 0,44–0,83 V, contra VIL de 0,99 V |
| Entrada analógica (A6, porta J5) | **Divisor 10 kΩ (R5) / 20 kΩ (R6) + 100 nF (C8)**, igual ao da vazão; o TVS U8 fica do lado do conector (rede AUX_A6_IN). O par marrom da porta entrega 5 V, e um sensor alimentado por ela poria até 5 V no pino (máximo absoluto VCC + 0,5 V = 3,8 V). O divisor leva 0–5 V a 0–3,33 V |
| Reed switch (SW1) | A4 (o nó nunca dorme; lido por polling). Fica **na própria placa**, na borda direita: o ímã é aproximado por fora do quadro, que é plástico |
| Antena | Fica **dentro do quadro**. A caixa é plástica, mas há metal por perto — barramentos, trilho DIN e disjuntores. Medir o alcance na instalação; se cair, levar o SMA para fora da parede |
| Fora do escopo | DHT, pH e oxigênio saem deste nó. O único sensor é a **vazão**, que chega pela porta J5 |

**Por que o /OE:** enquanto o Pro Mini inicia, os pinos dele ficam soltos. Sem o /OE, as saídas do
74HCT244 ficariam indefinidas e um relé poderia bater na partida. Com o /OE em alto, as saídas ficam
em alta impedância, a entrada do módulo de relé fica solta — e **entrada solta = relé desligado**,
a mesma regra de segurança do Nó 99 atual.

**Regra de firmware:** colocar D4–D8, A0 e A1 em OUTPUT e HIGH **antes** de baixar o A5. O /OE cobre
esse intervalo, desde que seja liberado por último.

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
| D9 | livre (era o DHT) | A4 | reed switch |
| D10–D13 | NSS / MOSI / MISO / SCK | A5 | /OE do 74HCT244 |
| | | A6 | entrada analógica da J5 (divisor 10 k / 20 k) |
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
| Reed (AP / reset) | A0 | Mesmo pino do botão antigo; o firmware não muda |

### 6.1 Blocos

| Bloco | Conteúdo |
|---|---|
| MCU | D1 mini (ESP8266), símbolo e footprint **stock**: `RF_Module:WEMOS_D1_mini` e `RF_Module:WEMOS_D1_mini_light`. O footprint traz uma zona de **keep-out sobre a antena** — nenhum cobre do carrier pode entrar ali |
| Mezanino (J1) | Soquete 2×5, `M360:M360-RF_Soquete_2x05`, numeração do mezanino (§3). **Sem pull-up no NSS** — ao contrário da LP; aqui o NSS é o GPIO15, que tem de estar em LOW no boot |
| Alimentação | 5 V externo no borne J2 → **Schottky SS14 (D1)** → pino 5V do módulo; o 3,3 V do regulador do próprio D1 mini alimenta o mezanino. 100 nF no 5 V, 10 µF + 100 nF no 3,3 V, somados aos 22 µF + 100 nF que o mezanino já tem junto ao rádio. **Por que o diodo:** o pino 5V do D1 mini é o VBUS do USB, sem diodo no módulo (esquemático v4.0.0). Sem o D1, fonte externa e porta USB ficariam em paralelo, e a mais alta alimentaria a outra. O diodo deixa a fonte externa ~0,45 V abaixo do VBUS e bloqueia fio invertido no borne. Mesmo assim, **não alimentar pelos dois ao mesmo tempo** |
| LEDs | **Fora da placa**, no painel da caixa. A bordo ficam só os resistores de 1 kΩ; o **J3** leva os três pares ao chicote. Vermelho (D2) e amarelo (D0) têm o cátodo no GND; o **verde vem do 3,3 V para o D4**, ativo em LOW — o pino afunda a corrente e, no boot, o GPIO2 é mantido alto pelo próprio LED |
| J3 (LEDs) | JST XH de **6 vias, em pares ânodo/cátodo**: 1-2 vermelho, 3-4 amarelo, 5-6 verde. Não são 4 vias com terra comum porque **o verde não tem o cátodo no GND** — ele vai ao D4. Com pares dedicados, os três LEDs são ligados da mesma forma no chicote apesar dessa assimetria, e o conector é chaveado, o que impede inversão no campo |
| Reed (AP / reset) | SW1, Littelfuse 59170, do A0 ao GND, com 10 kΩ para o 3,3 V e 100 nF. O ímã por fora da caixa aciona o modo AP; mantido por 3 s nos primeiros 30 s, o factory reset |
| Sem ligação | DIO1 e RESET no mezanino; RST, D3, RX e TX no D1 mini |

### 6.2 Montagem: SMD pela JLCPCB (até 2026-09-13, 100% THT)

A GW foi desenhada toda de inserção — resistores axiais, cerâmicos de disco, eletrolítico radial e
botão THT — e passou a **SMD montada pela JLCPCB**, pela regra da família: **mínimo de montagem
manual ao menor custo**. Em SMD os passivos são todos *basic* (sem taxa de setup), enquanto todo
substituto THT encontrado era *extended*; e a JLCPCB cobra US$ 3,50 de mão de obra THT mais
US$ 0,0173 por junta.

| Vai pela PCBA | Fica para solda manual |
|---|---|
| R1–R3 1 kΩ e R4 10 kΩ (0603), C1 10 µF (0805), C2–C4 100 nF (0603), D1 SS14 (SMA), SW1 reed | U1 (D1 mini — módulo, sem equivalente no catálogo), J1 (soquete do mezanino), J2 (borne), J3 (JST dos LEDs) |

O C1 deixou de ser eletrolítico: em cerâmico não há polaridade, e o símbolo voltou a `Device:C`.

**Por que o pull-up no A0:** sem ele o A0 flutua e o gateway entra em modo AP sozinho. O D1 mini já
traz um divisor 220 k/100 k entre o pino A0 e o ADC de 1,0 V do ESP; com 10 kΩ para o 3,3 V o pino
em repouso fica em ~3,2 V e a leitura satura em 1023 — inofensivo, porque `isA0Low()` só compara
com o limiar 400. Com o reed fechado o pino vai a 0 V e drena 0,33 mA.

> ⛔ **Pendência de firmware, não de hardware.** `M360Leds.h` ainda define `LED_GREEN D1` — que nesta
> placa é o DIO0 — e `M360Leds.cpp` acende os três LEDs com `digitalWrite(pin, HIGH)`. A placa exige
> o verde no **D4 e invertido** (LOW acende). A inversão toca `initLEDs`, `setLedState`, `updateLEDs`
> e `ledFlicker`: não é uma linha. Enquanto não for feita, o LED verde funciona ao contrário.
>
> Sem efeito, mas vale corrigir junto: em `libDryGatewayMqtt.cpp` a chamada é
> `initLEDs(LED_RED, LED_GREEN, LED_YELLOW)` e o protótipo é `(redPin, yellowPin, greenPin)`. Os
> argumentos estão trocados; o pino acionado sai certo porque `setLedState` despacha pelo número do
> pino, mas o nome da variável interna fica invertido.

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
| **J5 SENSORES** | Vazão / livre (era o DHT) | A6 (analógico, 0–5 V pelo divisor) / livre | livres |

Todas as linhas de sinal levam TVS no conector. SDA e SCL compartilham um par: aceitável até ~2 m a
100 kHz; acima disso, reavaliar.

---

## 8. Montagem e fabricação

| Item | Como entra |
|---|---|
| SMD (LDOs, MOSFETs, diodo, 74HC4051, 74HCT244, CD74HC4067, passivos, TVS, reed switch, SMA de borda Samtec) | PCBA JLCPCB, com peças escolhidas pela disponibilidade no LCSC |
| Pro Mini, D1 mini, módulo RFM95W (HopeRF, no mezanino), soquetes e headers 2×5, RJ45, bornes, JST | Solda manual após a PCBA, **fora do BOM e do arquivo de posição** (`tools/solda_manual.py`): LP — A1, J1, J2, J3, J4; ACT — A1, J1, J2, J3, J4, J5; GW — U1, J1, J2, J3. A propriedade `LCSC` continua nos símbolos, para a compra. Hoje só os esquemáticos estão marcados; as placas recebem a marca quando forem refeitas |
| Chave do mezanino | Cortar o pino 10 do header do mezanino e tampar o furo 10 do soquete (§3) |
| Gravação do Pro Mini | Adaptador FTDI **sem o fio VCC**, com `J_VIN` (LP) ou o borne (ACT) alimentado (§4.2) |
| Antena | SMA de borda no mezanino: ou o conector atravessa a parede da caixa (furo e vedação), ou a antena fica dentro dela — funciona em 915 MHz se a caixa for plástica |

---

## 9. Pendências

| Item | O que falta |
|---|---|
| **Placas defasadas** | Os esquemáticos das três placas foram corrigidos em 2026-09-13 (análise adversarial + reed + GW em SMD). LP, ACT e GW precisam ser sincronizadas, reposicionadas e rerroteadas pelo fluxo da §11.5, conferindo com `tools/paridade.py`. A GW recomeça do zero, com `place_gw.py` reescrito para SMD |
| Banda do RFM95W | Validar em bancada (RTL-SDR em 916,8 MHz ou RSSI contra referência) antes de comprar em quantidade |
| Seleção de peças | Candidatas em §9.1. O 10 kΩ basic (C25804) não aparece no catálogo local de peças **em estoque** (conferido de novo em 2026-09-12): confirmar no site antes do pedido. Está em 11 posições da LP e 3 da ACT; não há outro 10 kΩ 0603 basic em estoque — a alternativa é extended (FOJAN FRC0603F1002TS, C2906982) |
| Footprints da LP | Feitos em `hardware/lib/M360.pretty`: Pro Mini, soquete do mezanino, RJ45 C385834 e reed C315909 (os dois do LCSC via easyeda2kicad, com os pinos-guia do RJ45 corrigidos para furo não metalizado e o modelo 3D do RJ45 subido 6,3 mm, que vinha atravessando a placa). O reed voltou em 2026-09-13, agora nas três placas |
| Furos livres do Pro Mini | **Medidos por foto** (`docs/prominiFront.jpg`): homografia a partir dos 4 pinos de canto das fileiras, cuja posição é conhecida (passo 2,54 mm, fileiras a 15,24 mm). Escala consistente em 2% nas quatro bordas. **A4/A5** ficam uma coluna para dentro da fileira direita e a meio passo das linhas: (12,70; 11,43) e (12,70; 8,89). **GND/A6/A7** ficam na borda de baixo em 7,62 / 10,16 / 12,70, com y = 28,3 ± 0,3. Os espaçamentos medidos (2,57 e 2,59 mm) confirmam a grade de 2,54; o viés comum de −0,25 mm em x é erro de leitura dos pinos de canto. Esses cinco furos levam **1,2 mm** (os das fileiras, 1,0 mm) para absorver a incerteza. Confirmar com paquímetro antes de fabricar |
| Placa da LP | Caixa de 100 × 100 × 50 mm → interior de ~92 mm. O plugue RJ45 crimpado é montado dentro da caixa e precisa de ~25–30 mm livres à frente dos conectores (corpo do plugue e capa), com os prensa-cabos na parede em frente a eles. Isso limita a placa a **86 × 62 mm**, formato em que o posicionamento foi refeito: RJ45 e reed na borda de baixo; TVS logo atrás deles; matriz RC dos 8 canais na faixa do meio; Pro Mini deitado no alto à esquerda (FTDI para fora da borda esquerda) com o 74HC4051 ao lado; mezanino no alto à direita com o SMA pela borda; alimentação e trilho VSW **embaixo do mezanino** (só SMD baixo: o mezanino fica a ~11 mm, o MCP1700 é linear e o P-MOSFET do VSW chaveia devagar); `J_VIN` na borda direita; 4 furos M3. Sem sobreposição de courtyard no DRC. Roteada (§11), com planos de terra nas duas faces e serigrafia acertada — DRC com 0 erros e 0 ligações pendentes. Falta gerar os arquivos de fabricação |
| Placa da GW (versão THT, **substituída pela SMD** — §6.2) | **80 × 60 mm**, montagem 100% THT — em inserção os passivos cobram área: cada resistor axial ocupa 12,4 × 3,1 mm contra 1,6 × 0,8 de um 0603. Arranjo: D1 mini no alto à esquerda, com a ponta da antena/USB **na borda de cima** (o footprint traz keep-out sobre a antena, e assim o cabo entra sem abrir a caixa); mezanino no alto à direita, com a borda direita dele coincidindo com a da placa, para o SMA sair pela lateral; desacoplamento logo abaixo do módulo; borne de 5 V na borda esquerda; o J3 dos LEDs na borda de baixo (os LEDs em si ficam no painel, §6.1), com os resistores em pé logo acima; botão de AP e pull-up do A0 na faixa livre sob o mezanino. Furos M3: H1 desvia do D1 mini e **H2 não fica no canto** — ali cairia sob o mezanino e a placa teria de ser parafusada antes de encaixar o módulo. Roteada pelo Freerouting (§11): 0,7 s de roteamento e 4 s de otimização, ~53% de melhoria, 133 segmentos e **1 via só** — numa placa toda THT cada ilha já atravessa as duas faces, então o roteador troca de camada nos próprios furos. Planos de GND nas duas faces. O preenchimento acusou `starved_thermal` na ilha 2 do J3 e **as hastes de 0,35 mm com folga de 0,3 bastaram**, sem precisar da ligação sólida que os RJ45 da LP e da ACT exigiram: nas ilhas do JST (1,7 × 1,95 mm a passo de 2,5) os raios verticais têm espaço assim que a folga cai de 0,5 para 0,3. Marca "Viridiotech.com.br" na faixa livre entre o mezanino e o C3/R4 — a 1,5 mm de altura ela mede ~20 mm, e na primeira posição encostava na ilha do C3. DRC final: 0 violações, 0 ligações pendentes. Faltam os arquivos de fabricação |
| Gravação do Pro Mini | Com 86 mm de placa dentro de ~92 mm de interior sobram ~3 mm de cada lado, e o espaço livre restante fica à frente dos RJ45. **Não cabe o adaptador FTDI com a placa montada:** gravar na bancada, com a placa fora da caixa, ou prever cabo FTDI angulado. O nó é gravado antes da instalação; em campo, a atualização depende de retirar a placa |
| Regulador do Pro Mini | Com 3,3 V no VCC e RAW solto, o regulador da própria placa pode ser alimentado de volta pelo diodo interno e consumir a corrente quiescente dele. Medir a corrente de sono na primeira placa; se passar de poucos µA, retirar o regulador, como o LED |
| DHT11 no atuador | Alimentado a 5 V pelo RJ45, com pull-up para 3,3 V: confirmar que a saída é dreno aberto |
| Firmware das placas novas | Envs `pro8MHzatmega328`; pinos novos; `readBatteryVoltage()` pelo A6 na LP, com `analogReference(DEFAULT)` e calibração em dois pontos, e corte de TX por bateria baixa; LED verde ativo em LOW no gateway; na ACT, vazão com `INPUT` (hoje `INPUT_PULLUP` em `sensorDrivers.cpp:141`) e o A5 (/OE) liberado por último. Atualizar `inventario.md` e os `esquema_eletrico.md` na mesma entrega |
| Caixa | Modelo IP e dimensões internas; posição do SMA |
| Trilho de 3,3 V da GW | O ME6211 do D1 mini (500 mA) alimenta ESP8266 e RFM95W. Pelos datasheets a soma fica em ~290 mA; com os picos de WiFi medidos em bancada (300–400 mA) encosta no limite. Medir a queda com WiFi e LoRa transmitindo ao mesmo tempo — primeira suspeita se houver reset do gateway em campo |
| Fusível por porta RJ45 da ACT | Curto no chicote derruba o 5 V do quadro inteiro. Não há PTC *basic* no catálogo; avaliar PTC *extended* por porta |
| BOM da JLCPCB | Exportado em 2026-09-12. Códigos LCSC gravados nas três placas (ACT e GW com `add_lcsc.py --set`); só o **D1 mini** da GW fica sem código — não existe no catálogo. Os `PWR_FLAG` da GW (FLG1, FLG2) saíram do BOM em 2026-09-13. E o C13 da LP carregava o código do reed (C315909) desde `bc651ca`, o que faria a JLCPCB montar um reed no lugar de um capacitor de 100 nF — corrigido para C14663 |

### 9.1 Peças candidatas (LCSC — catálogo de 2026-09-09)

"Basic" é montada pelo JLC sem taxa; cada peça "extended" diferente cobra uma taxa de setup por pedido.

| Função | Peça | LCSC | Tipo | Estoque | Nota |
|---|---|---|---|---:|---|
| RF: rádio | RFM95W-915S2 (HopeRF) | — | manual | — | **Decidido:** soldar à mão os módulos HopeRF que já existem. O LCSC só tem o clone DL-RFM95-915M (C2844473), que exigiria refazer a validação de banda |
| RF: SMA de borda | SMA-J-P-H-ST-EM1 (Samtec, SMD) | C5199907 | ext | 1 125 | **Decidido.** Footprint stock do KiCad `Connector_Coaxial:SMA_Samtec_SMA-J-P-H-ST-EM1_EdgeMount`, com terra nas duas faces. ~US$ 5,4 |
| (descartado) | BWSMA-KE-P001 | C496550 | — | — | **Não é de borda:** é SMA vertical, 13,5 mm de altura (datasheet, p. 6). "Board Side" no LCSC quer dizer "lado da placa" (oposto de lado do cabo), não montagem na borda |
| RF: header 2×5 | PZ254V-12-10P | C492422 | ext | 114 152 | |
| 100 nF 0603 X7R | CC0603KRX7R9BB104 | C14663 | **basic** | 12,6 M | |
| 22 µF 0805 X5R 25 V | CL21A226MAQNNNE | C45783 | **basic** | 1,7 M | |
| LP: LDO 3,3 V | MCP1700T-3302E/TT (SOT-23) | C39051 | ext | 39 220 | Iq 1,6 µA; 178 mV @ 250 mA; Vin máx 6 V |
| LP: LDO alternativo | HT7333-A (SOT-89) | C347191 | ext | 162 436 | Iq 3 µA; entrada até 12 V; dropout maior com corrente alta |
| ACT: LDO 3,3 V | ME6211C33M5G-N (SOT-23-5) | C82942 | ext | 409 223 | **Decidido (2026-09-13):** estável com cerâmico, 500 mA. O AMS1117-3.3 (C6186, basic) saiu: o datasheet dele pede 22 µF de tântalo na saída |
| P-MOSFET | AO3401A | C15127 | **basic** | 715 902 | 85 mΩ @ −2,5 V. LP: Q1 (inversão) e Q2 (VSW); ACT: Q1 (inversão) |
| LP: MUX 8:1 | 74HC4051D,653 (SOIC-16) | C9386 | ext | 24 931 | |
| ACT: buffer | 74HCT244D,653 (SOIC-20) | C5978 | ext | 16 211 | |
| ACT: MUX 16:1 | CD74HC4067M96 (SOIC-24) | C496123 | ext | 12 982 | |
| ACT: buck 12 → 5 V | XL1509-5.0E1 (SOIC-8) | C61063 | **basic** | 708 804 | 150 kHz pede indutor grande; TPS54202 (C191884) é menor, mas extended |
| TVS de linhas analógicas | ESD5Z3.3T1G (SOD-523) | C2827729 | ext | 639 857 | 110 pF: **não** serve para 1-Wire/I2C em cabo |
| LP: TVS das linhas de campo | SRV05-4 (TECH PUBLIC, SOT-23-6) | C558418 | ext | 461 674 | 4 linhas, 0,3–0,8 pF: serve para solo, 1-Wire e I2C. Substitui o ESD5Z na LP |
| LP: entrada `J_VIN` | JST B2B-XH-A (2 vias) | C158012 | ext | 203 889 | J3; solda manual |
| LP: RJ45 | R-RJ45R08P-A004 (sem blindagem, sem LED) | C385834 | ext | 159 683 | J1 e J2; solda manual |
| Reed switch (SW1, as três placas) | Littelfuse 59170-1-S-00-D (SMD) | C315909 | ext | 35 144 | **Decidido (2026-09-13):** pela PCBA, nas três placas. O botão tátil TS665CJ (C393938), usado por um dia, saiu |
| LP: soquete do mezanino | PM254V-12-10P-H85 (fêmea 2×5, 8,5 mm) | C492399 | ext | 37 436 | J4; solda manual |
| 1 µF 0603 · 10 µF 0805 · 10 nF 0603 | CL10A105KB8NNNC · CL21A106KAYNNNE · 0603B103K500NT | C15849 · C15850 · C57112 | **basic** | milhões | |
| Resistores 0603 1 % | 1 k · 3,3 k · 4,7 k · 10 k · 20 k · 100 k · 1 M · 2 M | C21190 · C22978 · C23162 · C25804 · C4184 · C25803 · C22935 · C22976 | **basic** | | 10 k: ver §9. 20 k só na ACT (R4 e R6, divisores da vazão e do A6). 100 k também nos pulls do MUX da ACT (R7–R13) |
| ACT: 1 nF 0603 X7R | CL10B102KB8NNNC | C1588 | **basic** | 2 777 884 | C7, filtro da vazão |
| Borne de 5 V (ACT J1, GW J2) | WJ500V-5.08-2P | C8465 | ext | 234 530 | Solda manual |
| GW: diodo de entrada | SS14 (SMA) | C2480 | **basic** | 2 818 967 | D1 |
| GW: JST dos LEDs | B6B-XH-A (6 vias) | C144397 | ext | 47 935 | J3; solda manual |
| GW: D1 mini | — | — | manual | — | Não existe no catálogo. Os passivos da GW são os mesmos 0603/0805 basic das outras placas |

---

## 10. Ordem de trabalho

1. **M360-RF** — a menor placa e a única comum às outras três. ✔ esquemático e placa prontos.
2. **M360-LP** — cobre quatro nós. ✔ esquemático corrigido (2026-09-13); placa a refazer.
3. **M360-ACT**. ✔ esquemático corrigido (2026-09-13); placa a refazer.
4. **M360-GW** — D1 mini + mezanino + LEDs. ✔ esquemático em SMD (2026-09-13); placa a refazer do zero.

Antes das placas: nova análise adversarial dos esquemáticos, até zerar os achados. Depois, nas
quatro: arquivos de fabricação (Gerbers, furação, BOM e arquivo de posição).

---

## 11. Roteamento com o Freerouting

O KiCad não tem autoroteador. O roteamento desta família é feito pelo **Freerouting 1.9.0**,
chamado direto na linha de comando, com ida e volta em Specctra (DSN → SES).

É a única etapa sem script próprio: as demais (layout do esquemático, posicionamento, furos,
serigrafia, footprints) estão em [`tools/`](tools/README.md).

### 11.1 Por que na linha de comando, e não pelo MCP

A ferramenta `autoroute` do MCP do KiCad exige **Java 21** ou o Docker em execução; esta máquina
tem Java 17. O Freerouting **1.9.0** roda em Java 11+ — quem exige Java 21 é a série 2.x —,
então basta invocá-lo direto. O `.jar` (5 MB) fica em `~/.kicad-mcp/freerouting.jar` — fora do
repositório, mas nesse caminho de propósito: é onde o próprio MCP o procura, caso um dia o Java
seja atualizado. Para reproduzir em outra máquina, baixar da release do projeto:

```
https://github.com/freerouting/freerouting/releases/download/v1.9.0/freerouting-1.9.0.jar
```

### 11.2 Os três passos

1. **Exportar o DSN** — MCP `export_dsn`, ou *File → Export → Specctra DSN* no KiCad.
   As regras da placa precisam estar **salvas antes**: o DSN carrega isolamento e largura de
   trilha, e o MCP não persiste as regras sem um `save_project`.

   > ⚠️ Conferir a largura **no próprio DSN**, não no retorno da ferramenta. O
   > `set_design_rules` responde com o `trackWidth` que se pediu, mas o que ele grava na
   > netclass "Default" do `.kicad_pro` é o valor **mínimo**: na GW pedi 0,25 mm, a ferramenta
   > respondeu 0,25 e o DSN saiu com `(width 200)`. Quem o roteador lê é o DSN —
   > `(width 250)` = 0,25 mm, `(clearance 200)` = 0,2 mm. Se divergir, corrigir
   > `"track_width"` direto no `.kicad_pro` (é JSON) e reexportar.

   > ⛔ **O `export_dsn` ignora o `boardPath`: ele exporta sempre o projeto aberto na sessão.**
   > Com a ACT aberta, duas chamadas com `boardPath` apontando para LP e para ACT geraram
   > **dois arquivos com a ACT** — 72 964 e 72 965 bytes, e os dois roteamentos saíram com
   > exatamente 181 fios e 23 vias. Importar aquilo teria escrito a fiação da ACT dentro da LP.
   > Por isso: **`open_project` na placa certa antes de exportar**, e conferir de quem é o DSN
   > antes de rotear — contar `(place ` e procurar uma referência que só exista naquela placa
   > (`R25` na LP, `U8` ou `J5` na ACT). Como a sessão só tem uma placa corrente, **não dá para
   > trabalhar em duas placas em paralelo** pelo MCP.
2. **Rotear** — se a placa já tem zona de GND, tirando antes os planos do DSN (§11.6):

   ```
   python tools/dsn_sem_plano.py placa.dsn placa_sem_plano.dsn
   java -jar ~/.kicad-mcp/freerouting.jar -de placa_sem_plano.dsn -do placa.ses -mp 20
   ```

   `-de` é a entrada, `-do` a saída e `-mp` o máximo de passes. Com os dois arquivos informados
   ele roda em lote e encerra sozinho, sem abrir janela. Conferir no SES que o GND veio:
   `(net GND` precisa aparecer.
3. **Importar o SES** — MCP `import_ses`, ou *File → Import → Specctra Session*.

### 11.3 Conferência obrigatória depois de importar

O MCP **recusa a gravação automática** sempre que o arquivo da placa mudou em disco desde que
ele o carregou — e isso acontece o tempo todo neste fluxo, porque o `kicad-cli` e os scripts
escrevem por fora. Conferir sempre no arquivo, nunca pelo retorno da ferramenta:

| O quê | Como |
|---|---|
| Trilhas gravadas | contar `(segment` no `.kicad_pcb` |
| Roteamento completo | DRC com **0 não conectados** |
| Nada mais quebrou | DRC sem erros |

> ⛔ **Não recarregar o projeto no MCP antes dessa conferência.** Se o roteamento estiver apenas
> na memória, o recarregamento o apaga. Quando a gravação for recusada, o caminho seguro é
> recarregar **e refazer** a alteração — nunca `save_project --force`, que sobrescreve o disco
> a partir da memória.

### 11.4 O que esperar

| Placa | Ligações | Resultado | Tempo |
|---|---|---|---|
| M360-LP | 135 | 552 trilhas, 48 vias, 0 pendências | ~5,5 min |
| M360-ACT | 104 | 463 trilhas, 23 vias, 0 pendências | ~2,5 min |
| M360-LP, com botão (2026-09-12) | — | 572 trilhas, 44 vias, 0 pendências | ~2,5 min |
| M360-ACT, com botão (2026-09-12) | — | 489 trilhas, 24 vias, 0 pendências | ~3 min |

Quase todo o tempo é de otimização, que melhorou os desenhos em ~50%. O traçado sai com
cara de autoroteador — diagonais longas na face de baixo —, aceitável nestas placas, de sinais
lentos e sem requisito de impedância.

Os planos de terra entram **depois** do roteamento: o roteador trata o GND como qualquer outra
net e o plano preenche por cima. Isso só vale se o DSN **não declarar o plano** — no primeiro
roteamento as zonas ainda não existiam; num rerroteamento elas existem, e é preciso o
`dsn_sem_plano.py` (§11.6). É o preenchimento que costuma
revelar hastes térmicas insuficientes (`starved_thermal`) nos pads THT dos conectores. A
correção adotada nas duas placas, nesta ordem:

1. hastes de **0,35 mm** com folga de **0,3 mm** (o padrão, 0,5/0,5, não cabe no anel dos pads
   THT). Vale a pena insistir na haste antes de partir para o passo 2: é o alívio térmico que
   torna a solda manual do conector viável;
2. onde nem a haste fina cabe — pino 8 dos RJ45, cujo campo de pinos escalonado de 1,02 mm não
   comporta duas hastes —, ligação **sólida** (`zone_connect 2`) só naquele pad. Dá o melhor
   retorno de massa do chicote, ao custo de mais calor para soldar esse pino.

O passo 2 é o `tools/pad_zone_connect.py`, que age só no footprint pedido — um `sed` global
pelo pad "8" atingiria todo conector da placa.

### 11.5 Trocar peça depois de roteado obriga a rerrotear a placa inteira

Mudar o **footprint** ou a **posição** de uma peça já roteada invalida o roteamento: as trilhas
antigas continuam onde estavam e passam a cruzar as ilhas novas.

O sintoma é um amontoado de violações apontando todas para a mesma ilha — `clearance`,
`hole_clearance`, `solder_mask_bridge` e até `shorting_items`. **Não é defeito de projeto, é
cobre obsoleto.** Ao trocar o reed pelo botão, a M360-LP passou de 0 para 24 violações e a
M360-ACT para 37, todas com essa assinatura.

Remédio: `tools/clear_tracks.py` e rerrotear. **Rerrotear por cima não serve** — o DSN leva a
fiação existente na seção `wiring` e o Freerouting a preserva. E **rerrotear com o DSN como sai
do KiCad também não serve**: a placa já tem zona, e o GND fica sem fio (§11.6).

A sequência que funcionou na troca do reed pelo botão (2026-09-12), para cada placa:

```
del_fp.py <pcb> SW1                → sync_schematic_to_board (MCP) → place_*.py
clear_tracks.py <pcb>              → open_project (a placa certa)  → export_dsn
conferir o DSN: (place, referência exclusiva, (width 250), 0 (wire
dsn_sem_plano.py                   → Freerouting                   → import_ses
contar (segment no disco           → zone_thermals.py
kicad-cli pcb drc --refill-zones --save-board → pad_zone_connect.py / silk_*.py se o DRC pedir
```

A sincronização grava o footprint novo **direto no disco** e depois acusa "Auto-save refused"
sobre a cópia em memória — o aviso não significa que a troca se perdeu. Conferir no arquivo.

### 11.6 O GND não é roteado: ele é plano — e isso tem consequência

Se a placa já tem zona de GND, o DSN declara `plane GND` nas duas faces, e o Freerouting **não
desenha um fio de terra sequer**: medido, o SES da LP tem 0 ocorrências de GND e o da GW tem 1.
A rede de terra fica existindo só pelo preenchimento.

**Causa, confirmada em 2026-09-12.** O primeiro roteamento da LP (`bf701dc`) tem 88 segmentos
de GND: foi feito antes de as zonas existirem, e o GND foi roteado como net comum. Todo
rerroteamento posterior parte de uma placa **com** zonas, e perde esse GND. Rerroteada com o DSN
como sai do KiCad, a LP com o botão deu 16 `unconnected_items`, todos "Zona [GND]"; com o
`tools/dsn_sem_plano.py` antes do Freerouting, deu **0** — 100 segmentos de GND na LP, 95 na ACT.

Sem o fio de terra, o plano **se parte em ilhas** — o posicionamento que mudou só decide onde: cada
pedaço solto vira `unconnected_items` com "Zona [GND]" nos **dois** lados do par, e a coordenada
relatada é sempre o canto do polígono da zona (99,95; 99,95), nunca a posição da ilha — o que
torna o relatório quase inútil para localizar o problema. Na LP, o plano de F.Cu chegou a 14
ilhas e o de B.Cu a 6.

> ⛔ **Ajuste de zona não substitui roteamento.** Na M360-LP tentei, em sequência: vias de
> costura, refill, ligação sólida em 11 ilhas e remoção de ilhas mínimas. Os desconectados
> ficaram em **15 o tempo todo**, e a remoção de ilhas ainda piorou as violações de 9 para 91,
> porque apagou cobre que carregava ligação. O problema é topológico. Se o plano fragmentar,
> **rerrotear com o GND roteado** (`dsn_sem_plano.py`) — não remendar. Os 15 imóveis eram
> exatamente o aviso do fim da §11.7: o diagnóstico ("posicionamento") estava errado; a causa
> era o GND sem fio.

> ⚠️ O `tools/gnd_stitch.py` nasceu dessa tentativa e **não é confiável como está**: o critério
> de "ilha já ligada" é conter uma ilha de componente ou uma via de GND, o que não prova ligação
> ao plano principal. Depois das ligações sólidas ele passou a relatar "20 ilhas já ligadas, 0 a
> costurar" enquanto o DRC acusava 13 zonas soltas. Serve para **colocar** vias com verificação
> de colisão nas duas faces; não serve para **decidir** onde elas são necessárias.

### 11.7 Riscos operacionais do MCP

Além das duas armadilhas da §11.2 (`export_dsn` ignora o `boardPath`; `set_design_rules` não
persiste a largura de trilha), há uma terceira, de outra natureza: **o servidor pode travar por
inteiro.** Nesta sessão até o `get_board_info` passou a estourar os 30 s, e não há contorno — o
`kicad-cli` **não exporta nem importa Specctra** (a lista de subcomandos vai de `3dpdf` a `xao`,
sem DSN/SES). Com o MCP fora do ar, trabalho de roteamento simplesmente para.

Mitigação: fazer a etapa de placa inteira numa sessão só, e tratar o esquemático — que depende
bem menos do MCP — como trabalho separado.

**Dois hábitos que se pagaram, e um que faltou.** Busca vazia não é prova: três vezes nesta
sessão um `grep` meu voltou vazio por padrão errado (indentação, nome qualificado de símbolo,
`(via ` com espaço) e quase virou conclusão sobre a placa. Conferir no arquivo em vez de confiar
no retorno da ferramenta pegou erro real mais de uma vez. O que faltou: **número que não se move
enquanto tudo em volta muda é sinal de modelo errado, não de remédio faltando** — os 15
desconectados repetidos eram o aviso de que o diagnóstico estava furado, e insisti três
tentativas além do que devia.

---

## 12. Edição de esquemático: o que custou uma rodada

Medido nas correções da análise adversarial (2026-09-13).

**Toda edição é conferida por três comparações, não só pelo ERC:**

| O quê | Como | Pega |
|---|---|---|
| Ligações | `kicad-cli sch export netlist` antes e depois + `tools/netdiff.py` | fio que funde redes; label que não encosta |
| Campos de cada componente | valor, footprint, LCSC e flags de BOM comparados entre as duas netlists | propriedade gravada no componente errado |
| Símbolos de biblioteca embutidos | bloco `(lib_symbols)` comparado com o do HEAD | descrição, datasheet ou pinos alterados em outro símbolo |

> ⛔ **`replace_schematic_component` (MCP) corrompe outros componentes.** Ao trocar o C1 da GW de
> `Device:C_Polarized` para `Device:C`, ele **gravou o LCSC do C1 no J1** e escreveu "Polarized
> capacitor" como descrição do **símbolo do D1 mini**, apagando o datasheet. A netlist saiu idêntica e
> o ERC só acusou `lib_symbol_mismatch`. É o mesmo mecanismo que, em `0948368`, deixou o código do
> reed no C13 da LP. **Trocar símbolo por texto** (pinagem idêntica) ou apagar e recriar.

> ⚠️ **O MCP grava blocos fora do formato do KiCad** (espaços no lugar de tabulações). O KiCad lê,
> mas os scripts de `tools/` não enxergam o componente. Depois de cada rodada de edição pelo MCP,
> `kicad-cli sch upgrade --force <placa>.kicad_sch` regrava o arquivo no formato canônico — conferir
> que a netlist e os campos saem idênticos.

> ⚠️ **`fix_layout.py` não é idempotente.** Rodado de novo sobre um esquemático já arrumado, ele
> soltou pads do GND, dos canais de solo e dos pull-ups na LP e dos TVS na ACT. Serve para a primeira
> arrumação; peças novas recebem fio + label pelo padrão da folha (fio de `stub_len(nome)`, label na
> ponta) sem rodá-lo sobre a folha inteira.

> ⚠️ **`batch_add_and_connect` estoura os 30 s do MCP** com uma dúzia de peças e continua gravando
> depois do erro. Não repetir a chamada: esperar o arquivo parar de mudar e conferir o que entrou.
