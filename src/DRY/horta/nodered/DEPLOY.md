# Deploy do `flows.json` no Node-RED — procedimento único (SSoT)

Vale para qualquer agente (Claude Code, Google Antigravity, Codex) e para
humanos. Servidor: `https://nr.viridiotech.com.br`.

## 1. Autenticação: senha em runtime, nunca token gravado

Tokens do Node-RED expiram (7 dias por padrão). Por isso **não se guarda token
em lugar nenhum**. Usuário e senha ficam em um único arquivo, fora do
repositório, legível só pelo usuário do Windows:

```
%USERPROFILE%\.m360\nodered.env        (C:\Users\<user>\.m360\nodered.env)
```

```
NODE_RED_URL=https://nr.viridiotech.com.br
NODE_RED_USERNAME=...
NODE_RED_PASSWORD=...
```

ACL aplicada na criação (repetir se recriar o arquivo):

```
icacls %USERPROFILE%\.m360\nodered.env /inheritance:r /grant:r "%USERNAME%:(R,W)" /grant:r "SYSTEM:(F)"
```

Regras:

- **Nunca** copiar usuário, senha ou token para o repositório, para
  `~/.claude.json`, para `.gemini/settings.json`, para chat ou para commit.
- Para trocar a senha, edite só esse arquivo. Nada mais precisa mudar.
- Outro caminho pode ser indicado com a variável `NODE_RED_CREDENTIALS`.

Quem consome o arquivo:

| Consumidor | Como |
|---|---|
| Script `tools/nr_deploy.py` | faz `POST /auth/token`, usa o token só durante a execução e o revoga ao sair |
| MCP `node-red` (Claude Code) | wrapper `C:/Users/jmarc/Documents/MCP-servers/node-red-mcp-server/nr-mcp-login.mjs` faz login a cada início do MCP e injeta `NODE_RED_TOKEN` em memória. Em `~/.claude.json` o bloco `mcpServers.node-red` aponta para o wrapper e só tem `NODE_RED_URL` |

## 2. Ferramenta: `tools/nr_deploy.py`

```
python src/DRY/horta/nodered/tools/nr_deploy.py login
python src/DRY/horta/nodered/tools/nr_deploy.py backup
python src/DRY/horta/nodered/tools/nr_deploy.py diff
python src/DRY/horta/nodered/tools/nr_deploy.py pull
python src/DRY/horta/nodered/tools/nr_deploy.py deploy --tab <ID> [<ID> ...]
python src/DRY/horta/nodered/tools/nr_deploy.py deploy --all --yes
```

| Subcomando | O que faz |
|---|---|
| `login` | testa as credenciais |
| `backup` | `GET /flows` para `%USERPROFILE%\.m360\backups\nodered\<data>_flows_server.json` |
| `diff` | compara aba a aba o `flows.json` local com o servidor. Ignora `x`/`y` e o `wires: []` que o runtime descarta de `link out` |
| `pull` | grava o estado do servidor no `flows.json` local (faz backup do local antes) |
| `deploy --tab` | backup do servidor e da aba, depois `PUT /flow/{id}` com os nós daquela aba |
| `deploy --all --yes` | `PUT /flows` com o arquivo inteiro. Substitui **tudo**, inclusive config nodes (broker MQTT, dashboard, Telegram) |

Só depende da biblioteca padrão do Python 3.

## 3. Ordem obrigatória

1. `backup`
2. `diff`
3. Ler o diff. Se aparecer `só no servidor` ou `diferente` em nós que **você
   não alterou**, o servidor está à frente do repositório. Nesse caso:
   `pull`, reaplicar a sua mudança sobre o arquivo atualizado, e só então
   seguir. **Gravar a aba por cima apaga o trabalho que só existe no servidor.**
4. `deploy --tab <ID>` apenas das abas que você alterou.
5. `diff` de novo para confirmar que as abas alvo ficaram idênticas.
6. Commitar o `flows.json` no mesmo estado que subiu.

`deploy --all` só com pedido explícito do dono do projeto.

## 4. Abas e IDs

| Aba | ID |
|---|---|
| MQTT | `6e38860673f8fc3a` |
| Fluxo 2 - Solenóides | `a757ff0be26f944a` |
| Fluxo 2 - Bombas | `1e714cfec06339e2` |
| M360 Horta - ACK Handling | `50fbcdf0053fa19f` |
| Irrigação | `tab_irrigacao_m360` |
| IA Repórter (Diário) | `5f670c411a47a898` |
| Telegram Bot Interativo | `tab_telegram_bot` |

## 5. Cuidados específicos deste projeto

- **Irrigação** (`tab_irrigacao_m360`): abre solenoides de verdade. Só subir
  com o Nó 99 e o gateway já gravados com o failsafe de atuação
  (Nó 99: `nano_99reles` ou `act_99`/`act_99_rep`; gateway: `d1_mini_gateway` ou `d1_mini_gw`).
  Todos esses envs compilam com o failsafe.
- Um redeploy mata os `setTimeout` de desligamento dentro dos function nodes
  dos canteiros. Quem cobre isso é o failsafe do firmware e o reconciliador
  de irrigação que existe no servidor.
- Comandos de atuação em modo nativo levam o payload bruto `"1"`/`"0"`, nunca
  o envelope JSON. Ver `CLAUDE.md`, seção "Armadilhas Críticas".
- Segredos do Telegram são lidos de `env.get("TELEGRAM_BOT_TOKEN")` /
  `global.get(...)`. Não deixar token nem chat ID literal em function node.

## 6. Histórico do alinhamento

Em 2026-09-24 o `diff` mostrou o servidor à frente do repositório em todas
as abas (guardas anti-loop nos despachantes, caixa postal para nós LP,
reconciliador de irrigação, filtros de telemetria, mapa de rede). No mesmo
dia o `flows.json` foi alinhado ao servidor (commit `7ad0a04`) e houve
deploy das abas revisadas (`7ad0a04`, `9c8b768`). O estado atual só se
conhece rodando `diff`: faça-o sempre antes de editar.
