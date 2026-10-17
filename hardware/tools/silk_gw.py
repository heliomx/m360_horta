"""Serigrafia da M360-GW: marca da placa e legenda do header do nRF24.

Ao contrário da LP e da ACT, aqui não há texto de referência a reposicionar à mão: as referências
saem pelo tools/silk_refs.py (rodar DEPOIS deste script, cujas legendas entram como obstáculo), e
os textos dos quatro furos M3 já saem invisíveis pelo place_gw.py.

- A marca vai na faixa livre entre o contorno do mezanino (que termina em y = 123) e o topo do
  C3 e do R4 (~128,5) — uma tira que atravessa a placa em x 146-180. Cuidado com a largura: 18
  caracteres a 1,5 mm de altura ocupam ~20 mm, não ~16. Em (154, 130) a marca começava em ~144 e
  encostava na ilha 2 do C3, que termina em ~143,4 — `silk_over_copper` no DRC.
- "nRF24 OU mezanino" logo abaixo do header J4 (pinos em y 131): um rádio por placa, o CSN partilha
  o NSS e o CE partilha o DIO0 (ARQUITETURA §2, §6.1). "CE" marca o pino 1.

Idempotente: reposiciona o texto se já existir, em vez de duplicar.
Uso: <KiCad>/bin/python.exe silk_gw.py <placa.kicad_pcb>
"""
import sys

import pcbnew

# (conteúdo, x, y, altura[, ângulo])
TEXTOS = [
    ("Viridiotech.com.br", 160.0, 125.5, 1.5),
    ("nRF24 OU mezanino", 158.54, 134.3, 0.9),    # abaixo da fileira do J4
    ("CE", 149.65, 128.6, 0.9),                   # pino 1 do J4 (149,65; 131)
]
ESPESSURA = 0.25


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def texto_livre(board, conteudo, x, y, altura, angulo=0):
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
    alvo.SetTextAngleDegrees(angulo)
    print(f'   "{conteudo}" {acao} em ({x}, {y}), altura {altura} mm')


def main(path):
    board = pcbnew.LoadBoard(path)
    print("textos livres:")
    for conteudo, x, y, altura, *ang in TEXTOS:
        texto_livre(board, conteudo, x, y, altura, *ang)
    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    main(sys.argv[1])
