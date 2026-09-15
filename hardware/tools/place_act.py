"""Posicionamento da M360-ACT (rodar com o Python do KiCad).

A placa é parafusada na chapa perfurada do quadro, onde há espaço de sobra: ao contrário da
LP, o contorno não é apertado. Placa de 100 x 80 mm em (100,100).

Arranjo (refeito em 2026-09-14 sobre o esquemático corrigido, ARQUITETURA §5.2 e §11.8):
  - borda de baixo: os três RJ45 com a boca para fora; atrás de cada um, um PAR de TVS a 180°
    (U4+U5, U6+U7, U8 sozinho), na mesma distância da LP (9,84 mm do centro do jack). Os canais
    dos TVS seguem a ordem física dos pinos, então as pernas RJ45 -> TVS saem sem cruzar;
  - logo acima dos TVS, a fileira dos 100 ohm (R21-R32) e dos divisores da J5, em pé, na ordem
    em que as linhas saem dos TVS (não pelo número do canal);
  - faixa do meio: CD74HC4067 (U3) com I0-I7 virados para baixo, 74HCT244 (U2) à direita, os
    100 k do lado de 5 V (R7-R11) entre os dois e os do lado de 3,3 V (R14-R20) à direita do U2;
  - alto: Pro Mini deitado (FTDI para a borda esquerda) e mezanino (SMA pela borda direita);
  - coluna da esquerda, acima do borne: Q1 (inversão) -> F1 (PTC) -> D2 (TVS de 5 V), e o
    ME6211 (U1) com os capacitores;
  - borda direita: reed (SW1), acionado por ímã através da parede do quadro.

Idempotente: reposiciona tudo, os furos, os fiduciais, a área sem trilha e o contorno.
Uso: <KiCad>/bin/python.exe place_act.py <placa.kicad_pcb>
"""
import os
import sys

import pcbnew

X0, Y0, W, H = 100.0, 100.0, 93.0, 80.0   # 93 mm desde 2026-09-14 (ajuste do usuário na placa)

POS = {}


def put(ref, x, y, rot=0):
    POS[ref] = (x, y, rot)


# --- alto: Pro Mini (FTDI para fora da borda esquerda) e mezanino (SMA pela borda direita) --
put("A1", 115.0, 126.0, 90)                 # ocupa x 111,5-145,1 / y 109,2-127,6
put("J2", 171.96, 127.46, 90)              # soquete do mezanino a 90°, posição definida pelo usuário na placa
put("R1", 163.0, 112.0, 270)                # pull-up do NSS, fora do mezanino (x >= 165,9)

# --- coluna da esquerda: entrada de 5 V acima do borne --------------------------------------
# Borne: no datasheet (C8465, corte A-A) a entrada do fio fica a 5,5 mm do pino e o fundo a
# 4,5 mm — no footprint, a entrada é o lado -y (courtyard de -5,5 a +4,5). As marcas da serigrafia
# em x -5,68 são o perfil lateral, não a abertura: em rot 0 a entrada olhava para dentro da placa
# (revisão de placa de 2026-09-14). Rot 90 leva o -y para -x: entrada a 0,5 mm da borda esquerda.
put("J1", 106.0, 150.0, 90)
put("Q1", 104.0, 141.0)                     # P-MOSFET contra inversão, logo acima do borne
put("F1", 109.5, 141.0)                     # PTC 500 mA
put("D2", 116.0, 141.0)                     # SMAJ5.0A do +5V ao GND, depois do F1
put("C1", 103.0, 136.5, 90)                 # 10 uF em +5V
put("C2", 105.5, 136.5, 90)                 # 100 nF em +5V
put("U1", 109.5, 136.5)                     # ME6211C33
put("C3", 113.5, 136.5, 90)                 # 10 uF em 3V3
put("C4", 116.0, 136.5, 90)                 # 100 nF em 3V3

# --- faixa do meio: multiplexador, buffer de nível e seus pulls -----------------------------
# U3 a 90°: a fileira dos pinos 1-12 (SIG, I7..I0, S0, S1, GND) fica para baixo, de frente para
# os 100 ohm.
put("U3", 130.0, 140.0, 90)
put("C6", 130.0, 132.3)
for i, ref in enumerate(("R7", "R8", "R9", "R10", "R11")):   # S0-S3 para GND, SIG para +5V
    put(ref, 142.0, 135.5 + 1.8 * i)
