"""Reduz um DSN a trechos escolhidos, para o Freerouting desenhar só eles numa passada separada.

Por que existe: nas linhas de campo a ordem importa — o surto tem de encontrar o TVS antes do
resistor de série e dos pull-ups. O Freerouting liga uma rede pelo caminho mais curto, não
respeita ordem de pads e deriva em qualquer ponto do cobre da mesma rede — inclusive no meio de
um fio marcado como fixo. A revisão de placa de 2026-09-14 achou o TVS como ramo morto
(ARQUITETURA §11.8). O roteamento das linhas de campo é feito em passadas:

  1. trechos RJ45 → TVS:            REDE=J2-3,U5-4
  2. resto da rede a partir do TVS:  REDE=-J2-3   (todos os pinos, menos o do RJ45)
     com `--fios-viram-keepout`: os fios da passada 1 ficam fixos e ganham, por cima, áreas
     proibidas da mesma largura — o roteador não tem como derivar neles, e a junção só pode ser
     no TVS;
  3. as demais redes, com toda a fiação fixa e as áreas proibidas só sobre as pernas da passada 1
     (`--keepout-de pernas.dsn`, sem ESPEC): completa também o que a passada 2 deixou pela metade
     sem poder derivar nas pernas.

O pad do TVS não pode ficar dentro de área proibida: o Freerouting descarta a rede inteira quando
um pino cai numa. `--livre-perto REDE@X,Y[;REDE@X,Y...]` (mm) corta as áreas proibidas de cada
rede sobre o pad de TVS dela, com 0,15 mm de folga além da borda (tamanho do pad em
`--pad-tvs L,A`, padrão SOT-23-6) — só dela, para fio de outra rede nunca achar brecha sobre o
trecho. Um corte em raio de 1,2 mm do centro deixou o roteador derivar a 0,2 mm da borda do pad
(DHT) e numa face em que o pad nem existe (SCL, em B.Cu); o `ordem_tvs.py` reprovou os dois.

A ordem resultante é provada por `ordem_tvs.py`, não presumida.

Redes que não estão na lista saem da seção `network` e das classes; os pads delas continuam no
DSN como obstáculos.

Uso: python dsn_trechos.py <entrada.dsn> <saida.dsn> [--fios-viram-keepout] [--livre-perto REDE@X,Y;...]
                          [--pad-tvs L,A] [--keepout-de OUTRO.dsn] [--area-proibida X1,Y1,X2,Y2;...] [ESPEC ...]
     --area-proibida: retângulos (mm) proibidos nas duas faces; implica --fios-viram-keepout
     --manter-outras: as redes fora dos ESPEC continuam inteiras (sem isso, saem do DSN)
     --sem-keepout-de: fios e vias iguais aos da fiação de OUTRO.dsn não viram área proibida (o
                   leque do leque_tvs.py, onde o resto da rede precisa se ligar)
     --keepout-de: as áreas proibidas vêm da fiação de OUTRO.dsn (implica --fios-viram-keepout);
                   toda a fiação da entrada fica fixa. Sem ESPEC, todas as redes continuam.
     ESPEC = REDE                   mantém a rede inteira
           | REDE=PINO,PINO[,...]   mantém só esses pinos
           | REDE=-PINO[,-PINO...]  mantém todos os pinos da rede, menos esses
"""
import math
import re
import sys

from schlib import block_end

ISOL_UM = 200      # isolamento acrescentado de cada lado do fio que vira área proibida (µm)
PAD_LIVRE = (1.325, 0.6)   # mm, pad do TVS (SOT-23-6); `--pad-tvs L,A` troca
FOLGA_LIVRE = 0.15         # mm além da borda do pad
PASSO = 0.05       # mm
VIA_UM = 600       # diâmetro da via (µm)


def perto_do_pad(x, y, cx, cy):
    """Ponto a menos de FOLGA_LIVRE da borda do pad retangular centrado em (cx, cy)."""
    return (abs(x - cx) <= PAD_LIVRE[0] / 2 + FOLGA_LIVRE and
            abs(y - cy) <= PAD_LIVRE[1] / 2 + FOLGA_LIVRE)


