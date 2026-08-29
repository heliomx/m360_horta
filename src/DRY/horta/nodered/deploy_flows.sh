#!/usr/bin/env bash
#
# deploy_flows.sh — publica abas de flows.json no Node-RED de produção.
#
# ⚠️ ESTE É O CAMINHO DE EXCEÇÃO.
#
# A regra do CLAUDE.md é: alterar no servidor via MCP, depois espelhar em
# flows.json. Este script faz o inverso — empurra o arquivo para o servidor.
# Use-o só quando a alteração foi obrigatoriamente autorada fora do editor
# (revisão em lote, correção de muitos nós, payload grande demais para passar
# como parâmetro de ferramenta MCP). Depois de aplicar, flows.json e produção
# voltam a coincidir, que é o estado normal.
#
# Uso:
#   bash deploy_flows.sh                          # DRY RUN: mostra o que mudaria
#   bash deploy_flows.sh --apply <tabId> [...]    # publica as abas indicadas
#   bash deploy_flows.sh --apply --all-changed    # publica toda aba divergente
#
# Variáveis de ambiente (opcionais):
#   NODE_RED_URL    default https://nr.viridiotech.com.br
#   NODE_RED_TOKEN  se ausente, é lido de ~/.claude.json (bloco do MCP node-red)
#
# Segurança:
#   - Sem --apply, o script NUNCA escreve: só faz GET e imprime o diff.
#   - Com --apply, grava antes um backup do GET /flows em backups/.
#   - Aborta no primeiro HTTP != 200; não continua com deploy pela metade.
#   - O token nunca é ecoado.
#
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FLOWS="$HERE/flows.json"
BACKUPS="$HERE/backups"
HOST="${NODE_RED_URL:-https://nr.viridiotech.com.br}"

# A aba ACK Handling define os link out que as outras abas consomem: publicada
# fora de ordem, deixa referência pendente por alguns segundos.
ORDEM_PREFERIDA="50fbcdf0053fa19f"

[ -f "$FLOWS" ] || { echo "ERRO: não encontrei $FLOWS"; exit 1; }

