"""Serigrafia da M360-ACT: tira o texto de referência de cima de ilha e põe a marca da placa.

- U1 (AMS1117): o texto de referência caía sobre a ilha 2 do C3 — único aviso de serigrafia
  do DRC que não é proposital. Vai para a faixa livre entre o corpo do U1 e o borne J1.
- Acrescenta "Viridiotech.com.br" na área vazia entre o CD74HC4067 e o reed.

Texto de serigrafia sobre trilha não é problema (a máscara cobre a trilha); o que o DRC
cobra é serigrafia sobre ilha exposta ou sobre outra serigrafia.

Idempotente: reposiciona o texto se já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_act.py <placa.kicad_pcb>
"""
import sys

import pcbnew

# referência -> (x, y, ângulo)
REFS = {
    "U1": (109.0, 144.0, 0),     # entre o corpo do U1 e o borne J1
}

TEXTO = "Viridiotech.com.br"
TEXTO_POS = (176.0, 149.0)       # entre o U3 e o reed, área livre
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
