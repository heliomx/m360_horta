"""Apaga trilhas com ponta solta (o `track_dangling` do DRC), repetindo até não sobrar nenhuma.

Por que existe: o leque do `leque_tvs.py` desenha uma saída em B.Cu para cada pad de baixo dos
TVS, mas o Freerouting às vezes liga o resto da rede direto na via e deixa o toco sobrando. Além
de sujar o DRC, o toco solto ocupa a passagem de outra rede (ARQUITETURA §11.8).

Via de rede sem zona que só tem trilha numa face também sai (é o `via_dangling` do DRC); se a trilha
terminava nela, o toco cai na rodada seguinte.

Ponta solta: nenhuma outra trilha da mesma rede e camada encosta nela (ponta ou meio, ligação em
T conta), e ela não cai em pad nem em via da mesma rede. Uma trilha só é apagada inteira; a ponta
que ela deixa solta na vizinha é pega na rodada seguinte.

Uso: <KiCad>/bin/python.exe apara_pontas.py <placa.kicad_pcb> [REDE ...]   (sem REDE: todas)
"""
import math
import os
import subprocess
import sys

import pcbnew

TOL = 0.01   # mm


def dist_ponto_seg(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def mm(v):
    return pcbnew.ToMM(v)


def ponta_ligada(board, trilha, x, y, trilhas, vias, pads):
    rede, camada = trilha.GetNetCode(), trilha.GetLayer()
    for v in vias.get(rede, []):
        if math.hypot(mm(v.GetPosition().x) - x, mm(v.GetPosition().y) - y) <= mm(v.GetWidth(pcbnew.F_Cu)) / 2:
            return True
    ponto = pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))
    for p in pads.get(rede, []):
        if p.IsOnLayer(camada) and p.HitTest(ponto, trilha.GetWidth() // 2):   # toco que nasce na borda do pad
            return True
    for o in trilhas.get((rede, camada), []):
        if o is trilha:
            continue
        if dist_ponto_seg(x, y, mm(o.GetStart().x), mm(o.GetStart().y), mm(o.GetEnd().x), mm(o.GetEnd().y)) \
                <= mm(o.GetWidth()) / 2 + TOL:
            return True
    return False


def rodada(path, redes):
    """Uma rodada: apaga as soltas e devolve quantas. Uma por processo — numa segunda LoadBoard no
    mesmo processo, depois de um Remove, o SWIG devolve trilhas como objeto morto."""
    if True:
        board = pcbnew.LoadBoard(path)
        trilhas, vias, pads = {}, {}, {}
        for t in board.GetTracks():
            if t.Type() == pcbnew.PCB_VIA_T:
                vias.setdefault(t.GetNetCode(), []).append(t)
            else:
                trilhas.setdefault((t.GetNetCode(), t.GetLayer()), []).append(t)
        for p in board.GetPads():
            pads.setdefault(p.GetNetCode(), []).append(p)
        soltas = []
        for lista in trilhas.values():
            for t in lista:
                if redes and t.GetNetname() not in redes:
                    continue
                pontas = [(mm(t.GetStart().x), mm(t.GetStart().y)), (mm(t.GetEnd().x), mm(t.GetEnd().y))]
                if not all(ponta_ligada(board, t, x, y, trilhas, vias, pads) for x, y in pontas):
                    soltas.append(t)
        # via de rede sem zona que só uma trilha toca: ponta solta também (leva o toco na rodada seguinte)
        redes_zona = {z.GetNetCode() for z in board.Zones() if not z.GetIsRuleArea()}
        for lista in vias.values():
            for v in lista:
                if v.GetNetCode() in redes_zona or (redes and v.GetNetname() not in redes):
                    continue
                vx, vy = mm(v.GetPosition().x), mm(v.GetPosition().y)
                r = mm(v.GetWidth(pcbnew.F_Cu)) / 2
                camadas = {camada for (rede, camada), ts in trilhas.items() if rede == v.GetNetCode() for o in ts
                           if min(math.hypot(mm(o.GetStart().x) - vx, mm(o.GetStart().y) - vy),
                                  math.hypot(mm(o.GetEnd().x) - vx, mm(o.GetEnd().y) - vy)) <= r}
                if len(camadas) <= 1:   # não troca de face: é via_dangling no DRC
                    soltas.append(v)
        if not soltas:
            return 0
        for t in soltas:
            if t.Type() == pcbnew.PCB_VIA_T:
                print(f"   {t.GetNetname()} via ({mm(t.GetPosition().x):.2f}; {mm(t.GetPosition().y):.2f})")
            else:
                print(f"   {t.GetNetname()} {board.GetLayerName(t.GetLayer())} "
                      f"({mm(t.GetStart().x):.2f}; {mm(t.GetStart().y):.2f})-({mm(t.GetEnd().x):.2f}; {mm(t.GetEnd().y):.2f})")
            board.Remove(t)
        pcbnew.SaveBoard(path, board)
        return len(soltas)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if os.environ.get("APARA_RODADA"):
        sys.exit(10 if rodada(sys.argv[1], set(sys.argv[2:])) else 0)
    total = 0
    while subprocess.run([sys.executable, *sys.argv], env={**os.environ, "APARA_RODADA": "1"}).returncode == 10:
        total += 1
    print(f"{total} rodada(s) com trilha apagada")