put("U2", 153.0, 140.0, 90)
put("C5", 149.5, 131.8)
put("R2", 154.0, 131.8)                     # pull-up do /OE
for i, ref in enumerate(("R14", "R15", "R16", "R17", "R18", "R19", "R20")):  # lado de 3,3 V
    put(ref, 162.5, 134.9 + 1.8 * i)

# --- 100 ohm de série, na ordem de saída dos TVS --------------------------------------------
# Em pé (rot 270): pad 1 (lado do CI) em cima, pad 2 (lado do campo) embaixo.
Y_SERIE = 153.5
for i, ref in enumerate(("R26", "R24", "R25", "R22", "R21", "R23")):   # J3: CH5 CH3 CH4 CH1 CH0 CH2
    put(ref, 115.0 + 1.9 * i, Y_SERIE, 270)
for i, ref in enumerate(("R30", "R28", "R29", "R31", "R27", "R32")):   # J4: CH9 CH7 CH8 NFT CH6 OXI
    put(ref, 135.0 + 1.9 * i, Y_SERIE, 270)
put("R12", 140.7, 149.2, 270)               # +5V -> REL_NFT, acima do R31
put("R13", 144.5, 149.2, 270)               # +5V -> REL_OXI, acima do R32

# --- J5: vazão e entrada analógica ----------------------------------------------------------
put("R3", 157.0, Y_SERIE, 270)              # FLOW_5V -> FLOW_DIV
put("R4", 158.9, Y_SERIE, 270)              # FLOW_DIV -> GND
put("C7", 160.8, Y_SERIE, 270)
put("D1", 162.7, Y_SERIE, 270)              # BAT54WS FLOW_DIV -> 3V3
put("R5", 164.6, Y_SERIE, 270)              # AUX_A6_IN -> AUX_A6
put("R6", 166.5, Y_SERIE, 270)
put("C8", 168.4, Y_SERIE, 270)

# --- proteção ESD: par de TVS a 180° atrás de cada RJ45 -------------------------------------
Y_TVS = Y0 + H - 8.66 - 9.84                # mesma distância jack -> TVS da LP
put("U4", 117.5, Y_TVS, 180)                # J3: CH5 CH3 CH4 CH1 (+5V com perna)
put("U5", 124.0, Y_TVS, 180)                # J3: CH0 CH2
put("U6", 137.5, Y_TVS, 180)                # J4: CH9 CH7 CH8 NFT (+5V com perna)
put("U7", 144.0, Y_TVS, 180)                # J4: CH6 OXI
put("U8", 160.5, Y_TVS, 180)                # J5: FLOW AUX (+5V com perna)

# --- RJ45 na borda de baixo, boca para fora (corpo de y-6,58 a y+8,66) ----------------------
put("J3", 120.0, Y0 + H - 8.66)             # RELES-A: canais 0 a 5
put("J4", 140.0, Y0 + H - 8.66)             # RELES-B: canal 6, NFT, OXI, canais 7 a 9
put("J5", 160.0, Y0 + H - 8.66)             # SENSORES: vazão e A6

# --- reed na borda direita, entre o H2 e o H4 (a ACT fica dentro do quadro: o usuário abre a porta
#     e passa o ímã junto do SW1, §5.2) -------------------------------------------------------
put("SW1", 186.5, 152.2, 90)              # posição definida pelo usuário na placa

# --- header 1x8 do nRF24 legado (ARQUITETURA §2 e §5.2): exclusivo com o mezanino -------
# Coluna vertical logo abaixo do mezanino a 90° (que termina em y 134,5), entre a coluna
# R14-R20 / fileira Y_SERIE (x <= 170,2) e o SW1/H2 (x >= 183,5). Rot 0: pinos correm em +y, pino
# 1 (CE = DIO0) em cima. y 137,93 deixa a trilha de GND em y 151,9 a meio passo entre dois pinos
# (folga 0,32 mm); a trilha do REED em B.Cu (x 178,32) fica a 0,85 mm dos pads. O 10 uF fica à
# esquerda, na altura dos primeiros pinos.
put("J6", 176.5, 137.93, 0)
put("C9", 173.5, 141.0, 90)                 # 10 uF do 3,3 V do nRF24, na ponta do cabo

# H2 não fica no canto: ali ele cairia embaixo do mezanino, e a placa teria de ser parafusada
# antes de encaixar o módulo.
HOLES = {"H1": (104.0, 104.0), "H2": (186.5, 138.5),
         "H3": (104.0, Y0 + H - 4.0), "H4": (186.5, 174.5)}
