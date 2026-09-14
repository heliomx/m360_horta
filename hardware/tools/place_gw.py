"""Posicionamento da M360-GW (rodar com o Python do KiCad).

Placa de 80 x 60 mm em (100,100). Montagem 100% THT (ARQUITETURA §6.2), o que domina o
arranjo: cada resistor axial ocupa 12,4 x 3,1 mm contra 1,6 x 0,8 de um 0603, então os
passivos ganham faixas próprias em vez de se espremerem junto ao MCU.

Duas restrições mandam no desenho, e nenhuma das duas aparece no courtyard:

  - o **mezanino** M360-RF ocupa 34,2 x 23 mm *acima* do soquete (contorno na F.Fab, §3).
    O soquete sozinho tem courtyard de 6,2 x 13,8 — confiar nele colocaria peças embaixo do
    módulo. O SMA fica na borda direita: por isso J1 vai em x = borda − 27,2 mm;
  - o **D1 mini** traz uma zona de keep-out sobre a antena, na ponta onde também fica o USB.
    Essa ponta encosta na borda de cima: a antena não irradia para dentro do cobre e o cabo
    USB entra sem abrir a caixa.

Os furos M3 não ficam nos quatro cantos: o canto superior direito cairia embaixo do mezanino
(a placa teria de ser parafusada antes de encaixar o módulo) e o superior esquerdo, embaixo do
D1 mini. H2 desce para a faixa livre sob o mezanino.

Idempotente: reposiciona tudo e redesenha o contorno a cada execução.
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

# --- desacoplamento, junto dos pinos de alimentação do módulo -------------------------------
put("C1", 106.0, 138.0)                     # 10 uF no 3,3 V; subiu 2 mm para dar lugar ao J2 a 90°
put("C2", 115.0, 140.0)                     # 100 nF no 3,3 V
put("C3", 140.0, 130.0)                     # 100 nF no 5 V, entre o D1 mini e o mezanino

# --- entrada de 5 V na borda esquerda -------------------------------------------------------
# Borne C8465: no datasheet (corte A-A) a entrada do fio fica a 5,5 mm do pino e o fundo a 4,5 mm;
# no footprint a entrada é o lado -y (courtyard de -5,5 a +4,5). Em rot 0 ela olhava para dentro
# da placa. Rot 90 leva o -y para -x: entrada a 0,5 mm da borda esquerda (mesma correção da ACT).
# Cabe entre o C1 (termina em y 141,4) e o H3 (começa em 152,5): courtyard y 141,8-152,2.
put("J2", 106.0, 147.0, 90)

# --- LEDs na borda de baixo (visíveis) e seus resistores logo acima -------------------------
# Resistores girados 90°: em pé ocupam 3,1 mm de largura e cabem no passo de 8 mm dos LEDs.
# Atenção ao girar: o resistor NÃO fica centrado na origem. Com rot=90 o courtyard vai de
# y − 11,25 a y + 1,10, ou seja, quase todo ACIMA do ponto informado. Em y = 143 a ponta
# subia até 131,75 e entrava no courtyard do U1, que termina em 134,79 — quatro das cinco
# violações do primeiro DRC vinham daí. Em y = 148 a fileira ocupa 136,75 a 149,10: ~2 mm
# abaixo do U1 e ~3,6 mm acima dos LEDs.
for i, r in enumerate(("R1", "R2", "R3")):
    put(r, 128.0 + 8.0 * i, 148.0, 90)

# Os LEDs ficam no painel da caixa, não na placa: aqui só os resistores de limitação, e o J3
# leva os três pares para fora. 6 vias em pares ânodo/cátodo porque o verde é ativo em LOW —
# o cátodo dele vai ao D4, não ao GND, então não existe terra comum aos três (§6.1).
# Courtyard do JST XH: de x − 2,95 a x + 15,45, ou seja, quase todo à direita da origem.
put("J3", 130.0, 154.0)

# --- botão de AP e o pull-up do A0, na faixa livre sob o mezanino ---------------------------
put("SW1", 152.0, 152.0)
put("R4", 165.0, 140.0, 90)
put("C4", 172.0, 140.0)

# H1 evita o D1 mini; H2 não fica no canto (cairia sob o mezanino) e desce para a faixa livre.
HOLES = {"H1": (104.0, 104.0), "H2": (X0 + W - 4.0, 130.0),
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
        # Sem serigrafia: o furo não entra na BOM nem no arquivo de posição, então o texto não
        # serve para nada — e o do H1, a 4 mm das duas bordas, era cortado pelo Edge.Cuts.
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
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
