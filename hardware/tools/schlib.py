"""Leitura mínima de .kicad_sch (KiCad 10) para ajuste de layout por script.

Só o necessário: símbolos da lib embutida (pinos), símbolos colocados (posição,
rotação, campos), labels locais e fios. Coordenadas em mm, eixo Y para baixo.
"""
import math
import re
import sys

NUM = r"(-?[\d.]+)"


def block_end(text, start):
    """Índice logo após o ')' que fecha o bloco aberto em text[start] == '('."""
    depth, i, in_str = 0, start, False
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise ValueError("bloco sem fechamento")


def top_blocks(text, tag, start=0, stop=None, indent="\t"):
    """Blocos '(tag' no nível logo abaixo da raiz (uma tabulação)."""
    stop = len(text) if stop is None else stop
    pat = re.compile(r"\n" + indent + r"\(" + re.escape(tag) + r"\b")
    out = []
    pos = start
    while True:
        m = pat.search(text, pos, stop)
        if not m:
            return out
        s = m.start() + 1 + len(indent)
        e = block_end(text, s)
        out.append((s, e))
        pos = e


def rot(dx, dy, ang):
    """Vetor local (Y para cima) → folha (Y para baixo), girado 'ang' graus (anti-horário na tela)."""
    x, y = dx, -dy
    a = math.radians(ang)
    c, s = round(math.cos(a)), round(math.sin(a))
    return (x * c + y * s, -x * s + y * c)


def parse(path):
    text = open(path, encoding="utf-8").read()
    ls = text.index("(lib_symbols")
    le = block_end(text, ls)

    libs = {}
    for s, e in top_blocks(text, "symbol", ls, le, indent="\t\t"):
        blk = text[s:e]
        name = re.match(r'\(symbol "([^"]+)"', blk).group(1)
        pins = []
        for m in re.finditer(
            r"\(pin \w+ \w+\s*\(at " + NUM + " " + NUM + " " + NUM + r"\)\s*\(length " + NUM + r"\).*?\(number \"([^\"]*)\"",
            blk, re.S):
            pins.append(dict(x=float(m.group(1)), y=float(m.group(2)), ang=float(m.group(3)),
                             length=float(m.group(4)), num=m.group(5)))
        libs[name] = pins

    syms = []
    for s, e in top_blocks(text, "symbol", le):
        blk = text[s:e]
        lib = re.search(r'\(lib_id "([^"]+)"\)', blk).group(1)
        at = re.search(r"\(at " + NUM + " " + NUM + " " + NUM + r"\)", blk)
        x, y, r = float(at.group(1)), float(at.group(2)), float(at.group(3))
        mirror = re.search(r"\(mirror (\w)\)", blk)
        fields = {}
        for m in re.finditer(r'\(property "(Reference|Value)" "([^"]*)"\s*\(at ' + NUM + " " + NUM + " " + NUM + r"\)",
                             blk):
            fields[m.group(1)] = dict(text=m.group(2), x=float(m.group(3)), y=float(m.group(4)), ang=float(m.group(5)))
        pins = []
        for p in libs.get(lib, []):
            px, py = rot(p["x"], p["y"], r)
            # o pino entra no corpo na direção 'ang'; para fora é o oposto
            ox, oy = rot(-math.cos(math.radians(p["ang"])), -math.sin(math.radians(p["ang"])), r)
            pins.append(dict(num=p["num"], x=round(x + px, 3), y=round(y + py, 3), ox=round(ox), oy=round(oy)))
        syms.append(dict(ref=fields["Reference"]["text"], lib=lib, x=x, y=y, rot=r,
                         mirror=mirror.group(1) if mirror else None, fields=fields, pins=pins, span=(s, e)))

    labels = []
    for s, e in top_blocks(text, "label"):
        blk = text[s:e]
        m = re.match(r'\(label "([^"]*)"\s*\(at ' + NUM + " " + NUM + " " + NUM + r"\)", blk)
        labels.append(dict(name=m.group(1), x=float(m.group(2)), y=float(m.group(3)), ang=float(m.group(4)), span=(s, e)))

    wires = []
    for s, e in top_blocks(text, "wire"):
        m = re.search(r"\(xy " + NUM + " " + NUM + r"\) \(xy " + NUM + " " + NUM + r"\)", text[s:e])
        wires.append(dict(a=(float(m.group(1)), float(m.group(2))), b=(float(m.group(3)), float(m.group(4))), span=(s, e)))

    return text, libs, syms, labels, wires


def near(a, b, tol=0.02):
    return abs(a[0] - b[0]) < tol and abs(a[1] - b[1]) < tol


def report(path):
    text, libs, syms, labels, wires = parse(path)
    used = set()
    for sy in sorted(syms, key=lambda s: s["ref"]):
        f = sy["fields"]
        print(f"{sy['ref']:7} {sy['lib']:36} at=({sy['x']},{sy['y']}) rot={sy['rot']:.0f}"
              f"{' mirror=' + sy['mirror'] if sy['mirror'] else ''}"
              f" | Ref({f['Reference']['x']},{f['Reference']['y']},{f['Reference']['ang']:.0f})"
              f" Val({f['Value']['x']},{f['Value']['y']},{f['Value']['ang']:.0f})")
        for p in sy["pins"]:
            ls = [l for l in labels if near((l["x"], l["y"]), (p["x"], p["y"]))]
            ws = [w for w in wires if near(w["a"], (p["x"], p["y"])) or near(w["b"], (p["x"], p["y"]))]
            for l in ls:
                used.add(id(l))
            print(f"    pino {p['num']:>3} ({p['x']},{p['y']}) sai=({p['ox']},{p['oy']})"
                  f" labels={[(l['name'], l['ang']) for l in ls]} fios={len(ws)}")
    loose = [l for l in labels if id(l) not in used]
    print(f"\n{len(labels)} labels, {len(wires)} fios; labels fora de pino: {len(loose)}")
    for l in loose:
        print("   ", l["name"], (l["x"], l["y"]), l["ang"])
    for w in wires:
        print("    fio", w["a"], w["b"])


if __name__ == "__main__":
    report(sys.argv[1])
