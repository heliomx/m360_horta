"""Posicionamento da M360-GW (rodar com o Python do KiCad).

Placa de 80 x 60 mm em (100,100), montada em SMD pela JLCPCB desde 2026-09-14 (ARQUITETURA
§6.2); só U1 (D1 mini), J1 (soquete do mezanino), J2 (borne) e J3 (JST dos LEDs) são soldados à
mão. As restrições que mandam no desenho, e nenhuma delas aparece no courtyard:

  - o **mezanino** M360-RF ocupa 34,2 x 23 mm *acima* do soquete (contorno na F.Fab, §3). O
    SMA fica na borda direita: por isso J1 vai em x = borda − 27,2 mm. Embaixo dele só SMD baixo;
  - o **D1 mini** traz uma zona de keep-out sobre a antena, na ponta onde também fica o USB.
    Essa ponta encosta na borda de cima: a antena não irradia para dentro do cobre e o cabo USB
    entra sem abrir a caixa;
  - o **borne** J2 tem a entrada do fio no lado −y do footprint (datasheet C8465): rot 90 a leva
    para a borda esquerda;
  - o **reed** SW1 é acionado por ímã através da parede: vai na borda esquerda, a 1 mm dela, junto
    do pino A0 — **essa borda da placa precisa ficar rente à parede da caixa**.

Os furos M3 não ficam nos quatro cantos: o canto superior direito cairia embaixo do mezanino
(a placa teria de ser parafusada antes de encaixar o módulo) e o superior esquerdo, embaixo do
D1 mini. H2 desce para a faixa livre sob o mezanino.

Idempotente: reposiciona tudo, os furos, os fiduciais e o contorno a cada execução.
Uso: <KiCad>/bin/python.exe place_gw.py <placa.kicad_pcb>
"""
import os
import sys

import pcbnew

X0, Y0, W, H = 100.0, 100.0, 80.0, 60.0

POS = {}


def put(ref, x, y, rot=0):
    POS[ref] = (x, y, rot)


# --- alto: D1 mini à esquerda (antena/USB na borda de cima) e mezanino à direita ------------
# U1: courtyard 26,2 x 34,8 com origem no pino 1; a ponta da antena fica em y − 8,5.
put("U1", 110.0, 108.5)                     # ocupa x 108,3-134,5 / y 100,0-134,8
# J1: borda direita do mezanino = borda da placa, para o SMA sair pela lateral.
put("J1", X0 + W - 27.2, Y0 + 6.42)         # mezanino ocupa x 145,8-180,0 / y 100,0-123,0
put("R5", 140.0, 110.0, 90)                 # 100 k do MOSI ao GND, entre o D1 mini e o soquete

# --- coluna da esquerda: reed na borda, A0 e 3,3 V ao lado dos pinos do módulo --------------
put("SW1", X0 + 1.6, 120.0, 90)             # reed a 1 mm da borda, na altura do A0 (U1.2)
put("R4", 105.0, 112.5, 90)                 # 10 k do 3,3 V ao A0
put("C4", 105.0, 116.5, 90)                 # 100 nF no A0
put("C2", 105.0, 122.5, 90)                 # 100 nF no 3,3 V
put("C1", 105.0, 126.5, 90)                 # 10 uF no 3,3 V, junto do U1.8

# --- entrada de 5 V: borne na borda esquerda, cadeia SS14 -> PTC -> TVS até o U1.9 ----------
# Borne C8465: a entrada do fio fica a 5,5 mm do pino (datasheet, corte A-A), o lado -y do
# footprint; rot 90 leva o -y para -x: entrada a 0,5 mm da borda esquerda.
put("J2", 106.0, 147.0, 90)                 # courtyard x 100,5-110,5 / y 141,8-152,2
put("D1", 117.0, 139.5)                     # SS14: anodo no VIN_RAW (borne), catodo no VIN_POL
put("F1", 124.0, 139.5)                     # PTC 500 mA
put("D2", 131.0, 139.5)                     # SMAJ5.0A do +5V ao GND, depois do F1
put("C3", 137.5, 128.0, 90)                 # 100 nF no 5 V, junto do U1.9

# --- LEDs do painel: resistores e o 2N7002 do verde logo acima do J3 -----------------------
for i, r in enumerate(("R1", "R2", "R3")):
    put(r, 131.0 + 2.0 * i, 147.5, 90)
put("Q1", 140.5, 147.5)
# Courtyard do JST XH: de x − 3,0 a x + 15,5, ou seja, quase todo à direita da origem.
put("J3", 130.0, 154.0)

# H1 evita o D1 mini; H2 não fica no canto (cairia sob o mezanino) e desce para a faixa livre.
HOLES = {"H1": (104.0, 104.0), "H2": (X0 + W - 4.0, 130.0),
         "H3": (104.0, Y0 + H - 4.0), "H4": (X0 + W - 4.0, Y0 + H - 4.0)}
FIDUCIAIS = {"FID1": (140.0, 103.5), "FID2": (170.0, 140.0), "FID3": (116.0, 157.0)}
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
        # Sem serigrafia: o furo não entra na BOM nem no arquivo de posição, então o texto não
        # serve para nada — e o do H1, a 4 mm das duas bordas, era cortado pelo Edge.Cuts.
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
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
    print(f"{len(POS)} footprints posicionados, {len(HOLES)} furos, {len(FIDUCIAIS)} fiduciais, contorno {W:g} x {H:g} mm")


if __name__ == "__main__":
    main(sys.argv[1])
