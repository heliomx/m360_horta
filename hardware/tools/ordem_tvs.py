"""Prova que, nas linhas de campo, o cobre sai do pino do RJ45 e passa pelo TVS antes de ir a
qualquer outro lugar.

Por que existe: a revisão de placa de 2026-09-14 achou o TVS como ramo morto em SDA, DHT, SOIL6 e
VSW — a rede abria em estrela no próprio pad do RJ45, e o resistor de série saía por outro caminho
(ARQUITETURA §11.8). O DRC e a paridade não enxergam isso: a ligação está "certa".

Para cada rede, com a geometria de trilhas, vias e pads lida pelo pcbnew:
  1. o pino do RJ45 precisa chegar ao pad do TVS;
  2. o cobre é CORTADO onde cruza o pad do TVS: some o pad e só o trecho de cada trilha que está
     dentro dele — o resto da trilha continua existindo;
  3. o pino do RJ45 precisa ficar ISOLADO de qualquer outro pad da rede (resistor, pull-up, outro
     conector, fonte). Se ainda chegar a algum, existe caminho que contorna o TVS — reprovado.

O corte em (2) importa: apagar a trilha inteira que encosta no TVS aprovaria uma derivação feita
no meio dela, a milímetros do pad. Ligações em T (ponta de uma trilha no meio de outra) contam.

Só leitura. Rodar com o Python do KiCad.
Uso: <KiCad>/bin/python.exe ordem_tvs.py <placa.kicad_pcb> lp|act
     <KiCad>/bin/python.exe ordem_tvs.py <placa.kicad_pcb> REDE=J1-7,J2-7:U3-5,U5-5 [...]
Sai com código 1 se alguma rede reprovar.
"""
import math
import sys

import pcbnew

PRESETS = {
    "lp": [
        "SOIL0=J1-1:U3-1", "SOIL1=J1-2:U3-3", "SOIL2=J1-3:U3-4", "SOIL3=J1-6:U3-6",
        "SOIL4=J1-4:U4-1", "SOIL5=J1-5:U4-3", "SOIL6=J2-4:U4-4", "SOIL7=J2-5:U4-6",
        "ONEWIRE=J2-1:U5-1", "DHT=J2-2:U5-3", "SDA=J2-3:U5-4", "SCL=J2-6:U5-6",
        "VSW=J1-7,J2-7:U3-5,U4-5,U5-5",
    ],
    # canais atribuídos pela ordem física dos pinos do RJ45 (ARQUITETURA §5.2)
    "act": [
        "REL_CH0=J3-1:U5-1", "REL_CH1=J3-2:U4-3", "REL_CH2=J3-3:U5-3",
        "REL_CH3=J3-6:U4-6", "REL_CH4=J3-4:U4-1", "REL_CH5=J3-5:U4-4",
        "REL_CH6=J4-1:U7-1", "REL_NFT=J4-2:U6-3", "REL_OXI=J4-3:U7-3",
        "REL_CH7=J4-6:U6-6", "REL_CH8=J4-4:U6-1", "REL_CH9=J4-5:U6-4",
        "FLOW_5V=J5-1:U8-1", "AUX_A6_IN=J5-3:U8-3",
        "+5V=J3-7,J4-7,J5-7:U4-5,U5-5,U6-5,U7-5,U8-5",
    ],
}

PASSO = 0.05   # mm, amostragem das trilhas
TOL = 0.02     # mm, tolerância de encontro entre pontas e trilhas


def dist_ponto_seg(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def nome_pad(p):
    return f"{p.GetParentFootprint().GetReference()}-{p.GetNumber()}"


def dentro(pad, camada, x, y, folga):
    return pad.IsOnLayer(camada) and pad.HitTest(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)),
                                                  pcbnew.FromMM(folga))


def pedacos(board, rede, cortar):
    """Trilhas da rede como pedaços (camada, ax, ay, bx, by, meia_largura), já cortadas nos pads."""
    out = []
    for t in board.GetTracks():
        if t.GetNetname() != rede or t.Type() == pcbnew.PCB_VIA_T:
            continue
        lay = t.GetLayer()
        ax, ay = pcbnew.ToMM(t.GetStart().x), pcbnew.ToMM(t.GetStart().y)
        bx, by = pcbnew.ToMM(t.GetEnd().x), pcbnew.ToMM(t.GetEnd().y)
        w2 = pcbnew.ToMM(t.GetWidth()) / 2
        if not cortar:
            out.append((lay, ax, ay, bx, by, w2))
            continue
        n = max(1, int(math.hypot(bx - ax, by - ay) / PASSO))
        pts = [(ax + (bx - ax) * i / n, ay + (by - ay) * i / n) for i in range(n + 1)]
        fora = [not any(dentro(p, lay, x, y, w2) for p in cortar) for x, y in pts]
        i = 0
        while i <= n:
            if fora[i]:
                j = i
                while j + 1 <= n and fora[j + 1]:
                    j += 1
                out.append((lay, *pts[i], *pts[j], w2))
                i = j + 1
            else:
                i += 1
    return out


