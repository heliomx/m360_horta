"""Serigrafia da M360-LP: tira os textos de referência de cima de pads e põe a marca da placa.

- U2, U4 e U5: o texto de referência ia parar sobre os pads de C7, C12 e C15. Vai para a
  folga livre ao lado de cada peça (o componente não se move).
- Acrescenta "Viridiotech.com.br" no espaço vazio acima do SW1.

Idempotente: reposiciona o texto se ele já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_lp.py <placa.kicad_pcb>
"""
import sys

import pcbnew

# referência -> (x, y, ângulo)
REFS = {
    "U2": (152.5, 112.0, 90),    # folga entre o corpo do U2 e o soquete do mezanino
    "U4": (129.5, 143.5, 0),     # vão entre U4 e U5
    "U5": (141.5, 143.5, 0),     # à direita do U5, antes dos pull-ups
    "R3": (147.0, 120.8, 0),     # abaixo do próprio corpo: em cima ele toca o contorno do U2
}

TEXTO = "Viridiotech.com.br"
TEXTO_POS = (160.0, 150.5)       # acima do SW1, cuja referência está em y = 154
TEXTO_ALTURA = 1.5
TEXTO_ESPESSURA = 0.25


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    board = pcbnew.LoadBoard(path)

    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}
    for ref, (x, y, ang) in REFS.items():
        fp = fps.get(ref)
        if fp is None:
            print(f"{ref}: não encontrado")
            continue
        t = fp.Reference()
        t.SetPosition(v(x, y))
        t.SetTextAngleDegrees(ang)
        print(f"{ref}: texto de referência -> ({x}, {y}) ang={ang}")

    alvo = None
    for d in board.GetDrawings():
        if d.GetClass() == "PCB_TEXT" and d.GetText() == TEXTO:
            alvo = d
            break
    if alvo is None:
        alvo = pcbnew.PCB_TEXT(board)
        alvo.SetText(TEXTO)
        board.Add(alvo)
        acao = "acrescentado"
    else:
        acao = "reposicionado"
    alvo.SetLayer(pcbnew.F_SilkS)
    alvo.SetPosition(v(*TEXTO_POS))
    alvo.SetTextSize(v(TEXTO_ALTURA, TEXTO_ALTURA))
    alvo.SetTextThickness(pcbnew.FromMM(TEXTO_ESPESSURA))
    alvo.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    print(f'"{TEXTO}" {acao} em {TEXTO_POS}, altura {TEXTO_ALTURA} mm')

    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    main(sys.argv[1])
