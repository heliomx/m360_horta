"""Aplica o padrão de layout de labels e campos que o usuário fez à mão no R6.

- label sai do pino: fica na ponta de um fio curto, com o texto correndo ao lado do fio
  (orientação do label preservada; comprimento do fio = comprimento do texto, grade 1,27);
- R/C verticais: Reference e Value em pé, ao lado do corpo (offsets do R6);
- demais símbolos: campos fora do corpo;
- grade de solo deslocada +3,81 mm para alinhar com o R6 (só com --lp).

Uso: python fix_layout.py <arquivo.kicad_sch> [--lp | --act]   (reescreve o arquivo)
"""
import math
import re
import sys
import uuid

from schlib import NUM, block_end, near, parse, rot, top_blocks

G = 1.27
# Os ajustes abaixo são específicos da M360-LP e só valem com a opção --lp. Sem ela, o script
# aplica apenas a regra geral: label na ponta de um fio curto e campos fora do corpo.
# (Na ACT, R1, C4 e U2 existem com outro significado — no 74HCT244 os pinos 7 e 8 são 2Y2 e
# 1A3 —, então rodar os casos da LP aqui corromperia a fiação.)
LP = "--lp" in sys.argv
SKIP = {"R6"} if LP else set()    # R6 foi editado à mão pelo usuário
GRID_SHIFT = 3.81                 # alinha a grade de solo com o R6
GRID = ([f"R{i}" for i in range(7, 26)] + [f"C{i}" for i in range(8, 16)]) if LP else []

# Na M360-ACT os cinco TVS ficaram a 33 mm um do outro. Cada SRV05-4 tem VP para cima e VN
# para baixo, então sobravam 7,6 mm entre vizinhos — não cabem dois fios de 6,35 mm de nets
# diferentes, e o label do VP acabava em cima do símbolo. Espaça a coluna.
ACT = "--act" in sys.argv
ACT_SHIFT = ({"U5": (0.0, 7.62), "U6": (0.0, 15.24), "U7": (0.0, 22.86), "U8": (0.0, 30.48)}
             if ACT else {})


def fmt(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def stub_len(name):
    return max(5.08, round(math.ceil((len(name) * 1.0 + 2.5) / G - 1e-9) * G, 2))


def lib_bboxes(text):
    """Caixa do desenho (sem pinos) de cada símbolo da lib embutida, em coordenadas locais."""
    ls = text.index("(lib_symbols")
    le = block_end(text, ls)
    out = {}
    for s, e in top_blocks(text, "symbol", ls, le, indent="\t\t"):
        blk = text[s:e]
        name = re.match(r'\(symbol "([^"]+)"', blk).group(1)
        # remove os blocos de pino
        parts, i = [], 0
        for m in re.finditer(r"\(pin ", blk):
            if m.start() < i:
                continue
            parts.append(blk[i:m.start()])
            i = block_end(blk, m.start())
        parts.append(blk[i:])
        g = "".join(parts)
        g = re.sub(r'\(property .*?\n\t\t\t\)', "", g, flags=re.S)
        pts = [(float(a), float(b)) for a, b in re.findall(r"\((?:xy|start|end|mid) " + NUM + " " + NUM + r"\)", g)]
        for cx, cy, r in re.findall(r"\(circle\s*\(center " + NUM + " " + NUM + r"\)\s*\(radius " + NUM + r"\)", g):
            cx, cy, r = float(cx), float(cy), float(r)
            pts += [(cx - r, cy - r), (cx + r, cy + r)]
        if pts:
            out[name] = (min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts), max(p[1] for p in pts))
    return out


def sheet_bbox(sym, local):
    x1, y1, x2, y2 = local
    pts = [rot(x, y, sym["rot"]) for x in (x1, x2) for y in (y1, y2)]
    return (sym["x"] + min(p[0] for p in pts), sym["y"] + min(p[1] for p in pts),
            sym["x"] + max(p[0] for p in pts), sym["y"] + max(p[1] for p in pts))


def field_block(name, value, x, y, ang, hide=False, justify=None):
    lines = [f'\t\t(property "{name}" "{value}"', f"\t\t\t(at {fmt(x)} {fmt(y)} {fmt(ang)})"]
    if hide:
        lines.append("\t\t\t(hide yes)")
    lines += ["\t\t\t(show_name no)", "\t\t\t(do_not_autoplace no)", "\t\t\t(effects",
              "\t\t\t\t(font", "\t\t\t\t\t(size 1.27 1.27)", "\t\t\t\t)"]
    if justify:
        lines.append(f"\t\t\t\t(justify {justify})")
    lines += ["\t\t\t)", "\t\t)"]
    return "\n".join(lines)


