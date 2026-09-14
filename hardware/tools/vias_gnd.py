"""Põe vias de GND encostadas a pads escolhidos e costura o perímetro da placa.

Por que existe: a revisão de placa de 2026-09-14 achou o plano de F.Cu da M360-LP em 14 ilhas com
só 5 vias de GND na placa inteira, o GND do J1.8 saindo por uma via só, e capacitores de
desacoplamento sem via própria (ARQUITETURA §11.8). O GND é roteado como rede comum (§11.6) e os
planos preenchem por cima; estas vias ligam as duas faces onde o retorno importa.

Diferente do gnd_stitch.py, este script não "decide" ilha solta: você diz ONDE (pads) e ele só
acha lugar livre. A decisão de suficiência é do DRC e da contagem de ilhas depois do refill.

Colisão: cada candidata é testada com `HitTest(ponto, isolamento + raio da via)` contra pads,
trilhas e vias de outras redes nas DUAS faces (via atravessa as duas), contra qualquer pad (via
fora de pad, também do GND, para não roubar solda) e contra a borda da placa.

Idempotente: não põe via perto de outra via de GND já existente.

Uso: <KiCad>/bin/python.exe vias_gnd.py <placa.kicad_pcb> [--perimetro PASSO_MM] REF-PAD [...]
     p.ex. vias_gnd.py placa.kicad_pcb --perimetro 10 J1-8 J2-8 C1-2 C2-2
"""
import math
import sys

import pcbnew

VIA_TAM, VIA_FURO = 0.6, 0.3
ISOL = 0.2          # isolamento da placa (Default)
FOLGA_PAD = 0.25    # distância mínima entre a borda da via e qualquer pad
BORDA = 1.0         # distância mínima do centro da via à borda da placa, além do raio
PERTO = 2.0         # via de GND a menos disto já atende o pad
PERTO_PERIM = 3.0   # idem, para um ponto do perímetro


def ui(v):
    return pcbnew.FromMM(v)


def livre(board, x, y, gnd):
    p = pcbnew.VECTOR2I(ui(x), ui(y))
    r = VIA_TAM / 2
    caixa = board.GetBoardEdgesBoundingBox()
    if not (caixa.GetLeft() + ui(r + BORDA) <= p.x <= caixa.GetRight() - ui(r + BORDA) and
            caixa.GetTop() + ui(r + BORDA) <= p.y <= caixa.GetBottom() - ui(r + BORDA)):
        return False
    for pad in board.GetPads():   # qualquer pad, inclusive de GND: via fora do pad
        folga = ui(max(ISOL, FOLGA_PAD))
        if pad.GetNetCode() != gnd.GetNetCode():   # classe do pad (Power pede 0,3 mm)
            folga = max(folga, pad.GetOwnClearance(pcbnew.F_Cu))
        if pad.HitTest(p, ui(r) + folga):
            return False
    # área proibida para via: da placa e dos footprints (o keep-out da antena do D1 mini, na GW,
    # mora dentro do footprint)
    zonas = list(board.Zones()) + [z for f in board.GetFootprints() for z in f.Zones()]
    for z in zonas:
        if z.GetIsRuleArea() and z.GetDoNotAllowVias() and z.Outline().Collide(p, ui(r)):
            return False
    for t in board.GetTracks():
        if t.GetNetCode() == gnd.GetNetCode() and t.Type() != pcbnew.PCB_VIA_T:
            continue  # encostar em trilha de GND é bom
        # isolamento da classe da trilha (Power pede 0,3 mm), não só o padrão
        if t.HitTest(p, ui(r) + max(ui(ISOL), t.GetOwnClearance(t.GetLayer()))):
            return False
    for fp in board.GetFootprints():   # fiducial: 2 mm de máscara aberta e isolamento próprio
        if fp.GetReference().startswith("FID"):
            q = fp.GetPosition()
            if math.hypot(pcbnew.ToMM(q.x) - x, pcbnew.ToMM(q.y) - y) < 1.0 + r + ISOL:
                return False
    for fp in board.GetFootprints():   # furos NPTH (pegas do RJ45, furos M3)
        for pad in fp.Pads():
            if pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH and pad.HitTest(p, ui(r + 0.5)):
                return False
    return True