def cortar_polilinha(pts_um, livres):
    """Divide a polilinha (µm, y negativo do DSN), tirando o que fica sobre os pads livres."""
    pts = [(x / 1000, -y / 1000) for x, y in pts_um]
    amostra = []
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        n = max(1, int(math.hypot(bx - ax, by - ay) / PASSO))
        amostra += [(ax + (bx - ax) * i / n, ay + (by - ay) * i / n) for i in range(n)]
    amostra.append(pts[-1])
    runs, atual = [], []
    for x, y in amostra:
        if not any(perto_do_pad(x, y, lx, ly) for lx, ly in livres):
            atual.append((x, y))
        elif atual:
            runs.append(atual)
            atual = []
    if atual:
        runs.append(atual)
    return [[(round(x * 1000), round(-y * 1000)) for x, y in simplificar(r)] for r in runs if len(r) >= 2]


def simplificar(pts, tol=0.01):
    """Ramer-Douglas-Peucker: devolve só os vértices da polilinha amostrada (mm)."""
    if len(pts) < 3:
        return pts
    (ax, ay), (bx, by) = pts[0], pts[-1]
    L = math.hypot(bx - ax, by - ay) or 1e-9
    k, dmax = 0, -1.0
    for i, (x, y) in enumerate(pts[1:-1], 1):
        d = abs((bx - ax) * (ay - y) - (ax - x) * (by - ay)) / L
        if d > dmax:
            k, dmax = i, d
    if dmax <= tol:
        return [pts[0], pts[-1]]
    return simplificar(pts[:k + 1], tol)[:-1] + simplificar(pts[k:], tol)


def retangulo(a, b, meia):
    """Polígono (µm) que cobre o segmento a-b com meia largura `meia`, pontas estendidas.
    Mesmo sentido de giro dos polígonos que o KiCad exporta (área com sinal negativo)."""
    (ax, ay), (bx, by) = a, b
    L = math.hypot(bx - ax, by - ay) or 1.0
    ux, uy = (bx - ax) / L, (by - ay) / L
    nx, ny = -uy, ux
    ax, ay, bx, by = ax - ux * meia, ay - uy * meia, bx + ux * meia, by + uy * meia
    cantos = [(ax + nx * meia, ay + ny * meia), (bx + nx * meia, by + ny * meia),
              (bx - nx * meia, by - ny * meia), (ax - nx * meia, ay - ny * meia)]
    area = sum(x1 * y2 - x2 * y1 for (x1, y1), (x2, y2) in zip(cantos, cantos[1:] + cantos[:1]))
    if area > 0:
        cantos.reverse()
    cantos.append(cantos[0])
    return [(round(x), round(y)) for x, y in cantos]


def area_proibida(x1, y1, x2, y2):
    """Retângulo (mm, coordenadas da placa) proibido nas duas faces."""
    xs, ys = sorted((x1, x2)), sorted((y1, y2))
    c = [(xs[0], ys[0]), (xs[1], ys[0]), (xs[1], ys[1]), (xs[0], ys[1]), (xs[0], ys[0])]
    coords = " ".join(f"{round(x * 1000)} {round(-y * 1000)}" for x, y in c)
    return [f'    (keepout "" (polygon {cam} 0 {coords}))' for cam in ("F.Cu", "B.Cu")]


def assinaturas(texto):
    """Fios e vias da fiação de um DSN, como chaves comparáveis (camada, rede, pontos)."""
    i = texto.index("(wiring")
    w = texto[i:block_end(texto, i)]
    fios = {(m.group(1), m.group(4).strip('"'), frozenset(zip(*[iter(map(int, m.group(3).split()))] * 2)))
            for m in re.finditer(r"\(wire\s+\(path\s+(\S+)\s+(\d+)((?:\s+-?\d+)+)\s*\)\s*\(net\s+([^)\s]+)\)", w)}
    vias = {(m.group(1), m.group(2)) for m in re.finditer(r'\(via\s+"[^"]*"\s+(-?\d+)\s+(-?\d+)', w)}
    return fios, vias


