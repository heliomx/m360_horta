# Ferramentas da família M360

Scripts usados para construir as placas desta pasta. Nenhum é mágico: cada um automatiza um
passo que o KiCad faz à mão, mas que precisaria ser repetido dezenas de vezes sem errar.

Todos são **idempotentes** — rodar duas vezes dá o mesmo resultado — e todos **reescrevem o
arquivo passado por argumento**. Trabalhe com a árvore limpa no git: é o diff que mostra o que
o script fez, e é o `git checkout` que desfaz quando ele faz o que não devia.

## Os dois interpretadores

| Script | Interpretador | Por quê |
|---|---|---|
| `place_*.py`, `silk_*.py`, `pcb_info.py` | o Python do KiCad (`C:\Program Files\KiCad\10.0\bin\python.exe`) | usam o módulo `pcbnew`, que só existe lá |
| `promini_medida.py`, `grade.py`, `crop.py` | um venv qualquer com `opencv-python-headless`, `numpy` e `pymupdf` | dependências de imagem |
| os demais | qualquer Python 3 | mexem no texto do arquivo |

Os scripts de texto não usam o `pcbnew` de propósito: **remover ou adicionar um footprint pelo
SWIG do pcbnew provoca falha de segmentação**, e a manipulação do S-expression é previsível.
`schlib.py` é o analisador mínimo de `.kicad_sch` que os outros importam — rode os scripts de
dentro desta pasta, ou com ela no `PYTHONPATH`.

## Por etapa

### Esquemático
| Script | O que faz |
|---|---|
| `fix_layout.py <sch> [--lp\|--act]` | Põe cada label na ponta de um fio curto e os campos Reference/Value fora do corpo, no padrão que o usuário definiu à mão no R6. As opções `--lp` e `--act` aplicam os casos particulares de cada placa — **não troque uma pela outra**: as referências repetem entre as placas com significados diferentes |
| `add_lcsc.py <sch> [--set REF=CÓDIGO ...]` | Grava o código LCSC de cada peça. Sem `--set` aplica o mapa da LP — **só na LP**, porque as referências repetem entre as placas com outro significado; com `--set`, grava só as referências pedidas, em qualquer placa |
| `schlib.py <sch>` | Sem argumento de escrita: imprime símbolos, pinos, labels e fios — útil para conferir o que o `fix_layout.py` vai encontrar |
| `netdiff.py <antes.net> <depois.net>` | Compara duas netlists e acusa qualquer mudança de conectividade. Sai com código 1 se divergirem |

> ⚠️ O `fix_layout.py` desenha fios. Um fio que encoste no pino de outra net **funde as duas
> nets em silêncio** — foi assim que GND e +5 V viraram uma só na ACT. O script tem a checagem
> `livre()` para isso, mas a garantia é externa: **gere a netlist antes e depois e compare com o
> `netdiff.py`**. A ERC sozinha não acusa: o curto é eletricamente válido.

```
kicad-cli sch export netlist --format kicadxml -o antes.net  placa.kicad_sch
python fix_layout.py placa.kicad_sch
kicad-cli sch export netlist --format kicadxml -o depois.net placa.kicad_sch
python netdiff.py antes.net depois.net
```

O `netdiff.py` recusa netlist vazia de propósito: comparar dois conjuntos vazios "passa" sem
provar nada — foi o que aconteceu quando essa comparação era feita à mão, com uma expressão
regular que não casava com o formato e aprovava em silêncio.

### Biblioteca
| Script | O que faz |
|---|---|
| `gen_fp.py` | Gera `M360-RF_Soquete_2x05` e `Arduino_ProMini_DeeKRobot` em `../lib/M360.pretty` |
| `import_lcsc.py <dir> [--sch]` | Traz footprints e modelos 3D baixados pelo `easyeda2kicad` |
| `fix_rj45_3d.py` | Sobe 6,3 mm o modelo 3D do RJ45, que vem centrado no plano da placa. **Rodar sempre depois do `import_lcsc.py`**, que traz o erro de volta |
| `promini_medida.py`, `grade.py` | Medem por foto os furos livres do Pro Mini (A4/A5, GND/A6/A7). O `grade.py` dá as coordenadas em pixel; o `promini_medida.py` faz a homografia e devolve milímetros |

