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
# U3 e U5 a 180 graus: com o leque fixo do leque_tvs.py (GND e VSW nos pads do meio), a ordem
# dos lados de chegada fica mais próxima da ordem dos pinos no RJ45 — a 0 grau saía invertida.
put("U3", 112.0, 143.5, 180)
put("U4", 124.0, 143.5)
put("U5", 136.0, 143.5, 180)

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
# C3 (10 µF) junto da saída do U1: a revisão de placa de 2026-09-14 mediu 21,8 mm de cobre entre
# ele e o U1.2 na posição antiga (165; 116). Rot 180 deixa o pad de 3V3 do lado do U1.
put("C3", 171.0, 107.2, 180)

# --- 100 ohm em série entre o TVS U5 e o MCU nas quatro digitais de campo ------------------
# 1-Wire, DHT, SDA e SCL (ARQUITETURA §4.2). Logo ACIMA do U5, pad 2 (campo) para baixo: o leque
# do leque_tvs.py tira o lado MCU de cada rede por cima do TVS. Ao lado do U5 (posição anterior)
# eles ficavam no caminho das pernas RJ45 -> TVS e o roteador os cercava. Ordem da esquerda para
# a direita igual à das saídas do leque: SDA (topo esquerdo), SCL (centro), DHT (topo direito),
# 1-Wire (via lateral direita).
put("R28", 133.3, 139.8, 270)              # SDA, entre o C14 e o C15
put("R29", 139.0, 139.8, 270)              # SCL
put("R27", 140.9, 139.8, 270)              # DHT
put("R26", 142.8, 139.8, 270)              # ONEWIRE

# --- entrada e reed -------------------------------------------------------------------------
put("J3", 178.0, 140.0)                    # J_VIN na borda direita

# --- header 1x8 do nRF24 legado (ARQUITETURA §2 e §4.2): exclusivo com o mezanino -------
# Único vão livre: à direita do J2 (borda em 142), abaixo da fileira R26-R29 (y 139,8) e fora da
# área sem trilha em B.Cu. Em rot 90 os pinos correm em +x: pino 1 (CE = DIO0) em 147,1, pino 8 em
# 164,9. A marca e o nome do modelo sobem para y 143,5 / 147,0 (silk_lp.py). O 10 uF fica à
# direita, acima do fim do header (o courtyard do header vai até x 166,7) e antes do J3.
put("J5", 147.1, 152.0, 90)
put("C16", 169.0, 148.0, 270)              # 10 uF do 3,3 V do nRF24, na ponta do cabo
# Reed na borda esquerda (2026-09-14). Na borda de baixo ele ficava a ~30 mm da parede (é o lado
# do espaço do plugue RJ45), fora do alcance do ímã. A placa tem 86 mm num interior de ~92 mm:
# nas laterais sobram ~3 mm até a parede. Faixa livre entre o Pro Mini (y > 122,6) e o H3.
put("SW1", 101.6, 139.0, 90)

# Fiduciais para a montagem SMT (só na placa; fora do BOM e da posição).
FIDUCIAIS = {"FID1": (104.5, 123.5), "FID2": (184.0, 149.0), "FID3": (173.0, 160.0)}  # FID2 saiu de (150; 102,2): colava na trilha de 3V3; FID1 de (104,5; 125): na trilha do REED
FID_LIB = None  # definido abaixo, depois de KICAD_FP

# Área sem trilha em B.Cu sob os RJ45: mantém o plano de terra de baixo inteiro sob os conectores.
# A revisão de placa achou o GND do J1.8 numa ilha que saía por uma via só, porque as saídas dos
# pinos em B.Cu recortavam o plano. Vias e preenchimento continuam permitidos.
# Começa em y 150,6 (0,9 mm acima da fileira de pads), não mais em 146,3: entre os TVS e os RJ45
# as pernas precisam de uma faixa em B.Cu para se cruzarem — com a atribuição de canais dos TVS,
# S4 cruza S1 e S0 no J1, e S6 cruza SCL, VSW e S7 no J2.
# Recorte sob a metade direita do J1 (x 114,9-122,2, até y 157,2): os pinos 5 e 3 são da fileira de
# baixo e não passam entre os pads da fileira de cima (vão de 0,52 mm). O SOIL2 sai por baixo, pela
# esquerda, e fecha o J1.5; o SOIL5 escapa em B.Cu por baixo do J1.3 e do J1.1.
KEEPOUT_RJ45 = ("keepout_bcu_rj45", [(106.0, 150.6), (114.9, 150.6), (114.9, 157.2), (122.2, 157.2),
                                     (122.2, 150.6), (146.0, 150.6), (146.0, 162.0), (106.0, 162.0)])

HOLES = {"H1": (103.5, 103.5), "H2": (182.5, 128.0), "H3": (103.5, 158.5), "H4": (182.5, 158.5)}
KICAD_FP = os.environ.get("KICAD10_FOOTPRINT_DIR",
                          r"C:\Program Files\KiCad\10.0\share\kicad\footprints")
HOLE_LIB = os.path.join(KICAD_FP, "MountingHole.pretty")
FID_LIB = os.path.join(KICAD_FP, "Fiducial.pretty")


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    # Trocar o footprint de um componente NÃO é feito aqui: remover/adicionar footprint pelo
    # SWIG do pcbnew provoca falha de segmentação. O caminho é apagar o bloco do arquivo
    # (del_fp.py) e deixar a sincronização com o esquemático recriá-lo da biblioteca.
    board = pcbnew.LoadBoard(path)
    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}

    faltando = sorted(set(fps) - set(POS) - set(HOLES) - set(FIDUCIAIS))
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

    for ref, (x, y) in FIDUCIAIS.items():
        fp = fps.get(ref)
        if fp is None:
            fp = pcbnew.FootprintLoad(FID_LIB, "Fiducial_1mm_Mask2mm")
            fp.SetReference(ref)
            board.Add(fp)
        fp.SetBoardOnly(True)
        fp.SetExcludedFromBOM(True)
        fp.SetExcludedFromPosFiles(True)
        fp.Reference().SetVisible(False)   # referência na borda viraria silk_edge_clearance
        fp.SetPosition(v(x, y))

    # Reaproveita a área se ela já existe (apagar item pelo SWIG é caminho conhecido de falha) e
    # monta o contorno num SHAPE_POLY_SET próprio: o Outline() da zona recém-criada volta como
    # objeto SWIG sem os métodos de polígono.
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