FIDUCIAIS = {"FID1": (109.0, 104.0), "FID2": (160.0, 104.0), "FID3": (185.0, 165.0)}
# Área sem trilha em B.Cu sob a fileira de pads dos RJ45 (plano de baixo inteiro sob os
# conectores, §11.8); começa 0,9 mm acima da fileira de cima. Vias e preenchimento continuam.
KEEPOUT_RJ45 = ("keepout_bcu_rj45", [(110.0, 168.8), (170.0, 168.8), (170.0, 180.0), (110.0, 180.0)])

KICAD_FP = os.environ.get("KICAD10_FOOTPRINT_DIR",
                          r"C:\Program Files\KiCad\10.0\share\kicad\footprints")
HOLE_LIB = os.path.join(KICAD_FP, "MountingHole.pretty")
FID_LIB = os.path.join(KICAD_FP, "Fiducial.pretty")


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    board = pcbnew.LoadBoard(path)
    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}

    faltando = sorted(set(fps) - set(POS) - set(HOLES) - set(FIDUCIAIS))
    if faltando:
        print("sem posição definida:", ", ".join(faltando))
    ausentes = sorted(set(POS) - set(fps))
    if ausentes:
        print("não estão na placa:", ", ".join(ausentes))

    for ref, (x, y, rot) in POS.items():
        fp = fps.get(ref)
        if fp is None:
            continue
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(v(x, y))

    for ref, (x, y) in HOLES.items():
        fp = fps.get(ref)
        if fp is None:
            fp = pcbnew.FootprintLoad(HOLE_LIB, "MountingHole_3.2mm_M3")
            fp.SetReference(ref)
            board.Add(fp)
        fp.SetBoardOnly(True)
        fp.SetExcludedFromBOM(True)
        fp.SetExcludedFromPosFiles(True)
        fp.Reference().SetVisible(False)   # a do H1 saía pela borda (silk_edge_clearance)
        fp.SetPosition(v(x, y))

    for ref, (x, y) in FIDUCIAIS.items():
        fp = fps.get(ref)
        if fp is None:
            fp = pcbnew.FootprintLoad(FID_LIB, "Fiducial_1mm_Mask2mm")
            fp.SetReference(ref)
            board.Add(fp)
        fp.SetBoardOnly(True)
        fp.SetExcludedFromBOM(True)
        fp.SetExcludedFromPosFiles(True)
        fp.Reference().SetVisible(False)
        fp.SetPosition(v(x, y))

    # Reaproveita a área se ela já existe e monta o contorno num SHAPE_POLY_SET próprio (ver
    # place_lp.py: apagar pelo SWIG e o Outline() de zona nova falham).
    nome, cantos_ko = KEEPOUT_RJ45
    ko = next((z for z in board.Zones() if z.GetIsRuleArea() and z.GetZoneName() == nome), None)
    if ko is None:
        ko = pcbnew.ZONE(board)
        board.Add(ko)
    ko.SetIsRuleArea(True)
    ko.SetZoneName(nome)
    ko.SetLayer(pcbnew.B_Cu)
    ko.SetDoNotAllowTracks(True)
    ko.SetDoNotAllowVias(False)
    ko.SetDoNotAllowPads(False)
    ko.SetDoNotAllowFootprints(False)
    ko.SetDoNotAllowZoneFills(False)
    poligono = pcbnew.SHAPE_POLY_SET()
    poligono.NewOutline()
    for x, y in cantos_ko:
        poligono.Append(pcbnew.FromMM(x), pcbnew.FromMM(y))
    ko.SetOutline(poligono)

    for d in list(board.GetDrawings()):
        if d.GetLayer() == pcbnew.Edge_Cuts:
            board.Remove(d)
    cantos = [(X0, Y0), (X0 + W, Y0), (X0 + W, Y0 + H), (X0, Y0 + H)]
    for (x1, y1), (x2, y2) in zip(cantos, cantos[1:] + cantos[:1]):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(v(x1, y1))
        seg.SetEnd(v(x2, y2))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(pcbnew.FromMM(0.1))
        board.Add(seg)

    pcbnew.SaveBoard(path, board)
    print(f"{len(POS)} footprints posicionados, {len(HOLES)} furos, {len(FIDUCIAIS)} fiduciais, "
          f"contorno {W:g} x {H:g} mm")


if __name__ == "__main__":
    main(sys.argv[1])