def ha_via_gnd(board, x, y, raio, gnd):
    for t in board.GetTracks():
        if t.Type() == pcbnew.PCB_VIA_T and t.GetNetCode() == gnd.GetNetCode():
            q = t.GetPosition()
            if math.hypot(pcbnew.ToMM(q.x) - x, pcbnew.ToMM(q.y) - y) < raio:
                return True
    return False


def poe_via(board, x, y, gnd):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(pcbnew.VECTOR2I(ui(x), ui(y)))
    v.SetWidth(ui(VIA_TAM))
    v.SetDrill(ui(VIA_FURO))
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    v.SetNet(gnd)
    board.Add(v)


def junto_do_pad(board, pad, gnd):
    cx, cy = pcbnew.ToMM(pad.GetPosition().x), pcbnew.ToMM(pad.GetPosition().y)
    tam = max(pcbnew.ToMM(pad.GetSize().x), pcbnew.ToMM(pad.GetSize().y)) / 2
    if ha_via_gnd(board, cx, cy, tam + PERTO, gnd):
        return "já tem"
    for anel in [tam + VIA_TAM / 2 + FOLGA_PAD + 0.05 * k for k in range(0, 30)]:
        for i in range(24):
            a = 2 * math.pi * i / 24
            x, y = cx + anel * math.cos(a), cy + anel * math.sin(a)
            if livre(board, x, y, gnd):
                poe_via(board, x, y, gnd)
                return f"via em ({x:.2f}; {y:.2f})"
    return "SEM LUGAR"


def perimetro(board, passo, gnd):
    caixa = board.GetBoardEdgesBoundingBox()
    x1, y1 = pcbnew.ToMM(caixa.GetLeft()), pcbnew.ToMM(caixa.GetTop())
    x2, y2 = pcbnew.ToMM(caixa.GetRight()), pcbnew.ToMM(caixa.GetBottom())
    d = BORDA + VIA_TAM / 2 + 0.2
    pontos = []
    for lado in ((x1 + d, y1 + d, x2 - d, y1 + d), (x2 - d, y1 + d, x2 - d, y2 - d),
                 (x2 - d, y2 - d, x1 + d, y2 - d), (x1 + d, y2 - d, x1 + d, y1 + d)):
        ax, ay, bx, by = lado
        n = max(1, int(math.hypot(bx - ax, by - ay) / passo))
        pontos += [(ax + (bx - ax) * i / n, ay + (by - ay) * i / n) for i in range(n)]
    postas, falhas = 0, 0
    for x, y in pontos:
        if ha_via_gnd(board, x, y, PERTO_PERIM, gnd):
            continue
        # procura o ponto livre mais próximo, até 3 mm para dentro/ao longo
        achou = False
        for r in [0.25 * k for k in range(0, 13)]:
            for i in range(max(1, int(8 * r) or 1)):
                a = 2 * math.pi * i / max(1, int(8 * r) or 1)
                px, py = x + r * math.cos(a), y + r * math.sin(a)
                if livre(board, px, py, gnd):
                    poe_via(board, px, py, gnd)
                    postas += 1
                    achou = True
                    break
            if achou:
                break
        if not achou:
            falhas += 1
    return postas, falhas


def main(argv):
    path = argv[0]
    passo = None
    refs = []
    i = 1
    while i < len(argv):
        if argv[i] == "--perimetro":
            passo = float(argv[i + 1])
            i += 2
        else:
            refs.append(argv[i])
            i += 1
    board = pcbnew.LoadBoard(path)
    gnd = board.FindNet("GND")
    if gnd is None:
        sys.exit("rede GND não encontrada")
    pads = {f"{p.GetParentFootprint().GetReference()}-{p.GetNumber()}": p for p in board.GetPads()}
    sem = []
    for r in refs:
        pad = pads.get(r)
        if pad is None or pad.GetNetname() != "GND":
            sys.exit(f"{r}: pad inexistente ou fora do GND")
        res = junto_do_pad(board, pad, gnd)
        print(f"   {r}: {res}")
        if res == "SEM LUGAR":
            sem.append(r)
    if passo:
        postas, falhas = perimetro(board, passo, gnd)
        print(f"   perímetro a cada {passo:g} mm: {postas} via(s) nova(s), {falhas} ponto(s) sem lugar")
    pcbnew.SaveBoard(path, board)
    print("placa salva")
    if sem:
        sys.exit(f"sem lugar para via junto de: {', '.join(sem)}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1:])