def fios_para_keepout(t, livres, fonte=None, areas_extra=(), poupar=None):
    """Cobre a fiação existente com áreas proibidas e a marca como fixa.

    A fiação FICA no DSN, fixa (`type protect`): as outras redes a enxergam como cobre e guardam
    isolamento — tirá-la da fiação deixou fios de outra rede cruzarem o fim dos trechos (curto).
    Por cima vai uma área proibida da MESMA largura do cobre: é o que impede a própria rede de
    derivar ali. Ela é cortada só perto do pad de TVS da própria rede (`livres`: {rede: [(x, y)]}),
    para o roteador conseguir entrar no pad; uma derivação nesse trecho final é pega pelo
    `ordem_tvs.py`.

    `fonte`: texto de outro DSN cuja fiação dá as áreas proibidas (p.ex. só as pernas RJ45 → TVS),
    enquanto toda a fiação de `t` fica fixa.
    """
    i = t.index("(wiring")
    j = block_end(t, i)
    wiring = t[i:j]
    if fonte is None:
        fiacao_ko = wiring
    else:
        fi = fonte.index("(wiring")
        fiacao_ko = fonte[fi:block_end(fonte, fi)]
    ko = []
    areas_extra = list(areas_extra)
    n_fios = 0
    for m in re.finditer(r"\(wire\s+\(path\s+(\S+)\s+(\d+)((?:\s+-?\d+)+)\s*\)\s*\(net\s+([^)\s]+)\)", fiacao_ko):
        camada, largura, rede = m.group(1), int(m.group(2)), m.group(4).strip('"')
        nums = list(map(int, m.group(3).split()))
        if poupar and (camada, rede, frozenset(zip(nums[0::2], nums[1::2]))) in poupar[0]:
            continue
        # o Freerouting ignora área proibida em forma de caminho ("keepout is not an area"):
        # cada segmento vira um retângulo
        for run in cortar_polilinha(list(zip(nums[0::2], nums[1::2])), livres.get(rede, [])):
            for a, b in zip(run, run[1:]):
                if math.hypot(b[0] - a[0], b[1] - a[1]) < 20:   # trecho degenerado (< 20 µm) trava o Freerouting
                    continue
                coords = " ".join(f"{x} {y}" for x, y in retangulo(a, b, largura / 2))
                ko.append(f'    (wire_keepout "" (polygon {camada} 0 {coords}))')
                n_fios += 1
    n_vias = 0
    for m in re.finditer(r'\(via\s+"[^"]*"\s+(-?\d+)\s+(-?\d+)\s*\(net\s+([^)\s]+)\)', fiacao_ko):
        if poupar and (m.group(1), m.group(2)) in poupar[1]:
            continue
        vx, vy = int(m.group(1)) / 1000, -int(m.group(2)) / 1000
        rede = m.group(3).strip('"')
        if any(perto_do_pad(vx, vy, lx, ly) for lx, ly in livres.get(rede, [])):
            continue
        for camada in ("F.Cu", "B.Cu"):
            ko.append(f'    (wire_keepout "" (circle {camada} {VIA_UM} {m.group(1)} {m.group(2)}))')
        n_vias += 1
    t = t[:i] + re.sub(r"\(type route\)", "(type protect)", wiring) + t[j:]
    ko += areas_extra
    if ko:
        s = t.index("(structure")
        k = t.index("\n    (via ", s)   # primeira declaração (via ...) da estrutura
        t = t[:k] + "\n" + "\n".join(ko) + t[k:]
    return t, n_fios, n_vias


