"""Tira do BOM e do arquivo de posição as peças que são soldadas à mão depois da PCBA.

Marca as DUAS pontas, pela referência:
  - no esquemático, o símbolo colocado: (in_bom no) e (in_pos_files no);
  - na placa, o footprint: exclude_from_bom e exclude_from_pos_files no (attr ...).

Marcar só um lado não basta: o BOM sai do esquemático, o arquivo de posição sai da placa, e a
paridade entre os dois passaria a acusar a divergência. As definições de biblioteca dentro de
(lib_symbols) não são tocadas — só os símbolos colocados.

A propriedade LCSC continua no símbolo: ela serve para a compra (ARQUITETURA §8 e §9.1).

Idempotente. Trabalha no texto dos arquivos, sem o pcbnew.
Uso: python solda_manual.py <pasta da placa> <REF> [REF...]
     p.ex. python solda_manual.py ../m360-lp SW1 J1 J2 A1 J4
"""
import glob
import os
import re
import sys

from schlib import block_end


def um_arquivo(pasta, ext):
    achados = glob.glob(os.path.join(pasta, "*" + ext))
    if len(achados) != 1:
        sys.exit(f"esperado exatamente um *{ext} em {pasta}, achei {len(achados)}")
    return achados[0]


def esquematico(path, refs):
    texto = open(path, encoding="utf-8", newline="").read()
    fim_lib = block_end(texto, texto.index("(lib_symbols"))
    feitos = set()
    saida, pos = [texto[:fim_lib]], fim_lib
    for m in re.finditer(r"\n\t\(symbol\r?\n", texto[fim_lib:]):
        ini = fim_lib + m.start() + 1
        fim = block_end(texto, ini)
        bloco = texto[ini:fim]
        ref = re.search(r'\(property "Reference" "([^"]+)"', bloco).group(1)
        if ref in refs:
            novo, n_b = re.subn(r"(\n\t\t\(in_bom )(?:yes|no)\)", r"\1no)", bloco, count=1)
            novo, n_p = re.subn(r"(\n\t\t\(in_pos_files )(?:yes|no)\)", r"\1no)", novo, count=1)
            if n_b != 1 or n_p != 1:
                sys.exit(f"{path}: {ref} sem os campos in_bom/in_pos_files no nível do símbolo")
            saida.append(texto[pos:ini])
            saida.append(novo)
            pos = fim
            feitos.add(ref)
    saida.append(texto[pos:])
    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    return feitos


def placa(path, refs):
    texto = open(path, encoding="utf-8", newline="").read()
    feitos = set()
    saida, pos = [], 0
    for m in re.finditer(r"\n\t\(footprint ", texto):
        ini = m.start() + 1
        fim = block_end(texto, ini)
        bloco = texto[ini:fim]
        ref = re.search(r'\(property "Reference" "([^"]+)"', bloco).group(1)
        if ref not in refs:
            continue
        attr = re.search(r"\n\t\t\(attr([^)]*)\)", bloco)
        if not attr:
            sys.exit(f"{path}: {ref} sem (attr ...) — revisar à mão")
        tokens = attr.group(1).split()
        for t in ("exclude_from_pos_files", "exclude_from_bom"):
            if t not in tokens:
                tokens.append(t)
        novo = bloco[:attr.start()] + "\n\t\t(attr " + " ".join(tokens) + ")" + bloco[attr.end():]
        saida.append(texto[pos:ini])
        saida.append(novo)
        pos = fim
        feitos.add(ref)
    saida.append(texto[pos:])
    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    return feitos


def main(pasta, refs):
    refs = set(refs)
    sch = um_arquivo(pasta, ".kicad_sch")
    pcb = um_arquivo(pasta, ".kicad_pcb")
    no_sch = esquematico(sch, refs)
    no_pcb = placa(pcb, refs)
    print(f"{os.path.basename(sch)}: {', '.join(sorted(no_sch)) or 'nada'}")
    print(f"{os.path.basename(pcb)}: {', '.join(sorted(no_pcb)) or 'nada'}")
    faltam = (refs - no_sch) | (refs - no_pcb)
    if faltam:
        sys.exit(f"não encontradas nas duas pontas: {', '.join(sorted(faltam))}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2:])
