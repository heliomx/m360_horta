"""Desenha o leque (fanout) fixo dos TVS SOT-23-6: a saída de cada rede para o lado do MCU.

Por que existe: roteando as pernas RJ45 → TVS primeiro, o Freerouting escolhe o lado por onde
chega a cada pad e acaba cercando os outros — a perna do VSW contornou o U3 por cima e fechou a
saída do SOIL0 e do SOIL1; o resto da rede não tinha mais por onde sair (ARQUITETURA §11.8).
Aqui a saída de cada pad é desenhada ANTES das pernas, num padrão fixo, e o roteador só completa:

  pad de cima (I/O)        sai para cima, pelo topo do pad (nada desenhado: o topo fica livre
                           porque a passada das pernas proíbe tudo acima do TVS)
  pad do meio GND          toco para fora até uma via de GND
  pad do meio VSW          toco para dentro, sobe pelo vão entre as colunas até acima do TVS
  pad de baixo, lado GND   toco para fora até uma via
  pad de baixo, lado VSW   toco para o vão, via logo abaixo do TVS

Das vias em diante o roteador escolhe o caminho em B.Cu. Um toco em B.Cu desenhado aqui (subindo
por baixo do TVS) ficava sobrando quando o roteador ligava por outro lado, e fechava a passagem de
redes vizinhas — o SOIL0 da M360-LP não roteava por causa do toco do SOIL3.

A perna de cada pad chega pelo lado que sobra: de baixo nos pads de baixo, e em "L" pela lateral
nos pads de cima e no VSW. Os tocos entram fixos e cobertos de área proibida na passada das
pernas (`dsn_trechos.py --fios-viram-keepout`), e a ordem é provada pelo `ordem_tvs.py`.

Rodar em placa sem trilhas dessas redes (depois do `clear_tracks.py`).
Uso: <KiCad>/bin/python.exe leque_tvs.py <placa.kicad_pcb> U3 U4 U5
"""
import sys

import pcbnew

LARG, VIA_TAM, VIA_FURO = 0.25, 0.6, 0.3
DX_VIA = 2.45      # mm do centro do TVS até as vias laterais
DY_VIA_CENTRO = 2.0
Y_TOPO = 1.5       # os tocos que sobem terminam a esta distância acima do centro


def v(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def trilha(board, net, camada, pts):
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(v(ax, ay))
        t.SetEnd(v(bx, by))
        t.SetWidth(pcbnew.FromMM(LARG))
        t.SetLayer(camada)
        t.SetNet(net)
        board.Add(t)


def via(board, net, x, y):
    t = pcbnew.PCB_VIA(board)
    t.SetPosition(v(x, y))
    t.SetWidth(pcbnew.FromMM(VIA_TAM))
    t.SetDrill(pcbnew.FromMM(VIA_FURO))
    t.SetViaType(pcbnew.VIATYPE_THROUGH)
    t.SetNet(net)
    board.Add(t)


def leque(board, fp):
    mm = pcbnew.ToMM
    cx, cy = mm(fp.GetPosition().x), mm(fp.GetPosition().y)
    pads = {}
    for p in fp.Pads():
        x, y = mm(p.GetPosition().x), mm(p.GetPosition().y)
        lado = -1 if x < cx else 1
        linha = "cima" if y < cy - 0.3 else ("baixo" if y > cy + 0.3 else "meio")
        pads[(lado, linha)] = p
    gnd = next(k for k, p in pads.items() if k[1] == "meio" and p.GetNetname() == "GND")
    vsw = next(k for k, p in pads.items() if k[1] == "meio" and p.GetNetname() == "VSW")
    sg, sv = gnd[0], vsw[0]
    meia = mm(pads[gnd].GetBoundingBox().GetWidth()) / 2   # TVS a 0 ou 180 graus: pad deitado
    col = abs(mm(pads[gnd].GetPosition().x) - cx)
    dentro = col - meia          # distância do centro à ponta interna dos pads
    fora = col + meia
    F = pcbnew.F_Cu
    out = []

    # GND do meio -> via para fora
    p = pads[gnd]
    trilha(board, p.GetNet(), F, [(cx + sg * fora, cy), (cx + sg * DX_VIA, cy)])
    via(board, p.GetNet(), cx + sg * DX_VIA, cy)
    out.append("GND via")

    # VSW do meio -> vão -> para cima
    p = pads[vsw]
    trilha(board, p.GetNet(), F, [(cx + sv * dentro, cy), (cx, cy - 0.45), (cx, cy - Y_TOPO)])
    out.append("VSW pelo vão")

    yb = cy + (mm(pads[(sg, "baixo")].GetPosition().y) - cy)
    # de baixo, lado do GND -> via para fora -> B.Cu sobe por baixo do TVS
    p = pads[(sg, "baixo")]
    xv = cx + sg * DX_VIA
    trilha(board, p.GetNet(), F, [(cx + sg * fora, yb), (xv, yb)])
    via(board, p.GetNet(), xv, yb)
    out.append(f"{p.GetNetname()} via lateral")

    # de baixo, lado do VSW -> vão -> via abaixo do TVS -> B.Cu sobe pelo centro
    p = pads[(sv, "baixo")]
    yv = cy + DY_VIA_CENTRO
    trilha(board, p.GetNet(), F, [(cx + sv * dentro, yb), (cx, yb + 0.4), (cx, yv)])
    via(board, p.GetNet(), cx, yv)
    out.append(f"{p.GetNetname()} via central")
    return out


def main(path, refs):
    board = pcbnew.LoadBoard(path)
    fps = {f.GetReference(): f for f in board.GetFootprints()}
    for r in refs:
        print(f"   {r}: " + ", ".join(leque(board, fps[r])))
    pcbnew.SaveBoard(path, board)
    print("placa salva")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2:])