### Placa
| Script | O que faz |
|---|---|
| `place_lp.py <pcb>` | Posiciona a M360-LP (86 × 62 mm, formato imposto pela caixa) e desenha o contorno |
| `place_act.py <pcb>` | Posiciona a M360-ACT (100 × 80 mm) |
| `place_gw.py <pcb>` | Posiciona a M360-GW (80 × 60 mm). **Atenção ao girar peça em 90°:** o courtyard não fica centrado na origem — ver o comentário no script, que custou quatro violações de DRC |
| `zone_thermals.py <pcb> [haste] [folga]` | Afina o alívio térmico das zonas (padrão 0,35 / 0,3). É o **primeiro** recurso contra `starved_thermal`; só se não bastar vem a ligação sólida abaixo |
| `del_fp.py <pcb> <REF>...` | Apaga o bloco de um footprint, para a sincronização com o esquemático recriá-lo da biblioteca. É o caminho para trocar um footprint sem o segfault do SWIG |
| `clear_tracks.py <pcb>` | Apaga **todas** as trilhas e vias, para rerrotear do zero. Necessário sempre que uma peça muda de footprint ou de posição depois de roteada: as trilhas velhas cruzam as ilhas novas e o DRC enche de `clearance`, `hole_clearance` e até `shorting_items` apontando todos para a mesma ilha. As zonas ficam, e são refeitas no refill |
| `dsn_sem_plano.py <in.dsn> <out.dsn>` | Tira as declarações `plane` do DSN antes do Freerouting. **Obrigatório ao rerrotear placa que já tem zona de GND**: com o plano declarado o roteador não desenha fio de terra, o plano se parte em ilhas e o DRC acusa `unconnected_items` "Zona [GND]" que nenhum ajuste de zona resolve (ARQUITETURA §11.6) |
| `gnd_stitch.py <pcb> [--dry]` | Põe via de GND dentro de ilha de plano solta, com verificação de colisão nas duas faces. **Use só para colocar a via, nunca para decidir se ela é necessária** — o critério interno de "já ligada" é falho (§11.6). Se o plano fragmentou, o certo é rerrotear com o GND roteado (`dsn_sem_plano.py`) |
| `pad_zone_connect.py <pcb> <REF> <PAD> <MODO>` | Ligação do pad às zonas: 0 nenhuma, 1 alívio térmico, 2 sólida. Age só no footprint pedido |
| `silk_lp.py`, `silk_act.py`, `silk_gw.py` | Serigrafia de cada placa: tiram textos de referência de cima de ilhas, põem a marca e o nome do modelo, e acrescentam legendas. Na ACT, o campo **Value dos conectores** vai para a serigrafia ("RJ45 RELES-A", "5V do quadro"…), para o instalador não depender de saber o que é J3 ou J5. Na LP, **VCC e GND do J3** — ligar a alimentação invertida queima o nó, então a legenda é proteção, não conforto. Na GW não há texto a mover: os furos M3 já saem sem serigrafia pelo `place_gw.py` |

> Duas armadilhas de serigrafia que custaram rodada extra: **o campo Value herda a rotação do
> footprint** — num conector em rot 90 o texto sai na vertical, atravessando a própria ilha, a
> menos que se force o ângulo; e **texto fora do contorno da placa vira `silk_edge_clearance`**,
> então afastar um rótulo de uma ilha empurrando-o para a borda troca um problema por outro.
> O DRC também não pega tudo: legenda sobre o *corpo* de um conector passa no DRC e fica
> ilegível na placa. Conferir na imagem, não só no relatório.
| `solda_manual.py <pasta> <REF>...` | Tira do BOM e do arquivo de posição as peças soldadas à mão depois da PCBA. Marca **as duas pontas**: `in_bom`/`in_pos_files` no símbolo e `exclude_from_bom`/`exclude_from_pos_files` no footprint — o BOM sai do esquemático e a posição sai da placa, e marcar só um lado faria a paridade divergir. Conferir exportando BOM e posição antes e depois |
| `paridade.py <pasta>` | Confere que a placa liga **exatamente** o que o esquemático liga, comparando o conjunto de pads de cada rede — não o nome. **Use este, não a paridade do DRC**: a sincronização pelo MCP grava nomes de rede diferentes e o DRC acusava 237 / 195 / 69 itens com as placas corretas. Sai com código 1 se divergir |
| `pcb_info.py <pcb>` | Lista footprints com courtyard e posição — conferência do posicionamento |
| `crop.py <pdf> <prefixo>` | Recorta regiões do PDF do esquemático para inspeção visual |

O roteamento em si não tem script: é o Freerouting, chamado na linha de comando.
Ver **ARQUITETURA_FAMILIA.md §11**.

## A ordem que foi usada

```
esquemático (MCP) → fix_layout.py → ERC + netlist comparada → add_lcsc.py
                  → sincroniza com a placa → place_*.py → DRC
                  → Freerouting (§11) → planos de terra → pad_zone_connect.py → silk_*.py

rerroteamento (peça trocada ou movida): del_fp.py → sincroniza → place_*.py → clear_tracks.py
                  → export_dsn → dsn_sem_plano.py → Freerouting → import_ses
                  → zone_thermals.py → DRC com refill → pad_zone_connect.py / silk_*.py
```

## O que conferir, sempre

Confirme cada escrita **no arquivo em disco**, não pelo retorno da ferramenta que você chamou.
O MCP do KiCad recusa a gravação automática quando o arquivo mudou por fora — e neste fluxo
ele muda o tempo todo, porque estes scripts e o `kicad-cli` escrevem direto. Contar `(segment`
e `(footprint` no `.kicad_pcb` e rodar o DRC custa segundos e é a única prova que vale.
