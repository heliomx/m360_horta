# Datasheets

PDFs que fundamentaram decisões de hardware da família M360 (`hardware/ARQUITETURA_FAMILIA.md`).
Baixados da JLCPCB pela URL que o catálogo devolve (`get_jlcpcb_part`). Cada linha diz o que foi
conferido e em que página.

| LCSC | Peça | Arquivo | O que foi conferido |
|---|---|---|---|
| C315909 | Littelfuse 59170-1-S-00-D, reed SMD (gull wing, fita) | [C315909_Littelfuse_59170.pdf](C315909_Littelfuse_59170.pdf) | **p. 1:** corpo 11,43 × 2,29 × 2,29 mm; ponta a ponta dos terminais 16,25 mm, pé de 1,52 mm (de 6,61 a 8,13 mm do centro). O footprint `M360:Reed_Littelfuse_59170` tem pads 2,5 × 1,2 mm em ±7,80 mm (de 6,55 a 9,05 mm) e cobre o pé inteiro. **p. 2:** normalmente aberto; 0,5 A / 200 Vdc / 10 W; sensibilidade **S** (a mais sensível) = 10–15 AT, com distância de atuação média de **6,5 mm com o atuador 57045**. Outro ímã dá outra distância: medir com o ímã escolhido (§9) |
| C385834 | Cankemeng R-RJ45R08P-A004, RJ45 90° sem blindagem e sem LED | [C385834_Cankemeng_R-RJ45R08P-A004.pdf](C385834_Cankemeng_R-RJ45R08P-A004.pdf) | **p. 1** (desenho girado: ler com a página na horizontal): passo de 1,02 mm, fileiras a 1,78 mm; pegas Ø 3,25 mm a 12,70 mm entre si, 4,32 mm atrás da fileira de pinos; corpo 15,90 mm de largura × 15,10 mm de profundidade. Com pads de 1,52 mm no footprint, o vão entre dois pinos vizinhos da mesma fileira é de 0,52 mm, e não passa trilha de 0,25 mm com 0,2 mm de isolamento: os pinos da fileira de baixo não escapam por entre os de cima (ARQUITETURA §11.8) |
| C158012 | JST B2B-XH-A(LF)(SN), header THT de 2 vias, entrada por cima | [C158012_JST_B2B-XH-A.pdf](C158012_JST_B2B-XH-A.pdf) | **p. 1:** passo 2,5 mm; furo Ø 1,0 mm para 2 vias; 3 A / 250 V; placa de 1,6 mm. **p. 3:** B2B-XH-A com A = 2,5 mm e B = 7,4 mm; altura de 7 mm acima da placa |
