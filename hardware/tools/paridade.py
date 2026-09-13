"""Confere se a placa tem as MESMAS ligações que o esquemático — pela topologia, não pelo nome.

Por que existe: a paridade do DRC (`kicad-cli pcb drc --schematic-parity`) acusava 237 itens na
LP, 195 na ACT e 69 na GW com as placas corretas. A sincronização pelo MCP grava os nomes de rede
num formato diferente do esquemático (sem o prefixo "/", pad sem rede onde o esquemático diz
"unconnected-(…)"), e o DRC compara nomes. Com esse ruído ele não prova nada.

Aqui cada rede vira o CONJUNTO de pads (REF.pad) que ela liga, dos dois lados:
  - esquemático: netlist kicadxml exportada pelo kicad-cli;
  - placa: redes dos pads no .kicad_pcb.
Redes de um pad só e pads sem rede não ligam nada e são ignoradas. Símbolos sem footprint
(PWR_FLAG) saem das redes do esquemático, porque não vão à placa. Um símbolo COM footprint que
não aparece na placa é acusado à parte.

Validado com teste negativo: numa cópia da ACT com o pad 1 do R2 trocado de +5V para 3V3, as
duas redes aparecem como divergentes.

Sai com código 1 se algo divergir.

Uso: python paridade.py <pasta da placa>          (acha o .kicad_sch e o .kicad_pcb)
     KICAD_CLI=<caminho> para outro kicad-cli
"""
import glob
import os
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from schlib import block_end

KICAD_CLI = os.environ.get("KICAD_CLI", r"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe")


def um_arquivo(pasta, ext):
    achados = glob.glob(os.path.join(pasta, "*" + ext))
    if len(achados) != 1:
        sys.exit(f"esperado exatamente um *{ext} em {pasta}, achei {len(achados)}")
    return achados[0]


def grupos(ligacoes):
    """{rede: {(ref, pad)}} -> {frozenset de pads: rede}, só redes com 2 pads ou mais."""
    return {frozenset(p): nome for nome, p in ligacoes.items() if len(p) >= 2}


def do_esquematico(sch):
    """Devolve ({rede: {(ref, pino)}}, {refs com footprint})."""
    with tempfile.TemporaryDirectory() as tmp:
        net = os.path.join(tmp, "p.net")
        r = subprocess.run([KICAD_CLI, "sch", "export", "netlist", "--format", "kicadxml",
                            "-o", net, sch], capture_output=True, text=True)
        if r.returncode != 0 or not os.path.exists(net):
            sys.exit(f"kicad-cli falhou ao exportar a netlist:\n{r.stdout}\n{r.stderr}")
        raiz = ET.parse(net).getroot()
    com_footprint = {c.get("ref") for c in raiz.find("components").findall("comp")
                     if (c.findtext("footprint") or "").strip()}
    ligacoes = {}
    for n in raiz.find("nets").findall("net"):
        ligacoes[n.get("name")] = {(x.get("ref"), x.get("pin")) for x in n.findall("node")}
    if not ligacoes:
        sys.exit("netlist sem nenhuma rede — não dá para comparar com isso")
    return ligacoes, com_footprint


def da_placa(pcb):
    """Devolve ({rede: {(ref, pad)}}, {refs na placa})."""
    texto = open(pcb, encoding="utf-8").read()
    ligacoes, refs = {}, set()
    for m in re.finditer(r"\n\t\(footprint ", texto):
        ini = m.start() + 1
        bloco = texto[ini:block_end(texto, ini)]
        ref = re.search(r'\(property "Reference" "([^"]+)"', bloco).group(1)
        refs.add(ref)
        for pm in re.finditer(r'\n\t\t\(pad "([^"]*)"', bloco):
            if not pm.group(1):
                continue  # furo não metalizado
            j = pm.start() + 1
            pad = bloco[j:block_end(bloco, j)]
            rede = re.search(r'\(net (?:\d+ )?"([^"]*)"\)', pad)
            if rede and rede.group(1):
                ligacoes.setdefault(rede.group(1), set()).add((ref, pm.group(1)))
    if not ligacoes:
        sys.exit("placa sem nenhum pad ligado — não dá para comparar com isso")
    return ligacoes, refs


def fmt(pads):
    return " ".join(f"{r}.{p}" for r, p in sorted(pads))


def main(pasta):
    sch, pcb = um_arquivo(pasta, ".kicad_sch"), um_arquivo(pasta, ".kicad_pcb")
    l_sch, com_fp = do_esquematico(sch)
    l_pcb, refs_pcb = da_placa(pcb)

    # símbolo sem footprint (PWR_FLAG) não vai à placa: tirar os pinos dele das redes
    so_simbolo = {r for p in l_sch.values() for r, _ in p} - com_fp
    l_sch = {n: {x for x in p if x[0] not in so_simbolo} for n, p in l_sch.items()}

    g_sch, g_pcb = grupos(l_sch), grupos(l_pcb)
    print(f"esquemático: {len(g_sch)} redes ligando 2+ pads   ({os.path.basename(sch)})")
    print(f"placa      : {len(g_pcb)} redes ligando 2+ pads   ({os.path.basename(pcb)})")

    faltam = sorted(com_fp - refs_pcb)
    so_sch = sorted(set(g_sch) - set(g_pcb), key=lambda s: g_sch[s])
    so_pcb = sorted(set(g_pcb) - set(g_sch), key=lambda s: g_pcb[s])
    if not (faltam or so_sch or so_pcb):
        print("\nIDÊNTICAS — a placa liga exatamente o que o esquemático liga")
        return 0

    if faltam:
        print(f"\nFALTAM NA PLACA (têm footprint no esquemático): {', '.join(faltam)}")
    if so_sch or so_pcb:
        print("\nDIVERGEM:")
        for s in so_sch:
            print(f"  só no esquemático  {g_sch[s]}: {fmt(s)}")
        for s in so_pcb:
            print(f"  só na placa        {g_pcb[s]}: {fmt(s)}")
    return 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1]))