def grupos(board, rede, sem=()):
    pads = [p for p in board.GetPads() if p.GetNetname() == rede]
    cortar = [p for p in pads if nome_pad(p) in sem]
    vivos = [p for p in pads if nome_pad(p) not in sem]
    vias = [t for t in board.GetTracks() if t.GetNetname() == rede and t.Type() == pcbnew.PCB_VIA_T]
    segs = pedacos(board, rede, cortar)

    pai = {}

    def ache(x):
        pai.setdefault(x, x)
        while pai[x] != x:
            pai[x] = pai[pai[x]]
            x = pai[x]
        return x

    def una(a, b):
        pai[ache(a)] = ache(b)

    for k, (lay, ax, ay, bx, by, w2) in enumerate(segs):
        una(("s", k), ("s", k))
        n = max(1, int(math.hypot(bx - ax, by - ay) / PASSO))
        amostra = [(ax + (bx - ax) * i / n, ay + (by - ay) * i / n) for i in range(n + 1)]
        for p in vivos:
            if any(dentro(p, lay, x, y, w2) for x, y in amostra):
                una(("s", k), ("p", nome_pad(p)))
        for v in vias:
            vx, vy = pcbnew.ToMM(v.GetPosition().x), pcbnew.ToMM(v.GetPosition().y)
            vr = pcbnew.ToMM(v.GetWidth(pcbnew.F_Cu)) / 2
            if dist_ponto_seg(vx, vy, ax, ay, bx, by) <= vr + w2:
                una(("s", k), ("v", id(v)))
        for m, (lay2, cx, cy, dx, dy, w22) in enumerate(segs):
            if m == k or lay2 != lay:
                continue
            if (dist_ponto_seg(ax, ay, cx, cy, dx, dy) <= w22 + TOL or
                    dist_ponto_seg(bx, by, cx, cy, dx, dy) <= w22 + TOL):
                una(("s", k), ("s", m))
    for v in vias:
        una(("v", id(v)), ("v", id(v)))
        vx, vy = pcbnew.ToMM(v.GetPosition().x), pcbnew.ToMM(v.GetPosition().y)
        vr = pcbnew.ToMM(v.GetWidth(pcbnew.F_Cu)) / 2
        for p in vivos:
            if dentro(p, pcbnew.F_Cu, vx, vy, vr) or dentro(p, pcbnew.B_Cu, vx, vy, vr):
                una(("v", id(v)), ("p", nome_pad(p)))
    for p in vivos:
        una(("p", nome_pad(p)), ("p", nome_pad(p)))

    g = {}
    for k in list(pai):
        if k[0] == "p":
            g.setdefault(ache(k), set()).add(k[1])
    return {pad: grp for grp in g.values() for pad in grp}, [nome_pad(p) for p in pads]


def main(pcb, specs):
    board = pcbnew.LoadBoard(pcb)
    reprovadas = 0
    for spec in specs:
        rede, resto = spec.split("=", 1)
        js, tvs = (x.split(",") for x in resto.split(":"))
        g, todos = grupos(board, rede)
        falhas = []
        for j in js:
            if not any(t in g.get(j, set()) for t in tvs):
                falhas.append(f"{j} não chega a nenhum TVS ({', '.join(tvs)})")
        g_sem, _ = grupos(board, rede, sem=set(tvs))
        for j in js:
            outros = sorted(g_sem.get(j, {j}) - {j})
            if outros:
                falhas.append(f"{j} alcança {', '.join(outros)} sem passar pelo TVS")
        if falhas:
            reprovadas += 1
            print(f"REPROVADA  {rede}: " + "; ".join(falhas))
        else:
            print(f"ok         {rede}: {', '.join(js)} → {', '.join(tvs)} → resto ({len(todos) - len(js) - len(tvs)} pads)")
    print(f"\n{len(specs) - reprovadas} de {len(specs)} redes com o TVS no caminho")
    return 1 if reprovadas else 0


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    args = sys.argv[2:]
    specs = PRESETS[args[0]] if len(args) == 1 and args[0] in PRESETS else args
    sys.exit(main(sys.argv[1], specs))