TOK="${NODE_RED_TOKEN:-}"
if [ -z "$TOK" ]; then
  TOK=$(python -c "
import json,os,sys
try:
    cfg=json.load(open(os.path.expanduser('~/.claude.json'),encoding='utf-8'))
    sys.stdout.write(cfg['mcpServers']['node-red']['env']['NODE_RED_TOKEN'])
except Exception:
    pass
" 2>/dev/null)
fi
[ -n "$TOK" ] || { echo "ERRO: defina NODE_RED_TOKEN ou configure o MCP node-red em ~/.claude.json"; exit 1; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# ---------------------------------------------------------------- estado atual
echo "Lendo produção em $HOST ..."
# Retenta: logo depois de um deploy o runtime reinicia e o proxy devolve 404,
# ou a conexão cai (000). Só 401 é definitivo — token, não indisponibilidade.
code=000
for _ in $(seq 1 25); do
  code=$(curl -s -o "$TMP/prod.json" -w '%{http_code}' --max-time 30 \
          -H "Authorization: Bearer $TOK" "$HOST/flows" || echo 000)
  [ "$code" = "200" ] && break
  [ "$code" = "401" ] && break
  sleep 6
done
if [ "$code" != "200" ]; then
  echo "ERRO: GET /flows devolveu HTTP $code"
  [ "$code" = "401" ] && echo "  → token inválido ou processo MCP com valor obsoleto (ver CLAUDE.md §4)"
  [ "$code" = "404" ] && echo "  → 404 vem do proxy: o Node-RED ainda está reiniciando. Tente de novo em ~1 min."
  exit 1
fi

# ------------------------------------------------------------------ comparação
python - "$TMP/prod.json" "$FLOWS" "$TMP" <<'PY'
import json,sys,os
prod_p, loc_p, tmp = sys.argv[1], sys.argv[2], sys.argv[3]
prod = json.load(open(prod_p, encoding='utf-8'))
loc  = json.load(open(loc_p,  encoding='utf-8'))
idx  = lambda a: {n['id']: n for n in a}
p, l = idx(prod), idx(loc)
tabs = {n['id']: n.get('label') for n in loc if n.get('type') == 'tab'}
tabs.update({n['id']: n.get('label') for n in prod if n.get('type') == 'tab'})

def zof(n): return n.get('z') or (n['id'] if n.get('type') == 'tab' else '(config)')

novos    = [i for i in l if i not in p]
removidos= [i for i in p if i not in l]
alterados= [i for i in p if i in l and
            json.dumps(p[i], sort_keys=True) != json.dumps(l[i], sort_keys=True)]

afetadas = set()
for i in novos + alterados:  afetadas.add(zof(l[i]))
for i in removidos:          afetadas.add(zof(p[i]))
afetadas = {t for t in afetadas if t in tabs}

for titulo, ids, fonte in (('NOVOS', novos, l), ('REMOVIDOS', removidos, p), ('ALTERADOS', alterados, l)):
    if not ids: continue
    print('\n%s (%d):' % (titulo, len(ids)))
    for i in ids:
        n = fonte[i]
        print('   %-28s %-14s %s' % (tabs.get(zof(n), '(config)')[:28], n.get('type'), n.get('name') or i))

cfg = [i for i in (novos + removidos + alterados)
       if zof(l.get(i, p.get(i))) not in tabs]
if cfg:
    print('\n⚠️  %d nó(s) de configuração divergem. Este script publica POR ABA e' % len(cfg))
    print('    NÃO os toca — nós de config precisam de PUT /flows completo,')
    print('    que arrisca as credenciais do broker e do bot. Trate à mão.')

# newline='' é obrigatório: no Windows o modo texto traduz \n para \r\n e o id
# chegaria ao shell como "50fbcdf0053fa19f\r", que não casa com nada.
open(os.path.join(tmp, 'tabs.txt'), 'w', encoding='utf-8', newline='').write('\n'.join(sorted(afetadas)))
print('\nAbas divergentes: %s' % (', '.join('%s (%s)' % (tabs[t], t) for t in sorted(afetadas)) or 'nenhuma'))
PY
[ $? -eq 0 ] || exit 1

# tr -d '\r' é cinto e suspensório contra CRLF vindo de qualquer python/SO.
CHANGED=$(tr -d '\r' < "$TMP/tabs.txt" 2>/dev/null || true)

# --------------------------------------------------------------------- dry run
if [ "${1:-}" != "--apply" ]; then
  echo
  echo "DRY RUN — nada foi escrito."
  echo "Para publicar:  bash ${BASH_SOURCE[0]} --apply --all-changed"
  exit 0
fi
shift

if [ "${1:-}" = "--all-changed" ]; then
  ALVOS="$CHANGED"
else
  ALVOS="$*"
fi
[ -n "${ALVOS// /}" ] || { echo "Nada a publicar."; exit 0; }

# ACK Handling primeiro, se estiver na lista
ORDENADOS=""
for t in $ALVOS; do [ "$t" = "$ORDEM_PREFERIDA" ] && ORDENADOS="$t"; done
for t in $ALVOS; do [ "$t" != "$ORDEM_PREFERIDA" ] && ORDENADOS="$ORDENADOS $t"; done

# ---------------------------------------------------------------------- backup
mkdir -p "$BACKUPS"
STAMP=$(date +%Y%m%d-%H%M%S)
cp "$TMP/prod.json" "$BACKUPS/flows-producao-$STAMP.json"
echo
echo "Backup da produção: backups/flows-producao-$STAMP.json"

# ---------------------------------------------------------------------- deploy
RESTANTES=$(echo $ORDENADOS | wc -w)
for TID in $ORDENADOS; do
  python - "$FLOWS" "$TID" "$TMP/put.json" <<'PY'
import json,sys
flows, tid, out = sys.argv[1], sys.argv[2], sys.argv[3]
d = json.load(open(flows, encoding='utf-8'))
tab = next((n for n in d if n['id'] == tid and n.get('type') == 'tab'), None)
if tab is None:
    sys.stderr.write('aba %s nao existe em flows.json\n' % tid); sys.exit(1)
obj = {'id': tid, 'label': tab.get('label'), 'disabled': tab.get('disabled', False),
       'info': tab.get('info', ''), 'env': tab.get('env', []),
       'nodes': [n for n in d if n.get('z') == tid]}
open(out, 'w', encoding='utf-8').write(json.dumps(obj, ensure_ascii=False))
sys.stderr.write('%s | %s | %d nos\n' % (tid, obj['label'], len(obj['nodes'])))
PY
  [ $? -eq 0 ] || { echo ">>> ABORTADO ao montar payload de $TID"; exit 1; }

  printf '  publicando ... '
  : > "$TMP/resp.txt"
  code=$(curl -s -o "$TMP/resp.txt" -w '%{http_code}' -X PUT --max-time 120 \
    -H "Authorization: Bearer $TOK" \
    -H "Content-Type: application/json" \
    -H "Node-RED-API-Version: v2" \
    -H "Node-RED-Deployment-Type: nodes" \
    --data-binary @"$TMP/put.json" "$HOST/flow/$TID")
  echo "HTTP $code  $(head -c 100 "$TMP/resp.txt")"
  if [ "$code" != "200" ]; then
    echo ">>> ABORTADO em $TID. Produção pode estar parcialmente atualizada;"
    echo "    o estado anterior está em backups/flows-producao-$STAMP.json"
    echo "    Rode o dry run para ver o que ainda diverge e reaplique."
    exit 1
  fi

  RESTANTES=$((RESTANTES - 1))
  [ "$RESTANTES" -eq 0 ] && break   # nada depois da última aba para esperar

  # O runtime reinicia os nós alterados e o proxy fica sem rota nesse intervalo:
  # curl devolve 000 (conexão caiu) ou 404. Dormir um tempo fixo não basta —
  # medido, a aba ACK (63 nós) levou mais de 90 s para voltar. Sem esta espera a
  # aba seguinte falha sem culpa nenhuma do payload.
  printf '  aguardando runtime '
  pronto=0
  for _ in $(seq 1 60); do
    sleep 4
    c=$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 \
          -H "Authorization: Bearer $TOK" "$HOST/settings" || echo 000)
    if [ "$c" = "200" ]; then pronto=1; echo " ok"; break; fi
    printf '.'
  done
  [ "$pronto" = "1" ] || { echo; echo ">>> Servidor não voltou em 4 min. Aborto."; exit 1; }
done

echo
echo "Deploy concluído. O Node-RED reinicia os nós alterados e pode levar ~30 s"
echo "para voltar a responder — um 404 do proxy nesse intervalo é normal."
echo "Confira com: bash ${BASH_SOURCE[0]}   (deve dizer 'Abas divergentes: nenhuma')"
