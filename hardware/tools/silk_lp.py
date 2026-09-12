"""Serigrafia da M360-LP: marca da placa, nome do modelo e polaridade da entrada.

- U2, U4 e U5: o texto de referência ia parar sobre os pads de C7, C12 e C15. Vai para a
  folga livre ao lado de cada peça (o componente não se move).
- "Viridiotech.com.br" e, logo abaixo, "M360-LP".
- **VCC e GND do J3**: a entrada de alimentação é um JST de 2 vias, e nada na placa dizia
  qual é qual. Ligar invertido queima o nó, então a legenda não é conforto, é proteção.

A polaridade foi conferida na netlist, não suposta pela ordem dos pinos:
  - J3 pino 1 = /VIN_RAW  → ilha em x = 178,0
  - J3 pino 2 = GND       → ilha em x = 180,5 (a mesma rede de C4.2, C6.2, J1.8, J2.8...)

Idempotente: reposiciona o que já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_lp.py <placa.kicad_pcb>
"""
import sys

import pcbnew

# referência -> (x, y, ângulo) do texto de REFERÊNCIA
REFS = {
    "U2": (152.5, 112.0, 90),    # folga entre o corpo do U2 e o soquete do mezanino
    "U4": (129.5, 143.5, 0),     # vão entre U4 e U5
    "U5": (141.5, 143.5, 0),     # à direita do U5, antes dos pull-ups
    "R3": (147.0, 120.8, 0),     # abaixo do próprio corpo: em cima ele toca o contorno do U2
    # O botão (SW_PUSH_6mm, origem no pad 1) traz a referência acima do corpo, em (163,25; 151):
    # em cima do "M360-LP". Vai para a faixa livre entre o botão (x <= 168) e o H4 (x >= 179).
    "SW1": (172.0, 155.0, 0),
}

# textos livres: (conteúdo, x, y, altura)
# O J3 ocupa y 137,1-143,9; as legendas ficam acima, na faixa livre até os pull-ups (y 130).
TEXTOS = [
    # A referência do SW1 fica em y ≈ 154: com "M360-LP" em 153,5 os dois se imprimiam um
    # sobre o outro (silk_overlap). O par sobe para a faixa livre acima do SW1.
    ("Viridiotech.com.br", 158.0, 147.0, 1.5),   # acima do SW1
    ("M360-LP", 158.0, 150.5, 1.5),              # logo abaixo da marca
    ("VCC", 177.0, 135.0, 0.8),                  # sobre a ilha 1 do J3 (VIN_RAW)
    ("GND", 181.5, 135.0, 0.8),                  # sobre a ilha 2 do J3
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
    # textos pequenos pedem traço mais fino, senão as letras fecham
    alvo.SetTextThickness(pcbnew.FromMM(ESPESSURA if altura >= 1.2 else 0.15))
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

    print("textos livres:")
    for conteudo, x, y, altura in TEXTOS:
        texto_livre(board, conteudo, x, y, altura)

    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    main(sys.argv[1])
