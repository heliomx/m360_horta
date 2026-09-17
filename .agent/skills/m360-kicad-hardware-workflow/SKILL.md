---
name: m360-kicad-hardware-workflow
description: Regras obrigatórias para editar, verificar e revisar os esquemáticos e placas KiCad da família M360 (kicad-cli, MCP KiCad, catálogo JLCPCB, evidências)
---

# Fluxo de hardware KiCad — M360 Horta

> [!IMPORTANT]
> **Single Source of Truth (SSoT):** este arquivo é a referência única para qualquer agente que
> toque em `hardware/`. Foi escrito a partir do que cinco rodadas de análise adversarial
> (2026-09-13) mediram, não do que as ferramentas prometem. O detalhe histórico está em
> `hardware/ARQUITETURA_FAMILIA.md` §11–§12 e em `hardware/tools/README.md`, na branch
> `feat/hardware-familia-pcb`.

---

## 1. Quem faz o quê

| Tarefa | Ferramenta | Por quê |
|---|---|---|
| Netlist, ERC, DRC, PDF/SVG, BOM, posição, Gerbers, `upgrade` | **`kicad-cli`** | Determinístico, sem servidor, roda em segundos, nunca falhou |
| Consulta ao catálogo JLCPCB (`get_jlcpcb_part`, `search_jlcpcb_parts`) | **MCP KiCad** | Único acesso ao banco local; por código é confiável |
| Leitura de placa/esquemático e lint (`find_*`, `list_floating_labels`) | MCP KiCad | Só leitura, risco baixo; o servidor pode travar |
| Especctra DSN/SES (roteamento) | MCP KiCad + Freerouting CLI | O `kicad-cli` não exporta nem importa Specctra |
| Posicionamento de placa | `hardware/tools/place_*.py` (Python do KiCad, `pcbnew`) | Mais previsível que os mutadores do MCP |
| Edição de esquemático | Scripts de texto sobre `hardware/tools/schlib.py`, ou a interface do KiCad | Os mutadores do MCP falham em silêncio (ver §3) |

O `kicad-cli` é o **juiz**; o MCP é o **assistente**. Nada que o MCP grave é aceito sem o
`kicad-cli` confirmar.

---

## 2. Verificação obrigatória depois de qualquer edição de esquemático

O ERC **não** acusa fio que funde duas redes: o curto é eletricamente válido. Por isso toda
edição passa por três comparações, além do ERC. Até existir `tools/verify_sch.py` (pendência),
a sequência é manual:

```bash
K="C:/Program Files/KiCad/10.0/bin/kicad-cli.exe"
git show HEAD:hardware/m360-lp/M360-LP.kicad_sch > /tmp/antes.kicad_sch
"$K" sch export netlist --format kicadxml -o antes.net  /tmp/antes.kicad_sch
"$K" sch export netlist --format kicadxml -o depois.net hardware/m360-lp/M360-LP.kicad_sch
python hardware/tools/netdiff.py antes.net depois.net       # só as redes previstas podem mudar
"$K" sch erc --format json --severity-all -o erc.json hardware/m360-lp/M360-LP.kicad_sch
```

| O quê | Como | Pega |
|---|---|---|
| Ligações | `netdiff.py` entre as duas netlists | Fio que funde redes; label que não encosta |
| Campos de cada componente | Valor, footprint, `LCSC`, `in_bom` comparados entre as netlists | Propriedade gravada no componente errado |
| Símbolos embutidos | Bloco `(lib_symbols)` comparado com o do HEAD | Descrição, datasheet ou pino alterado em outro símbolo |
| ERC | JSON com `--severity-all`, **zero** violações | O resto |

Regras que decorrem disso:

- **Só as redes previstas mudam.** Qualquer outra diferença no `netdiff.py` é defeito até prova
  em contrário, mesmo com ERC zero.
- **Netlist vazia não passa.** Comparar dois conjuntos vazios "aprova" sem provar nada.
- **Busca vazia não é prova.** `grep` que volta vazio por padrão errado (tabulação, nome
  qualificado, `(via ` com espaço) já quase virou conclusão três vezes. Conferir no arquivo.
- Isso vira **hook de pre-commit** para `hardware/**/*.kicad_sch` assim que o script existir.

---

## 3. Escrita pelo MCP KiCad: o que é proibido e o que é obrigatório

Defeitos **medidos** (2026-09-13), todos silenciosos para o ERC:

