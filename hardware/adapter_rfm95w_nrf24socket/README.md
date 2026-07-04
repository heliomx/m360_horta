Adaptador RFM95W → soquete 2x4 (NRF24L01 / placa YL-105)

Arquivos


rfm95_nrf24_adapter.kicad_sch — esquemático (KiCad, formato nativo S-expression, gerado com kiutils)
rfm95_nrf24_adapter.kicad_pro — projeto KiCad para abrir o esquemático



Nota sobre o formato: o KiCad não usa JSON para esquemáticos/placas — o formato nativo
(.kicad_sch, .kicad_pcb) é S-expression. O kiutils foi usado exatamente para gerar esse
formato nativo (é a biblioteca Python padrão para isso). O .kicad_pro (esse sim) é JSON.



Mapeamento de sinais

Pino do soquete J1 (padrão NRF24L01, 2x4)Sinal no RFM95WObservação1 GNDGNDcomum a todos os GNDs do módulo (pinos 1, 8, 10)2 VCC3.3V (pino 13)a YL-105 já regula para 3.3V via AMS1117 on-board — seguro para o RFM95W3 CENSS (pino 5)reaproveitado conforme solicitado4 CSNNC (não conectado)não usado nesta adaptação — assumido, veja nota abaixo5 SCKSCK (pino 4)6 MOSIMOSI (pino 3)7 MISOMISO (pino 2)8 IRQDIO0 (pino 14)interrupção principal (TxDone/RxDone) — assumido, veja nota abaixo—RESET (pino 6)NC, conforme solicitado—DIO1, DIO3, DIO4, DIO5NC — não usados neste adaptador—ANT (pino 9)levado a um conector/pad de antena (ANT1)

Suposições assumidas (por favor revise)


CSN: você só especificou CE→NSS e RST não usado. Como CSN não tinha destino definido,
deixei como não conectado. Se seu firmware ainda espera togglear um pino de "chip select"
separado, me avise para eu reatribuir.
IRQ → DIO0: não foi especificado; DIO0 é o pino de interrupção mais comum em bibliotecas
LoRa (RadioHead, LMIC, Arduino-LoRa) para sinalizar fim de TX/RX.
Adicionei dois capacitores de desacoplamento (C1 100 nF, C2 10 µF) entre 3V3 e GND próximos
ao RFM95W — recomendação padrão do datasheet HopeRF para estabilidade do regulador interno do rádio.
O footprint do RFM95W foi deixado como referência de texto (RF_Module:HopeRF_RFM9XW_SMD);
confirme se sua instalação do KiCad tem essa biblioteca ou associe o footprint correto no
editor de símbolos/PCB antes de rotear.


Próximos passos no KiCad


Abra rfm95_nrf24_adapter.kicad_pro.
Confirme/edite os footprints de J1, U1, C1, C2 e ANT1.
Gere o PCB (Ferramentas → Atualizar PCB a partir do esquemático) e roteie as trilhas
(atenção especial à trilha da antena — trate como linha de RF, mantenha curta e casada a 50 Ω).