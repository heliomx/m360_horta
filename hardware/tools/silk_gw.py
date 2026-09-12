"""Serigrafia da M360-GW: põe a marca da placa no espaço livre.

Ao contrário da LP e da ACT, aqui não há texto de referência a reposicionar: o DRC do
posicionamento não acusou serigrafia sobre ilha, e os textos dos quatro furos M3 já saem
invisíveis pelo place_gw.py (são board-only e ficam fora da BOM e do arquivo de posição).

A marca vai na faixa livre entre o contorno do mezanino (que termina em y = 123) e o topo do
C3 e do R4 (~128,5) — uma tira que atravessa a placa em x 146-180.

Cuidado com a largura do texto: 18 caracteres a 1,5 mm de altura ocupam ~20 mm, não ~16. Em
(154, 130) a marca começava em ~144 e encostava na ilha 2 do C3, que termina em ~143,4 —
`silk_over_copper` no DRC.

Idempotente: reposiciona o texto se já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_gw.py <placa.kicad_pcb>
"""
import sys

import pcbnew

TEXTO = "Viridiotech.com.br"
TEXTO_POS = (160.0, 125.5)
TEXTO_ALTURA = 1.5
TEXTO_ESPESSURA = 0.25


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def main(path):
    board = pcbnew.LoadBoard(path)

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
