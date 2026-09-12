"""Posicionamento da M360-ACT (rodar com o Python do KiCad).

A placa é parafusada na chapa perfurada do quadro, onde há espaço de sobra: ao contrário da
LP, o contorno não é apertado. Placa de 100 x 80 mm em (100,100).

Arranjo:
  - borda de baixo: os três RJ45 com a boca para fora, e os cinco TVS logo atrás deles;
  - faixa do meio: 74HCT244 (U2) e CD74HC4067 (U3), com seus desacoplamentos;
  - alto à esquerda: Pro Mini deitado, com o header FTDI apontando para fora da borda;
  - alto à direita: mezanino, com o SMA saindo pela borda (a antena fica dentro do quadro);
  - coluna da esquerda: entrada de 5 V no borne, LDO e capacitores;
  - borda direita: reed (o ímã é aproximado pela parede) e o divisor da vazão.

Idempotente: reposiciona tudo e redesenha o contorno a cada execução.
Uso: <KiCad>/bin/python.exe place_act.py <placa.kicad_pcb>
"""
import os
import sys

import pcbnew

X0, Y0, W, H = 100.0, 100.0, 100.0, 80.0

POS = {}


def put(ref, x, y, rot=0):
    POS[ref] = (x, y, rot)


# --- alto: Pro Mini (FTDI para fora da borda esquerda) e mezanino (SMA pela borda direita) --
put("A1", 115.0, 126.0, 90)                 # ocupa x 111,5-145,2 / y 109,2-127,6
put("J2", X0 + W - 27.2, 102.0 + 6.42)      # borda do mezanino (lado do SMA) = borda da placa
put("R1", 150.0, 112.0, 270)                # pull-up do NSS
put("R2", 150.0, 120.0, 270)                # pull-up do /OE do 74HCT244

# --- coluna da esquerda: entrada de 5 V, LDO e capacitores ---------------------------------
# O borne abre para −x (as guias de fio ficam em x ≈ −5,68 no footprint). Em rot=90 a abertura
# apontava para baixo, para DENTRO da placa; em rot=0 ela fica voltada para a borda esquerda.
put("J1", 106.0, 150.0, 0)                  # borne: fio do barramento entra pela borda
put("U1", 106.0, 138.0)                     # AMS1117-3.3
put("C1", 112.0, 138.0, 270)                # 10 uF na entrada de 5 V
put("C2", 112.0, 142.0, 270)                # 100 nF em 5 V
put("C3", 106.0, 132.0, 270)                # 10 uF em 3,3 V
put("C4", 112.0, 132.0, 270)                # 100 nF em 3,3 V

# --- faixa do meio: fronteira de nível e multiplexador --------------------------------------
put("U2", 125.0, 140.0)                     # 74HCT244
put("C5", 135.0, 132.0, 270)
put("U3", 155.0, 140.0)                     # CD74HC4067
put("C6", 165.0, 132.0, 270)

# --- proteção ESD, logo atrás dos RJ45 ------------------------------------------------------
for i, ref in enumerate(("U4", "U5", "U6", "U7", "U8")):
    put(ref, 115.0 + 12.0 * i, 160.5)

# --- RJ45 na borda de baixo, boca para fora (corpo de y−6,58 a y+8,66) ----------------------
put("J3", 120.0, Y0 + H - 8.66)             # RELES-A: canais 0 a 5
put("J4", 140.0, Y0 + H - 8.66)             # RELES-B: canal 6, NFT, OXI, canais 7 a 9
put("J5", 160.0, Y0 + H - 8.66)             # SENSORES: vazão e A6

# --- divisor da vazão, junto do J5, e reed na borda direita ---------------------------------
put("R3", 176.0, 158.0, 270)
put("R4", 176.0, 164.0, 270)
put("C7", 182.0, 161.0, 270)
# Botão de repareamento (era reed). Sem rotação: o SW_PUSH_6mm é quase quadrado (9,6 x 7,6) e
# em rot=0 cabe na faixa livre da borda direita, entre o mezanino e o divisor da vazão.
put("SW1", 190.0, 145.0, 0)

# H2 não fica no canto: ali ele cairia embaixo do mezanino, e a placa teria de ser parafusada
# antes de encaixar o módulo. Desce para a faixa livre entre o mezanino e o reed.
HOLES = {"H1": (104.0, 104.0), "H2": (X0 + W - 4.0, 131.0),
         "H3": (104.0, Y0 + H - 4.0), "H4": (X0 + W - 4.0, Y0 + H - 4.0)}
KICAD_FP = os.environ.get("KICAD10_FOOTPRINT_DIR",
                          r"C:\Program Files\KiCad\10.0\share\kicad\footprints")
HOLE_LIB = os.path.join(KICAD_FP, "MountingHole.pretty")


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    board = pcbnew.LoadBoard(path)
    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}

    faltando = sorted(set(fps) - set(POS) - set(HOLES))
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
        fp.SetPosition(v(x, y))

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
    print(f"{len(POS)} footprints posicionados, {len(HOLES)} furos, contorno {W:g} x {H:g} mm")


if __name__ == "__main__":
    main(sys.argv[1])
