"""Compara duas netlists do KiCad (formato kicadxml) e acusa qualquer mudança de conectividade.

É o portão de segurança do fluxo: scripts que mexem no esquemático (fix_layout.py à frente)
desenham fios, e um fio que encoste no pino de outra net funde as duas **em silêncio** — a ERC
não acusa, porque o curto é eletricamente válido. Só a netlist mostra.

Gerar as duas pontas com:

    kicad-cli sch export netlist --format kicadxml -o antes.net  placa.kicad_sch
    <mexe no esquemático>
    kicad-cli sch export netlist --format kicadxml -o depois.net placa.kicad_sch
    python netdiff.py antes.net depois.net

Sai com código 1 se divergirem, 0 se forem idênticas — dá para usar em cadeia com &&.

Recusa netlist sem nenhuma net: comparar dois conjuntos vazios "passa" e não prova nada.

Uso: python netdiff.py <antes.net> <depois.net> [-v]
     -v lista todas as nets, mesmo quando idênticas.
"""
import sys
import xml.etree.ElementTree as ET


def carrega(path):
    raiz = ET.parse(path).getroot()
    nets = raiz.find("nets")
    if nets is None:
        sys.exit(f"{path}: sem seção <nets> — isto é netlist kicadxml?")
    fora = {}
    for net in nets.findall("net"):
        nome = net.get("name")
        fora[nome] = sorted((n.get("ref"), n.get("pin")) for n in net.findall("node"))
    if not fora:
        sys.exit(f"{path}: nenhuma net encontrada — não dá para comparar com isso")
    return fora


def descreve(d):
    return f"{len(d)} nets, {sum(len(v) for v in d.values())} nós"


def main(p_antes, p_depois, verboso=False):
    a = carrega(p_antes)
    b = carrega(p_depois)
    print(f"antes  : {descreve(a)}   ({p_antes})")
    print(f"depois : {descreve(b)}   ({p_depois})")

    if a == b:
        print("\nIDÊNTICAS — a conectividade não mudou")
        if verboso:
            for k in sorted(a):
                print(f"  {k:28} {len(a[k]):2}  {' '.join(r + '.' + p for r, p in a[k])}")
        return 0

    print("\nDIVERGEM:\n")
    for k in sorted(set(a) | set(b)):
        if a.get(k) == b.get(k):
            continue
        if k not in b:
            print(f"  SUMIU    {k}: {a[k]}")
        elif k not in a:
            print(f"  SURGIU   {k}: {b[k]}")
        else:
            so_a = sorted(set(a[k]) - set(b[k]))
            so_b = sorted(set(b[k]) - set(a[k]))
            print(f"  MUDOU    {k}")
            if so_a:
                print(f"      saíram : {' '.join(r + '.' + p for r, p in so_a)}")
            if so_b:
                print(f"      entraram: {' '.join(r + '.' + p for r, p in so_b)}")
    return 1


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "-v"]
    if len(args) != 2:
        sys.exit(__doc__)
    sys.exit(main(args[0], args[1], "-v" in sys.argv))
