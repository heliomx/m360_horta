"""Remove do .kicad_pcb o bloco de um footprint, pela referência.

Serve para forçar a sincronização com o esquemático a recriá-lo a partir da biblioteca
(útil quando o footprint mudou na biblioteca). Trabalha no texto: não usa o pcbnew, porque
remover e adicionar footprint pelo SWIG provoca falha de segmentação.

Uso: python del_fp.py <placa.kicad_pcb> <REF> [REF...]
"""
import re
import sys

from schlib import block_end


def main(path, refs):
    texto = open(path, encoding="utf-8").read()
    for ref in refs:
        achou = False
        for m in re.finditer(r"\n\t\(footprint ", texto):
            ini = m.start() + 1
            fim = block_end(texto, ini)
            bloco = texto[ini:fim]
            if re.search(r'\(property "Reference" "' + re.escape(ref) + r'"', bloco):
                texto = texto[:m.start()] + texto[fim:]
                print(f"{ref}: bloco removido ({fim - ini} bytes)")
                achou = True
                break
        if not achou:
            print(f"{ref}: não encontrado")
    open(path, "w", encoding="utf-8", newline="\n").write(texto)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2:])
