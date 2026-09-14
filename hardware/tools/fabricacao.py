"""Gera os arquivos de fabricação e montagem (PCBA) da JLCPCB para uma placa da família.

Saída em <pasta>/fabricacao/:
  <PLACA>_gerbers.zip   Gerbers (sem X2, máscara subtraída da serigrafia) + furação Excellon em mm,
                        PTH e NPTH separados — é o que sobe no pedido de PCB
  <PLACA>_bom.csv       BOM no formato da JLCPCB: Comment, Designator, Footprint, LCSC Part #
  <PLACA>_cpl.csv       Posição no formato da JLCPCB: Designator, Mid X, Mid Y, Layer, Rotation

Antes de gerar, recusa placa com ERC ou DRC (--severity-all) sujos: arquivo de fabricação de placa
com violação não sai daqui. Depois confere que BOM e posição listam as mesmas referências e que
toda linha do BOM tem código LCSC.

As peças de solda manual (tools/solda_manual.py) já saem do BOM pelo esquemático e da posição pela
placa. As rotações seguem a convenção do KiCad: conferir as peças polarizadas no visualizador da
JLCPCB antes de confirmar o pedido (skill m360-kicad-hardware-workflow §6).

Uso: python fabricacao.py <pasta da placa>     p.ex. python fabricacao.py ../m360-lp
"""
import csv
import glob
import json
import os
import re
import subprocess
import sys
import tempfile
import zipfile

KICAD_CLI = os.environ.get("KICAD_CLI", r"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe")
CAMADAS = "F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts"
# prefixos de referência de peça com polaridade ou pino 1: a JLCPCB gira vários encapsulamentos
POLARIZADAS = ("U", "Q", "D", "SW")


def cli(*args):
    r = subprocess.run([KICAD_CLI, *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if r.returncode not in (0, 5):   # 5: o DRC/ERC terminou e achou violação — o JSON diz quais
        sys.exit(f"kicad-cli {' '.join(args[:3])} falhou ({r.returncode}):\n{r.stdout}\n{r.stderr}")
    return r


def conferir(sch, pcb, tmp):
    erc = os.path.join(tmp, "erc.json")
    cli("sch", "erc", "--format", "json", "--severity-all", "-o", erc, sch)
    n_erc = sum(len(s["violations"]) for s in json.load(open(erc, encoding="utf-8"))["sheets"])
    drc = os.path.join(tmp, "drc.json")
    cli("pcb", "drc", "--format", "json", "--severity-all", "--refill-zones", "-o", drc, pcb)
    d = json.load(open(drc, encoding="utf-8"))
    n_drc, n_pend = len(d["violations"]), len(d["unconnected_items"])
    print(f"ERC: {n_erc} violação(ões)   DRC: {n_drc} violação(ões), {n_pend} pendência(s)")
    if n_erc or n_drc or n_pend:
        sys.exit("placa ou esquemático com violação: nada foi gerado")


def main(pasta):
    pasta = os.path.abspath(pasta)
    sch = glob.glob(os.path.join(pasta, "*.kicad_sch"))[0]
    pcb = glob.glob(os.path.join(pasta, "*.kicad_pcb"))[0]
    nome = os.path.splitext(os.path.basename(pcb))[0]
    saida = os.path.join(pasta, "fabricacao")
    os.makedirs(saida, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        conferir(sch, pcb, tmp)

        # --- Gerbers + furação -> zip ---------------------------------------------------------
        ger = os.path.join(tmp, "gerbers")
        os.makedirs(ger)
        cli("pcb", "export", "gerbers", "-o", ger, "-l", CAMADAS, "--no-x2", "--no-netlist",
            "--subtract-soldermask", pcb)
        cli("pcb", "export", "drill", "-o", ger + os.sep, "--format", "excellon", "--excellon-units", "mm",
            "--excellon-separate-th", "--generate-map", "--map-format", "gerberx2", pcb)
        arquivos = sorted(os.listdir(ger))
        zpath = os.path.join(saida, f"{nome}_gerbers.zip")
        with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
            for a in arquivos:
                z.write(os.path.join(ger, a), a)
        print(f"{os.path.basename(zpath)}: {len(arquivos)} arquivo(s) — {', '.join(arquivos)}")

        # --- BOM ------------------------------------------------------------------------------
        bom_tmp = os.path.join(tmp, "bom.csv")
        cli("sch", "export", "bom", "-o", bom_tmp, "--exclude-dnp",
            "--fields", "Value,Reference,Footprint,LCSC",
            "--labels", "Comment,Designator,Footprint,LCSC Part #",
            "--group-by", "Value,Footprint,LCSC", "--ref-range-delimiter", "", sch)
        linhas = list(csv.DictReader(open(bom_tmp, encoding="utf-8")))
        for l in linhas:
            l["Footprint"] = l["Footprint"].split(":", 1)[-1]   # sem o nome da biblioteca
        bom_path = os.path.join(saida, f"{nome}_bom.csv")
        with open(bom_path, "w", encoding="utf-8", newline="") as f:
            w = csv.DictWriter(f, fieldnames=["Comment", "Designator", "Footprint", "LCSC Part #"])
            w.writeheader()
            w.writerows(linhas)
        refs_bom = {r.strip() for l in linhas for r in l["Designator"].split(",") if r.strip()}
        sem_lcsc = [l["Designator"] for l in linhas if not l["LCSC Part #"].strip()]
        print(f"{os.path.basename(bom_path)}: {len(linhas)} linha(s), {len(refs_bom)} peça(s)")

        # --- Posição --------------------------------------------------------------------------
        pos_tmp = os.path.join(tmp, "pos.csv")
        cli("pcb", "export", "pos", "-o", pos_tmp, "--format", "csv", "--units", "mm", "--side", "both",
            "--exclude-dnp", pcb)
        cpl_path = os.path.join(saida, f"{nome}_cpl.csv")
        refs_pos = set()
        with open(cpl_path, "w", encoding="utf-8", newline="") as f:
            w = csv.writer(f)
            w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
            for l in csv.DictReader(open(pos_tmp, encoding="utf-8")):
                refs_pos.add(l["Ref"])
                w.writerow([l["Ref"], f'{float(l["PosX"]):.4f}mm', f'{float(l["PosY"]):.4f}mm',
                            "Top" if l["Side"] == "top" else "Bottom", f'{float(l["Rot"]):g}'])
        print(f"{os.path.basename(cpl_path)}: {len(refs_pos)} peça(s)")

    # --- Conferência cruzada ----------------------------------------------------------------------
    erros = []
    if refs_bom != refs_pos:
        erros.append(f"BOM sem posição: {sorted(refs_bom - refs_pos)}; posição sem BOM: {sorted(refs_pos - refs_bom)}")
    if sem_lcsc:
        erros.append(f"sem código LCSC: {sem_lcsc}")
    polar = sorted(r for r in refs_pos if re.match(r"(%s)\d" % "|".join(POLARIZADAS), r))
    print("conferir a rotação no visualizador da JLCPCB: " + ", ".join(polar))
    if erros:
        sys.exit("\n".join(erros))
    print("BOM e posição com as mesmas referências; todas com código LCSC")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