def replace_field(blk, name, newtxt):
    i = blk.index(f'\t\t(property "{name}"')
    j = block_end(blk, i + 2)
    return blk[:i] + newtxt + blk[j:]


def main(path):
    text, libs, syms, labels, wires = parse(path)
    bboxes = lib_bboxes(text)
    # Modelo de fio: copia um existente quando há, senão usa o literal abaixo — um esquemático
    # recém-criado só com labels não tem nenhum fio para servir de molde.
    WIRE_TPL = ('(wire\n\t\t(pts\n\t\t\t(xy 0 0) (xy 0 0)\n\t\t)\n\t\t(stroke\n\t\t\t(width 0)\n'
                '\t\t\t(type default)\n\t\t)\n\t\t(uuid "0")\n\t)')
    wire_tpl = text[wires[0]["span"][0]:wires[0]["span"][1]] if wires else WIRE_TPL
    label_tpl = text[labels[0]["span"][0]:labels[0]["span"][1]]

    edits = []                    # (início, fim, novo texto) no texto original
    new_items = []                # blocos novos (fios, junções, labels)
    new_wires = []                # para a checagem de conectividade

    def add_wire(a, b):
        blk = re.sub(r"\(xy [^)]*\) \(xy [^)]*\)", f"(xy {fmt(a[0])} {fmt(a[1])}) (xy {fmt(b[0])} {fmt(b[1])})", wire_tpl)
        blk = re.sub(r'\(uuid "[^"]*"\)', f'(uuid "{uuid.uuid4()}")', blk)
        new_items.append(blk)
        new_wires.append((a, b))

    def add_junction(p):
        new_items.append(f"(junction\n\t\t(at {fmt(p[0])} {fmt(p[1])})\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n"
                         f'\t\t(uuid "{uuid.uuid4()}")\n\t)')

    def add_label(name, p, ang, justify):
        blk = re.sub(r'^\(label "[^"]*"', f'(label "{name}"', label_tpl)
        blk = re.sub(r"\(at [^)]*\)", f"(at {fmt(p[0])} {fmt(p[1])} {fmt(ang)})", blk, count=1)
        blk = re.sub(r"\(justify [^)]*\)", f"(justify {justify})", blk)
        blk = re.sub(r'\(uuid "[^"]*"\)', f'(uuid "{uuid.uuid4()}")', blk)
        new_items.append(blk)

    def delete(span):
        s, e = span
        assert text[s - 2:s] == "\n\t"
        edits.append((s - 2, e, ""))

    def move_label(lab, p):
        s, e = lab["span"]
        blk = re.sub(r"\(at [^)]*\)", f"(at {fmt(p[0])} {fmt(p[1])} {fmt(lab['ang'])})", text[s:e], count=1)
        edits.append((s, e, blk))

    # --- pinos (coordenadas antigas) e seus labels ---------------------------------------
    by_ref = {s["ref"]: s for s in syms}
    pin_labels = {}
    for sy in syms:
        for p in sy["pins"]:
            pin_labels[(sy["ref"], p["num"])] = [l for l in labels if near((l["x"], l["y"]), (p["x"], p["y"]))]

    shift = {r: (GRID_SHIFT, 0.0) for r in GRID}
    shift.update(ACT_SHIFT)

    # Os marcadores de "sem ligação" (X) são blocos soltos, presos à coordenada do pino: quando
    # o símbolo é deslocado eles precisam ir junto, senão sobram o pino sem X e o X sem pino.
    desloc_por_ponto = {}
    for sy in syms:
        d = shift.get(sy["ref"])
        if d:
            for p in sy["pins"]:
                desloc_por_ponto[(round(p["x"], 2), round(p["y"], 2))] = d
    movidos = 0
    for m in re.finditer(r"\n\t\(no_connect\s*\(at " + NUM + " " + NUM + r"\)", text):
        pt = (round(float(m.group(1)), 2), round(float(m.group(2)), 2))
        d = desloc_por_ponto.get(pt)
        if d:
            edits.append((m.start(1), m.end(2), f"{fmt(pt[0] + d[0])} {fmt(pt[1] + d[1])}"))
            movidos += 1
    if movidos:
        print(f"{movidos} marcador(es) de sem-ligação deslocados junto com o símbolo")

    def new_pin(ref, num):
        sy = by_ref[ref]
        p = next(q for q in sy["pins"] if q["num"] == num)
        dx, dy = shift.get(ref, (0.0, 0.0))
        return (round(p["x"] + dx, 3), round(p["y"] + dy, 3)), p

    # --- casos especiais (só na M360-LP) ----------------------------------------------------
    handled = set()
    if LP:
        # nó VBAT_SENSE: R1.2 — R2.1 já têm fio; liga R2.1 ao C4.1 e deixa um só label
        for key in (("R1", "2"), ("C4", "1")):
            for lab in pin_labels[key]:
                delete(lab["span"])
            handled.add(key)
        r2, _ = new_pin("R2", "1")
        c4, _ = new_pin("C4", "1")
        add_wire(r2, c4)
        add_junction(r2)
        add_label("VBAT_SENSE", c4, 180, "right bottom")
        handled.add(("R2", "1"))

        # U2 (74HC4051): GND dos pinos 7 (VEE) e 8 unidos por fio, um só label
        for lab in pin_labels[("U2", "7")]:
            delete(lab["span"])
        p7, _ = new_pin("U2", "7")
        p8, _ = new_pin("U2", "8")
        mid = (p8[0], round(p8[1] + 2.54, 3))
        add_wire(p7, (p7[0], mid[1]))
        add_wire((p7[0], mid[1]), mid)
        add_wire(p8, mid)
        end8 = (p8[0], round(p8[1] + 6.35, 3))
        add_wire(mid, end8)
        add_junction(mid)
        for lab in pin_labels[("U2", "8")]:
            move_label(lab, end8)
        handled |= {("U2", "7"), ("U2", "8")}

    # --- regra geral dos labels -------------------------------------------------------------
    # O fio curto não pode encostar em pino ou fio de OUTRA net: isso criaria ligação falsa
    # (foi assim que, sem esta checagem, GND e +5V viraram uma net só na ACT). Encostar em algo
    # da mesma net é inofensivo e fica permitido.
    net_do_pino, pontos = {}, []
    for sy in syms:
        dx, dy = shift.get(sy["ref"], (0.0, 0.0))
        for p in sy["pins"]:
            key = (sy["ref"], p["num"])
            pontos.append(((round(p["x"] + dx, 3), round(p["y"] + dy, 3)), key))
            ls = pin_labels[key]
            net_do_pino[key] = ls[0]["name"] if ls else None

    stubs = []                      # (início, fim, net) dos fios já criados

    def livre(a, d, L, key, net):
        """Maior comprimento utilizável a partir de 'a' na direção axial 'd'."""
        lim = L

        def corta(pt):
            t = (pt[0] - a[0]) * d[0] + (pt[1] - a[1]) * d[1]
            lat = abs((pt[0] - a[0]) * d[1] - (pt[1] - a[1]) * d[0])
            return t - 1.27 if 0 < t <= L + 0.6 and lat < 0.4 else None

        for pt, k in pontos:
            if k == key or net_do_pino.get(k) == net:
                continue
            c = corta(pt)
            if c is not None:
                lim = min(lim, c)
        for s, e, n in stubs:
            if n == net:
                continue
            for pt in (s, e):
                c = corta(pt)
                if c is not None:
                    lim = min(lim, c)
            # cruzamento perpendicular com o fio já existente
            if abs(s[0] - e[0]) < 1e-6 and abs(d[1]) < 1e-6:
                t = (s[0] - a[0]) * d[0]
                if 0 < t <= L and min(s[1], e[1]) - 0.4 <= a[1] <= max(s[1], e[1]) + 0.4:
                    lim = min(lim, t - 1.27)
            if abs(s[1] - e[1]) < 1e-6 and abs(d[0]) < 1e-6:
                t = (s[1] - a[1]) * d[1]
                if 0 < t <= L and min(s[0], e[0]) - 0.4 <= a[0] <= max(s[0], e[0]) + 0.4:
                    lim = min(lim, t - 1.27)
        return lim

    pulados = 0
    for sy in syms:
        if sy["ref"] in SKIP:
            continue
        for p in sy["pins"]:
            key = (sy["ref"], p["num"])
            if key in handled:
                continue
            for lab in pin_labels[key]:
                pn, _ = new_pin(*key)
                d = (p["ox"], p["oy"])
                L = round(min(stub_len(lab["name"]), livre(pn, d, stub_len(lab["name"]), key, lab["name"])), 3)
                if L < 2.54:
                    pulados += 1
                    print(f"   sem espaço em {key[0]}.{key[1]} ({lab['name']}): label fica no pino")
                    continue
                end = (round(pn[0] + d[0] * L, 3), round(pn[1] + d[1] * L, 3))
                add_wire(pn, end)
                stubs.append((pn, end, lab["name"]))
                move_label(lab, end)
    if pulados:
        print(f"{pulados} label(s) ficaram no pino por falta de espaço")

    # --- campos e deslocamento dos símbolos ------------------------------------------------
    for sy in syms:
        ref = sy["ref"]
        if ref in SKIP:
            continue
        s, e = sy["span"]
        blk = text[s:e]
        dx, dy = shift.get(ref, (0.0, 0.0))
        if dx or dy:
            blk = re.sub(r"\(at " + NUM + " " + NUM + " " + NUM + r"\)",
                         lambda m: f"(at {fmt(float(m.group(1)) + dx)} {fmt(float(m.group(2)) + dy)} {m.group(3)})", blk)
        x, y = sy["x"] + dx, sy["y"] + dy
        rv, vv = sy["fields"]["Reference"]["text"], sy["fields"]["Value"]["text"]
        lib = sy["lib"]
        if lib == "Device:R" and sy["rot"] == 0:
            fr = field_block("Reference", rv, x - 2.032, y - 0.254, 90)
            fv = field_block("Value", vv, x + 2.286, y - 0.254, 90)
        elif lib == "Device:C" and sy["rot"] == 0:
            fr = field_block("Reference", rv, x - 3.175, y, 90)
            fv = field_block("Value", vv, x + 3.175, y, 90)
        elif lib == "Device:R" and sy["rot"] in (90, 270):
            fr = field_block("Reference", rv, x, y - 2.286, 0)
            fv = field_block("Value", vv, x, y + 2.286, 0)
        elif lib == "power:PWR_FLAG":
            fr = field_block("Reference", rv, x, y - 1.905, 0, hide=True)
            fv = field_block("Value", vv, x, y - 3.81, 0)
        elif lib == "Transistor_FET:AO3401A":
            side = 1 if sy["rot"] == 0 else -1
            just = "left" if side == 1 else "right"
            fr = field_block("Reference", rv, x + side * 5.08, y - 1.27, 0, justify=just)
            fv = field_block("Value", vv, x + side * 5.08, y + 1.27, 0, justify=just)
        else:
            x1, y1, x2, y2 = sheet_bbox(dict(sy, x=x, y=y), bboxes[lib])
            cx = (x1 + x2) / 2
            tops = [q for q in sy["pins"] if q["oy"] == -1]
            bots = [q for q in sy["pins"] if q["oy"] == 1]
            if tops:
                fr = field_block("Reference", rv, max(q["x"] for q in tops) + dx + G, y1 - 1.905, 0, justify="left")
            else:
                fr = field_block("Reference", rv, cx, y1 - 1.905, 0)
            if bots:
                fv = field_block("Value", vv, max(q["x"] for q in bots) + dx + G, y2 + 1.905, 0, justify="left")
            else:
                fv = field_block("Value", vv, cx, y2 + 1.905, 0)
        blk = replace_field(blk, "Reference", fr)
        blk = replace_field(blk, "Value", fv)
        edits.append((s, e, blk))

    # --- checagem: fio novo não pode encostar em pino alheio; label não pode cair em pino ----
    all_pins = []
    for sy in syms:
        dx, dy = shift.get(sy["ref"], (0.0, 0.0))
        for p in sy["pins"]:
            all_pins.append((sy["ref"], p["num"], (round(p["x"] + dx, 3), round(p["y"] + dy, 3))))

    def on_seg(pt, a, b):
        (x, y), (ax, ay), (bx, by) = pt, a, b
        if abs((bx - ax) * (y - ay) - (by - ay) * (x - ax)) > 1e-3:
            return False
        return min(ax, bx) - 1e-3 <= x <= max(ax, bx) + 1e-3 and min(ay, by) - 1e-3 <= y <= max(ay, by) + 1e-3

    problems = 0
    for a, b in new_wires:
        hits = [(r, n) for r, n, pt in all_pins if on_seg(pt, a, b)]
        if len(hits) > 1 or (hits and not (near(a, hits and next(pt for r, n, pt in all_pins if (r, n) == hits[0])) or
                                           near(b, next(pt for r, n, pt in all_pins if (r, n) == hits[0])))):
            print("ATENÇÃO fio", a, b, "encosta em", hits)
            problems += 1

    # --- aplica ------------------------------------------------------------------------------
    edits.sort(key=lambda t: t[0])
    for (s1, e1, _), (s2, e2, _) in zip(edits, edits[1:]):
        assert e1 <= s2, f"edições sobrepostas em {s1}"
    out, pos = [], 0
    for s, e, rep in edits:
        out.append(text[pos:s])
        out.append(rep)
        pos = e
    out.append(text[pos:])
    new_text = "".join(out)
    anchor = new_text.index("\n\t(sheet_instances")
    new_text = new_text[:anchor] + "".join("\n\t" + b for b in new_items) + new_text[anchor:]
    open(path, "w", encoding="utf-8", newline="\n").write(new_text)
    print(f"{len(new_wires)} fios novos, {sum(1 for b in new_items if b.startswith('(junction'))} junções, "
          f"{len(edits)} blocos editados, {problems} problemas")


if __name__ == "__main__":
    main(sys.argv[1])
