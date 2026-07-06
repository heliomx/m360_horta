# Adaptador RFM95W → soquete 2x4 (NRF24L01 / placa YL-105)

Placa compacta (28 x 28 mm) com o módulo RFM95W soldado de um lado e um header macho
2x4 (2.54mm, THT) do outro, para plugar direto no soquete fêmea da placa YL-105
(a mesma usada para o NRF24L01+), conforme as duas imagens de referência.

## Arquivos

- `adapter_rfm95w_nrf24.kicad_sch` — esquemático (KiCad, formato nativo S-expression)
- `adapter_rfm95w_nrf24.kicad_pcb` — placa (KiCad, formato nativo S-expression, com os pads de solda)
- `M360_RFM95W.pretty/RFM95W_SMD_16P.kicad_mod` — footprint do módulo RFM95W (biblioteca local), 16 pads
  SMD/castelados, 2 mm de passo, corpo 16x16 mm, gerado a partir do desenho mecânico da HopeRF
  (`docs/RFM95_96_97_98W.pdf`, Seção 1.4 "Pin Description" e Figura 56 "Package Outline Drawing")

> **Nota sobre o formato:** o KiCad não usa JSON para esquemáticos/placas — o formato nativo
> (`.kicad_sch`, `.kicad_pcb`) é S-expression. É o formato correto para abrir no KiCad 7/8.

## Layout da placa

- **U1** — RFM95W, footprint com os 16 pads reais nas bordas superior/inferior (8 de cada lado),
  pino 1 marcado no canto inferior esquerdo do footprint.
- **J1** — header macho 2x4, 2.54mm, THT — pluga direto no soquete fêmea da YL-105 (segunda
  imagem de referência: GND, VCC 3.3V, NSS, MOSI, DIO0, SCK, MISO, GND, VCC 5V).

## Mapeamento de sinais

| Pino do soquete J1 (padrão NRF24L01, 2x4) | Sinal no RFM95W (U1) | Observação |
|---|---|---|
| 1 GND   | GND (pinos 1, 8, 10)   | comum a todos os GNDs do módulo |
| 2 VCC   | 3.3V (pino 13)    | a YL-105 já regula para 3.3V via AMS1117 on-board — seguro para o RFM95W |
| 3 CE    | **NC** (não conectado) | conforme solicitado |
| 4 CSN   | **NSS** (pino 5)  | reaproveitado como chip-select SPI, conforme solicitado |
| 5 SCK   | SCK (pino 4)      | |
| 6 MOSI  | MOSI (pino 3)     | |
| 7 MISO  | MISO (pino 2)     | |
| 8 IRQ   | DIO0 (pino 14)    | interrupção principal (TxDone/RxDone) — assumido, veja nota abaixo |
| —       | RESET (pino 6)    | **NC**, conforme solicitado |
| —       | DIO1, DIO3, DIO4, DIO5 (pinos 15, 11, 12, 7) | NC — não usados neste adaptador |
| —       | ANT (pino 9)      | pad exposto na borda do footprint — solde o fio/whip da antena direto aqui (sem conector dedicado) |

## Footprint do RFM95W (U1) — numeração real (datasheet Seção 1.4)

| Pino | Sinal | Pino | Sinal |
|---|---|---|---|
| 1 | GND   | 9  | ANT (antena) |
| 2 | MISO  | 10 | GND |
| 3 | MOSI  | 11 | DIO3 (NC) |
| 4 | SCK   | 12 | DIO4 (NC) |
| 5 | NSS   | 13 | 3.3V |
| 6 | RESET (NC) | 14 | DIO0 |
| 7 | DIO5 (NC)  | 15 | DIO1 (NC) |
| 8 | GND   | 16 | DIO2 (NC) |

## Suposições assumidas (por favor revise)

- **IRQ → DIO0**: não foi especificado explicitamente; DIO0 é o pino de interrupção mais comum
  em bibliotecas LoRa (RadioHead, LMIC, Arduino-LoRa) para sinalizar fim de TX/RX.
- Capacitores de desacoplamento (100 nF + 10 µF entre 3V3 e GND, próximos a U1) não foram
  incluídos neste layout — recomendação padrão do datasheet HopeRF, adicione antes de fabricar.
- O footprint do RFM95W usa pads SMD retangulares (1.2 x 2.4 mm) para aproximar os pads
  castelados reais do módulo; ajuste as dimensões se o seu breakout específico divergir.

## Próximos passos no KiCad

1. Abra `adapter_rfm95w_nrf24.kicad_sch` e `adapter_rfm95w_nrf24.kicad_pcb` no KiCad (a pasta
   `M360_RFM95W.pretty` deve estar configurada como biblioteca de footprints do projeto).
2. Confirme o footprint de U1 contra o breakout RFM95W físico que você possui.
3. Adicione C1/C2 (desacoplamento 3V3) próximos a U1, se desejar.
4. Roteie as trilhas (atenção especial à trilha da antena — trate como linha de RF, mantenha
   curta e casada a 50 Ω).
