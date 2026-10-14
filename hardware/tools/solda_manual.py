"""Tira do BOM e do arquivo de posição as peças que são soldadas à mão depois da PCBA.

Marca as DUAS pontas, pela referência:
  - no esquemático, o símbolo colocado: (in_bom no) e (in_pos_files no);
  - na placa, o footprint: exclude_from_bom e exclude_from_pos_files no (attr ...).

Marcar só um lado não basta: o BOM sai do esquemático, o arquivo de posição sai da placa, e a
paridade entre os dois passaria a acusar a divergência. As definições de biblioteca dentro de
(lib_symbols) não são tocadas — só os símbolos colocados.

A propriedade LCSC continua no símbolo: ela serve para a compra (ARQUITETURA §8 e §9.1).

Opções:
  --desfazer        devolve as peças ao BOM e ao arquivo de posição (in_bom yes, sem exclude_*);
  --so-esquematico  mexe só no .kicad_sch — para quando a placa ainda vai ser refeita e a peça
                    nem existe nela. Rodar de novo sem a opção depois de sincronizar a placa.

Idempotente. Trabalha no texto dos arquivos, sem o pcbnew.
Uso: python solda_manual.py <pasta da placa> [--desfazer] [--so-esquematico] <REF> [REF...]
     p.ex. python solda_manual.py ../m360-lp SW1 J1 J2 A1 J4
"""
import glob
import os
import re
import sys

from schlib import block_end

EXCLUI = ("exclude_from_pos_files", "exclude_from_bom")


def um_arquivo(pasta, ext):
    achados = glob.glob(os.path.join(pasta, "*" + ext))
    if len(achados) != 1:
        sys.exit(f"esperado exatamente um *{ext} em {pasta}, achei {len(achados)}")
    return achados[0]


def esquematico(path, refs, desfazer):
    texto = open(path, encoding="utf-8", newline="").read()
    fim_lib = block_end(texto, texto.index("(lib_symbols"))
    valor = "yes" if desfazer else "no"
    feitos = set()
    saida, pos = [texto[:fim_lib]], fim_lib
    for m in re.finditer(r"\n\t\(symbol\r?\n", texto[fim_lib:]):
        ini = fim_lib + m.start() + 1
        fim = block_end(texto, ini)
        bloco = texto[ini:fim]
        ref = re.search(r'\(property "Reference" "([^"]+)"', bloco).group(1)
        if ref in refs:
            novo, n_b = re.subn(r"(\n\t\t\(in_bom )(?:yes|no)\)", rf"\g<1>{valor})", bloco, count=1)
            novo, n_p = re.subn(r"(\n\t\t\(in_pos_files )(?:yes|no)\)", rf"\g<1>{valor})", novo, count=1)
            if n_b != 1 or n_p != 1:
                sys.exit(f"{path}: {ref} sem os campos in_bom/in_pos_files no nível do símbolo")
            saida.append(texto[pos:ini])
            saida.append(novo)
            pos = fim
            feitos.add(ref)
    saida.append(texto[pos:])
    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    return feitos


def placa(path, refs, desfazer):
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
        tokens = [t for t in attr.group(1).split() if t not in EXCLUI]
        if not desfazer:
            tokens += list(EXCLUI)
        novo = bloco[:attr.start()] + "\n\t\t(attr " + " ".join(tokens) + ")" + bloco[attr.end():]
        saida.append(texto[pos:ini])
        saida.append(novo)
        pos = fim
        feitos.add(ref)
    saida.append(texto[pos:])
    open(path, "w", encoding="utf-8", newline="").write("".join(saida))
    return feitos


def main(pasta, refs, desfazer, so_esquematico):
    refs = set(refs)
    sch = um_arquivo(pasta, ".kicad_sch")
    no_sch = esquematico(sch, refs, desfazer)
    acao = "devolvidas ao BOM" if desfazer else "fora do BOM"
    print(f"{os.path.basename(sch)} ({acao}): {', '.join(sorted(no_sch)) or 'nada'}")
    faltam = refs - no_sch
    if not so_esquematico:
        pcb = um_arquivo(pasta, ".kicad_pcb")
        no_pcb = placa(pcb, refs, desfazer)
        print(f"{os.path.basename(pcb)} ({acao}): {', '.join(sorted(no_pcb)) or 'nada'}")
        faltam |= refs - no_pcb
    if faltam:
        sys.exit(f"não encontradas: {', '.join(sorted(faltam))}")


if __name__ == "__main__":
    opcoes = {"--desfazer", "--so-esquematico"}
    args = [a for a in sys.argv[1:] if a not in opcoes]
    if len(args) < 2:
        sys.exit(__doc__)
    main(args[0], args[1:], "--desfazer" in sys.argv, "--so-esquematico" in sys.argv)
