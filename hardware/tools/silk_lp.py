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
    # U3-U5: em pé, à esquerda de cada TVS. Logo abaixo (y 146,1) o render mostrou o texto sob o
    # corpo dos RJ45, e acima saem as trilhas do leque.
    "U3": (108.3, 143.5, 90),
    "U4": (120.2, 143.5, 90),
    "U5": (132.3, 143.5, 90),
    "SW1": (104.1, 139.0, 90),
    # R26-R29 em pé acima do U5, a 1,9 mm um do outro: referência girada, logo acima de cada um
    "R26": (142.8, 136.9, 90),
    "R27": (140.9, 136.9, 90),
    "R28": (133.3, 136.95, 0),   # deitada na faixa entre a referência do R20 e o corpo
    "R29": (139.0, 136.9, 90),
    # O R28 ficou entre o C14 e o C15, onde caía a referência do C14: ela desce para baixo do C14
    "C14": (129.4, 142.0, 0),
    "C3": (167.6, 107.2, 0),     # à esquerda do C3; acima dele fica o U1   # reed na borda esquerda: referência entre ele e a coluna R6/R14/C8
    "R3": (147.0, 120.8, 0),     # abaixo do próprio corpo: em cima ele toca o contorno do U2
}

# textos livres: (conteúdo, x, y, altura[, ângulo])
# O J3 ocupa y 137,1-143,9; as legendas ficam acima, na faixa livre até os pull-ups (y 130).
TEXTOS = [
    # O header J5 do nRF24 (pinos em y 152, x 147,1-164,9) ocupa a faixa onde a marca ficava:
    # o par sobe para entre a fileira R26-R29 (courtyard até 141,3) e o header (courtyard a partir
    # de 150,2). As trilhas de DHT/SCL em y 142,2-142,7 ficam sob a máscara, sem ilha.
    ("Viridiotech.com.br", 158.0, 143.5, 1.5),
    ("M360-LP", 158.0, 147.0, 1.5),
    # Um rádio por placa: o CSN partilha o NSS e o CE partilha o DIO0 (ARQUITETURA §2, §4.2).
    ("nRF24 OU mezanino", 156.0, 155.3, 0.9),    # abaixo da fileira do J5
    ("CE", 147.1, 149.6, 0.9),                   # pino 1 do J5
    # 1,0 mm é o mínimo de texto legível na serigrafia da JLCPCB (a revisão de placa achou 0,8).
    ("VCC", 177.0, 134.0, 1.0),                  # sobre a ilha 1 do J3 (VIN_RAW)
    ("GND", 181.5, 134.0, 1.0),                  # sobre a ilha 2 do J3
    # O header FTDI do Pro Mini liga direto no trilho de 3,3 V, que alimenta o rádio: um
    # adaptador em 5 V queima o SX1276 (§4.2). Abaixo do Pro Mini: na faixa da borda esquerda ela
    # ficava sob a projeção de um header FTDI angulado.
    ("FTDI sem VCC", 112.0, 124.6, 1.0),
    # O mezanino encaixa girado 180° no soquete simétrico (§3): marca o pino 1 do J4, que fica
    # na coluna da direita (pad 1 em 158,8; 107,42).
    ("1", 161.6, 107.42, 1.0),
    # Furo 10 do soquete: é o que se tampa para chavear o mezanino (§3). Pad 10 em (156,26; 117,58).
    ("10", 153.3, 117.58, 1.0),
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
    for conteudo, x, y, altura, *ang in TEXTOS:
        texto_livre(board, conteudo, x, y, altura, *ang)

    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    main(sys.argv[1])
