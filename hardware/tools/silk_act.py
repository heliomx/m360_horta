"""Serigrafia da M360-ACT: marca da placa, nome do modelo e legenda dos conectores.

- U1 (AMS1117): o texto de referência caía sobre a ilha 2 do C3 — único aviso de serigrafia
  do DRC que não era proposital. Vai para a faixa livre entre o corpo do U1 e o borne J1.
- "Viridiotech.com.br" e, logo abaixo, "M360-ACT".
- **Legenda dos conectores:** o campo Value de cada J* passa a aparecer na serigrafia, para
  orientar quem for ligar o chicote — "RJ45 RELES-A", "RJ45 SENSORES", "5V do quadro" etc.
  Sem isso o instalador só vê J3/J4/J5 e não tem como saber qual é qual.

As posições foram escolhidas sobre a geometria medida da placa:
  - os três RJ45 ocupam y 164,76-180 e os TVS terminam em y 162,25; sobra uma faixa de 2,5 mm
    entre eles, onde cabe texto de 1,0 mm de altura;
  - o J1 (borne, rot 0) vai de y 144,5 a 154,6 e os TVS começam em 158,75: a legenda fica embaixo;
  - o mezanino cobre y 102-125, então a legenda do J2 fica logo abaixo dele.

Texto de serigrafia sobre trilha não é problema (a máscara cobre a trilha); o que o DRC cobra
é serigrafia sobre ilha exposta ou sobre outra serigrafia.

Idempotente: reposiciona o que já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_act.py <placa.kicad_pcb>
"""
import sys

import pcbnew

# referência -> (x, y, ângulo) do texto de REFERÊNCIA
REFS = {
    # Em y = 144 o texto encostava na serigrafia do J1 depois que ele passou a rot 0 (topo em
    # y 144,5). O courtyard do U1 termina em 141,6: y = 143 fica no meio da folga.
    "U1": (109.0, 143.0, 0),     # entre o corpo do U1 e o borne J1
}

# referência -> (x, y, altura, ângulo) do campo VALUE, que vira legenda na serigrafia
#
# O ÂNGULO É OBRIGATÓRIO: o campo Value herda a rotação do footprint. O J1 esteve em rot 90, e
# definir só a posição deixou "5V do quadro" na vertical, atravessando a própria ilha do borne
# (silk_over_copper no DRC). Sempre zerar o ângulo em legenda que se quer horizontal.
#
# Altura 0,8 mm nos RJ45: a faixa livre entre os TVS (terminam em y 162,25) e os conectores
# (começam em 164,76) tem só 2,5 mm, e com 1,0 mm o texto encostava nos corpos.
VALORES = {
    # NÃO mover para x < 101: a placa começa em x = 100, e em x = 99 o texto saiu pela borda
    # (silk_edge_clearance) e ainda encostou na referência e no contorno do próprio borne.
    # A faixa boa é ABAIXO do J1, que termina em y 154,6, antes dos TVS em y 158,75.
    "J1": (106.0, 157.2, 0.8, 0),   # entre o borne e a fileira de TVS
    "J2": (176.0, 128.0, 1.0, 0),   # abaixo do contorno do mezanino
    # A faixa entre os TVS (terminam em y 162,25) e os RJ45 (começam em 164,76) tem 2,5 mm e
    # NÃO serve: mesmo a 0,8 mm o texto encostava nos corpos dos conectores. As legendas sobem
    # para ACIMA da fileira de TVS (que começa em y 158,75), onde há espaço de verdade.
    "J3": (120.0, 156.0, 0.9, 0),
    "J4": (140.0, 156.0, 0.9, 0),
    "J5": (160.0, 156.0, 0.9, 0),
}

# textos livres: (conteúdo, x, y, altura)
TEXTOS = [
    ("Viridiotech.com.br", 176.0, 149.0, 1.5),   # entre o U3 e o SW1
    ("M360-ACT", 176.0, 152.0, 1.5),             # logo abaixo da marca
]

ESPESSURA = 0.25


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def texto_livre(board, conteudo, x, y, altura):
    """Cria ou reposiciona um texto solto na serigrafia da face de cima."""
    alvo = None
    for d in board.GetDrawings():
        if d.GetClass() == "PCB_TEXT" and d.GetText() == conteudo:
            alvo = d
            break
    acao = "reposicionado"
    if alvo is None:
        alvo = pcbnew.PCB_TEXT(board)
        alvo.SetText(conteudo)
        board.Add(alvo)
        acao = "acrescentado"
    alvo.SetLayer(pcbnew.F_SilkS)
    alvo.SetPosition(v(x, y))
    alvo.SetTextSize(v(altura, altura))
    alvo.SetTextThickness(pcbnew.FromMM(ESPESSURA))
    alvo.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    print(f'   "{conteudo}" {acao} em ({x}, {y}), altura {altura} mm')


def main(path):
    board = pcbnew.LoadBoard(path)
    fps = {fp.GetReference(): fp for fp in board.GetFootprints()}

    print("textos de referência:")
    for ref, (x, y, ang) in REFS.items():
        fp = fps.get(ref)
        if fp is None:
            print(f"   {ref}: não encontrado")
            continue
        t = fp.Reference()
        t.SetPosition(v(x, y))
        t.SetTextAngleDegrees(ang)
        print(f"   {ref}: -> ({x}, {y}) ang={ang}")

    print("legendas dos conectores (campo Value na serigrafia):")
    for ref, (x, y, altura, ang) in VALORES.items():
        fp = fps.get(ref)
        if fp is None:
            print(f"   {ref}: não encontrado")
            continue
        t = fp.Value()
        t.SetLayer(pcbnew.F_SilkS)
        t.SetVisible(True)
        t.SetPosition(v(x, y))
        t.SetTextSize(v(altura, altura))
        t.SetTextThickness(pcbnew.FromMM(0.15))
        t.SetTextAngleDegrees(ang)
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
        print(f'   {ref}: "{t.GetText()}" -> ({x}, {y}) ang={ang}, altura {altura} mm')

    print("textos livres:")
    for conteudo, x, y, altura in TEXTOS:
        texto_livre(board, conteudo, x, y, altura)

    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    main(sys.argv[1])