| Ferramenta | Defeito | Regra |
|---|---|---|
| `replace_schematic_component` | Gravou o LCSC de um componente em outro e apagou o datasheet de um terceiro símbolo | **Nunca usar.** Trocar símbolo por texto (pinagem idêntica) ou apagar e recriar |
| `add_schematic_component` | Ignora `angle`: tudo entra em rotação 0 | Ler a rotação real no arquivo **antes** de desenhar fios; fileiras com ≥ 7,62 mm de folga |
| `batch_add_and_connect` | Estoura os 30 s e continua gravando depois do erro | Uma peça por chamada. Se estourar, **não repetir**: esperar o `mtime` parar de mudar e conferir o que entrou |
| Qualquer escrita | Grava fora do formato canônico (espaços em vez de tabulações); os scripts de `tools/` não enxergam | `kicad-cli sch upgrade --force <sch>` depois de cada rodada de edição, e conferir netlist e campos idênticos |
| `export_dsn` | Ignora `boardPath` | Conferir o arquivo gerado |
| `set_design_rules` | Não persiste a largura de trilha | Conferir no `.kicad_pro` |
| Servidor | Pode travar por inteiro, até `get_board_info` estourando 30 s | Etapa de placa numa sessão só; esquemático como trabalho separado |

Depois de qualquer escrita, rodar as ferramentas de **leitura** do MCP como lint, antes da
conferência visual: `find_wires_crossing_symbols`, `find_overlapping_elements`,
`find_orphaned_wires`, `list_floating_labels`. Elas apontam valor sobre corpo e label dentro de
outra peça, que hoje só aparecem no PDF.

`fix_layout.py` **não é idempotente**: só na primeira arrumação de uma folha.

---

## 4. Catálogo JLCPCB

- **Por código, sempre que houver código:** `get_jlcpcb_part` foi confiável em todas as
  consultas. Conferir peça, encapsulamento, tipo (basic / preferred / extended) e estoque.
- **Busca por texto é fraca.** `fuse` e `fuse 1206` voltaram vazios; `resettable fuse` voltou
  quinze peças. Tentar pelo menos dois termos e a categoria antes de concluir "não existe".
- **Registrar a consulta junto da conclusão.** Onde o documento diz "não há PTC basic", anotar
  categoria e termos usados. Sem isso a afirmação não é reproduzível.
- **Não rodar `download_jlcpcb_database`.** O banco local já está populado; recarregar com
  `~/.kicad-mcp/jlcpcb_import_cdfer_v2.py`.
- Peça **preferred** não paga taxa de setup; **extended** paga por tipo de peça no pedido.

---

## 5. Evidências ficam no repositório

- O catálogo devolve a URL do datasheet na JLCPCB; ela baixa com `curl`. O PDF que fundamentou
  uma decisão vai para **`docs/datasheets/`**, não para pasta temporária, com uma linha em
  `docs/datasheets/README.md`: código LCSC, peça, o que foi conferido, página.
- Toda afirmação numérica do `ARQUITETURA_FAMILIA.md` (pinagem, limite, ruptura, fuga) aponta
  para o datasheet e a página.
- Página de PDF girada: renderizar nas duas rotações e usar a que lê normalmente antes de
  interpretar uma vista. Um jack RJ45 lido de cabeça para baixo inverte a lingueta e, com ela,
  o pino 1.

---

## 6. Placa

- `kicad-cli pcb drc --severity-all --format json` com zero violações e zero ligações
  pendentes, no mesmo hook do esquemático.
- Trabalhar com a árvore **limpa** no git: os scripts reescrevem o arquivo, e é o diff que
  mostra o que fizeram; `git checkout` desfaz.
- Trocar peça depois de roteado obriga a rerrotear a placa inteira (§11.5).
- Antes do pedido de PCBA, conferir no visualizador da JLCPCB a rotação das peças
  polarizadas (SS14, SMAJ5.0A, BAT54WS, ME6211, 2N7002, CIs): o arquivo de posição do
  `kicad-cli` usa a convenção do KiCad, e o zero da JLCPCB é outro para vários encapsulamentos.
- Peças de solda manual ficam fora do BOM e da posição (`tools/solda_manual.py`), com a
  propriedade `LCSC` mantida para a compra.

---

## 7. Protocolo de revisão adversarial

Toda rodada começa e termina com evidência gerada pelo `kicad-cli`, nunca com a memória da
rodada anterior:

1. `git log -1` e árvore limpa; netlist e ERC regenerados do commit revisado.
2. `diff` das netlists contra a rodada anterior: só o que o commit declara pode ter mudado.
3. Placas marcadas como **defasadas de propósito** não são comparadas com o esquemático.
4. A lista "já conhecido" vive em `ARQUITETURA_FAMILIA.md` §9; a rodada a recebe no briefing e
   não repete o que está lá.
5. Cada achado traz: placa, referência e pino, rede, consequência, prova (linha da netlist,
   datasheet e página), gravidade e confiança (confirmado / plausível).
6. O que foi verificado e está correto também é listado, para a rodada seguinte não refazer.
7. Nenhum arquivo do repositório é alterado pela revisão; saídas ficam no scratchpad.