def main(entrada, saida, pedidos, keepout, livres, fonte=None, areas=(), poupar=None, manter_outras=False):
    alvo = {}
    for p in pedidos:
        nome, _, pinos = p.partition("=")
        alvo[nome] = pinos.split(",") if pinos else []   # só o nome: todos os pinos

    t = open(entrada, encoding="utf-8", newline="").read()
    if keepout:
        t, n_fios, n_vias = fios_para_keepout(t, livres, fonte, areas, poupar)
        print(f"{n_fios} trecho(s) de fio e {n_vias} via(s) viraram áreas proibidas ({len(livres)} ponto(s) livres)")

    if not pedidos:   # sem ESPEC: todas as redes continuam
        open(saida, "w", encoding="utf-8", newline="").write(t)
        print("todas as redes mantidas")
        return

    ini = t.index("(network")
    fim = block_end(t, ini)
    rede = t[ini:fim]

    vistos, resumo = set(), {}
    saida_rede, pos = [], 0
    for m in re.finditer(r"\(net (\S+)", rede):
        a = m.start()
        if a < pos:
            continue
        b = block_end(rede, a)
        nome = m.group(1).strip('"')
        saida_rede.append(rede[pos:a])
        if nome in alvo:
            original = re.search(r"\(pins ([^)]*)\)", rede[a:b]).group(1).split()
            pedidos_rede = alvo[nome]
            if not pedidos_rede:
                faltam, pinos = [], original
            elif all(x.startswith("-") for x in pedidos_rede):
                tirar = [x[1:] for x in pedidos_rede]
                faltam = [x for x in tirar if x not in original]
                pinos = [x for x in original if x not in tirar]
            else:
                faltam = [x for x in pedidos_rede if x not in original]
                pinos = pedidos_rede
            if faltam:
                sys.exit(f"{nome}: pinos {faltam} não pertencem à rede no DSN ({original})")
            saida_rede.append(f'(net {m.group(1)}\n      (pins {" ".join(pinos)})\n    )')
            vistos.add(nome)
            resumo[nome] = len(pinos)
        elif manter_outras:
            saida_rede.append(rede[a:b])
        else:
            anterior = saida_rede.pop()
            saida_rede.append(anterior.rstrip(" \t").rstrip("\r\n") if anterior.strip() == "" else anterior)
        pos = b
    saida_rede.append(rede[pos:])
    nova_rede = "".join(saida_rede)

    ausentes = set(alvo) - vistos
    if ausentes:
        sys.exit(f"redes pedidas que não existem no DSN: {sorted(ausentes)}")

    def limpa_classe(mm):
        cab, corpo = mm.group(1), mm.group(2)
        tokens = [x for x in corpo.split() if x.strip('"') in alvo]
        return cab + (" " + " ".join(tokens) if tokens else "")

    if not manter_outras:
        nova_rede = re.sub(r"(\(class \S+)((?:\s+[^\s()]+)*)", limpa_classe, nova_rede)

    t = t[:ini] + nova_rede + t[fim:]
    open(saida, "w", encoding="utf-8", newline="").write(t)
    print(f"{len(alvo)} rede(s) mantidas: " + ", ".join(f"{k}({v})" for k, v in resumo.items()))


if __name__ == "__main__":
    argv = sys.argv[1:]
    livres = {}
    if "--livre-perto" in argv:
        k = argv.index("--livre-perto")
        for item in argv[k + 1].split(";"):
            rede, xy = item.split("@")
            livres.setdefault(rede, []).append(tuple(map(float, xy.split(","))))
        del argv[k:k + 2]
    if "--pad-tvs" in argv:
        k = argv.index("--pad-tvs")
        PAD_LIVRE = tuple(map(float, argv[k + 1].split(",")))
        del argv[k:k + 2]
    areas = []
    if "--area-proibida" in argv:
        k = argv.index("--area-proibida")
        for item in argv[k + 1].split(";"):
            areas += area_proibida(*map(float, item.split(",")))
        del argv[k:k + 2]
    poupar = None
    if "--sem-keepout-de" in argv:
        k = argv.index("--sem-keepout-de")
        poupar = assinaturas(open(argv[k + 1], encoding="utf-8", newline="").read())
        del argv[k:k + 2]
    fonte = None
    if "--keepout-de" in argv:
        k = argv.index("--keepout-de")
        fonte = open(argv[k + 1], encoding="utf-8", newline="").read()
        del argv[k:k + 2]
    keepout = "--fios-viram-keepout" in argv or fonte is not None or bool(areas)
    manter = "--manter-outras" in argv
    args = [a for a in argv if a not in ("--fios-viram-keepout", "--manter-outras")]
    if len(args) < 3 and not (keepout and len(args) == 2):
        sys.exit(__doc__)
    main(args[0], args[1], args[2:], keepout, livres, fonte, areas, poupar, manter)
