"""Posicionamento da M360-LP no formato imposto pela caixa de 100 x 100 x 50 mm.

Placa 86 x 62 mm em (100,100). O plugue RJ45 é crimpado dentro da caixa e pede ~25-30 mm
livres à frente dos conectores, com os prensa-cabos na parede em frente: por isso a placa é
larga e baixa, encostada no lado oposto ao dos cabos.

Idempotente: reposiciona tudo e redesenha o contorno a cada execução.
Uso: <KiCad>/bin/python.exe place_lp.py <placa.kicad_pcb>
"""
import os
import sys

import pcbnew

X0, Y0, W, H = 100.0, 100.0, 86.0, 62.0

POS = {}


def put(ref, x, y, rot=0):
    POS[ref] = (x, y, rot)


# --- faixa de cima: Pro Mini deitado (FTDI para a esquerda) e mezanino com o SMA na borda ---
put("A1", 112.0, 121.0, 90)                # ocupa x 108,5-142,2 / y 104,2-122,6
put("J4", X0 + W - 27.2, 101.0 + 6.42)     # borda do mezanino (lado do SMA) = borda da placa
put("U2", 147.0, 112.0)                    # 74HC4051 entre o Pro Mini e o mezanino
put("C7", 147.0, 105.5)
put("R3", 147.0, 118.5)

# --- divisor da bateria, logo abaixo do furo A6 do Pro Mini --------------------------------
put("R1", 134.0, 125.5)
put("R2", 138.0, 125.5)
put("C4", 142.0, 125.5)

# --- matriz RC dos 8 canais de solo --------------------------------------------------------
for k in range(8):
    x = 106.0 + 4.2 * k
    put(f"R{6 + k}", x, 129.5, 270)        # VSW -> SOILk
    put(f"R{14 + k}", x, 134.5, 270)       # SOILk -> SOILk_F
    put(f"C{8 + k}", x, 139.5, 270)        # SOILk_F -> GND

# --- pull-ups das linhas digitais ----------------------------------------------------------
for i, ref in enumerate(("R22", "R23", "R24", "R25")):
    put(ref, 155.0 + 4.0 * i, 130.0, 270)

# --- proteção ESD logo atrás dos RJ45 ------------------------------------------------------
put("U3", 112.0, 143.5)
put("U4", 124.0, 143.5)
put("U5", 136.0, 143.5)

# --- RJ45 na borda de baixo, boca para fora ------------------------------------------------
put("J1", 116.0, Y0 + H - 8.66)
put("J2", 134.0, Y0 + H - 8.66)

# --- alimentação e trilho VSW: embaixo do mezanino, que fica a ~11 mm de altura -------------
# Só SMD baixo aqui. O MCP1700 é linear (não injeta ruído no rádio) e o P-MOSFET do VSW chaveia
# devagar. As três fileiras desviam do soquete J4 (x 154,4-160,6).
put("Q1", 165.0, 104.0)                    # proteção contra inversão
put("C1", 169.5, 104.0, 270)
put("U1", 174.0, 104.0)                    # MCP1700
put("C2", 178.0, 104.0, 270)
put("Q2", 165.0, 110.0)                    # chave do VSW
put("R4", 169.5, 110.0, 270)
put("C5", 172.5, 110.0, 270)
put("R5", 175.5, 110.0, 270)
put("C6", 178.5, 110.0, 270)
put("C3", 165.0, 116.0, 270)

# --- 100 ohm em série entre o TVS U5 e o MCU nas quatro digitais de campo ------------------
# 1-Wire, DHT, SDA e SCL (ARQUITETURA §4.2). Na faixa livre entre o U5 e os pull-ups R22-R25,
# mesmo passo e orientação dos pull-ups.
for i, ref in enumerate(("R26", "R27", "R28", "R29")):
    put(ref, 141.5 + 3.0 * i, 136.0, 270)

# --- entrada e reed -------------------------------------------------------------------------
put("J3", 178.0, 140.0)                    # J_VIN na borda direita
# Reed de volta (2026-09-13), na posição que tinha antes do botão de um dia: 11,5 x 2,4 mm.
put("SW1", 160.0, 158.0)                   # reed no canto de baixo à direita

HOLES = {"H1": (103.5, 103.5), "H2": (182.5, 128.0), "H3": (103.5, 158.5), "H4": (182.5, 158.5)}
KICAD_FP = os.environ.get("KICAD10_FOOTPRINT_DIR",
                          r"C:\Program Files\KiCad\10.0\share\kicad\footprints")
HOLE_LIB = os.path.join(KICAD_FP, "MountingHole.pretty")


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    # Trocar o footprint de um componente NÃO é feito aqui: remover/adicionar footprint pelo
    # SWIG do pcbnew provoca falha de segmentação. O caminho é apagar o bloco do arquivo
    # (del_fp.py) e deixar a sincronização com o esquemático recriá-lo da biblioteca.
    board = pcbnew.LoadBoard(path)
    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}

    faltando = sorted(set(fps) - set(POS) - set(HOLES))
    if faltando:
        print("sem posição:", ", ".join(faltando))
    ausentes = sorted(set(POS) - set(fps))
    if ausentes:
        print("não estão na placa:", ", ".join(ausentes))

    for ref, (x, y, rot) in POS.items():
        fp = fps.get(ref)
        if fp is None:
            continue
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(v(x, y))

    # furos de fixação M3 (só na placa; fora da BOM e do arquivo de posição)
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

    # contorno: apaga o Edge.Cuts anterior e redesenha o retângulo
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
