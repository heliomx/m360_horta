# Regra Obrigatória — Gestão de Fluxos Node-RED e Backup

1. **Servidor em Produção é a Fonte Primária (SSoT Ativa):**
   - Qualquer alteração em fluxos, nós, configurações ou lógica do Node-RED **deve ser executada diretamente no servidor via ferramentas MCP Node-RED** (`update-flow`, `update-flows`, `create-flow`, etc.).
   - **NUNCA** editar manualmente o arquivo `src/DRY/horta/nodered/flows.json` como fonte de desenvolvimento ou aplicação primária.

2. **Arquivo `flows.json` é Estritamente Cópia de Backup Versionada:**
   - O arquivo `src/DRY/horta/nodered/flows.json` serve exclusivamente como backup versionado no repositório Git.
   - **Após qualquer alteração realizada no servidor via MCP**, o arquivo `flows.json` deve ser imediatamente atualizado para refletir o estado exato de produção no Git.

3. **Documentação Funcional Obrigatória — `funcionalidades_nodered.md`:**
   - **Toda** alteração no Node-RED exige atualizar `src/DRY/horta/nodered/funcionalidades_nodered.md` **na mesma entrega e no mesmo commit** que atualiza o `flows.json`.
   - A entrega só está completa com os três passos: alterar no servidor via MCP → atualizar `flows.json` → atualizar `funcionalidades_nodered.md`.
   - A §11 desse arquivo lista os gatilhos (qual mudança exige rever qual seção) e a verificação final. Consultá-la antes de dar a tarefa por concluída.
   - **Por que isso não é burocracia:** as regras de irrigação, os critérios de matching de ACK e os limiares do watchdog são invisíveis no JSON — só existem em forma legível nesse arquivo. Defasado, ele faz a próxima sessão de debug partir de premissa errada.
   - A §9 desse arquivo lista os **contratos com o firmware**. Mudar um lado sem o outro quebra o sistema em silêncio, sem erro de compilação nem de log.

4. **Autenticação Node-RED (HTTP 401 Unauthorized):**
   - As credenciais vivem em `src/DRY/horta/nodered/.env`, um arquivo **`chave=valor`** com `NODERED_USER`, `NODERED_PASS`, `TELEGRAM_BOT_TOKEN` e `TELEGRAM_CHAT_ID` — **não** é "linha 1 = usuário, linha 2 = senha". Gitignored; nunca versionar nem ecoar o conteúdo.
   - **Cada CLI lê um config diferente.** Claude Code: `~/.claude.json` → `mcpServers/node-red/env/NODE_RED_TOKEN`. Gemini CLI: `~/.gemini/config/mcp_config.json` → mesma chave.
   - **Diagnosticar antes de renovar.** Testar o token vigente direto na Admin API: `GET https://nr.viridiotech.com.br/flows` com `Authorization: Bearer <token>`. Se responder 200, o token é válido e o 401 vem do **processo MCP com valor obsoleto em memória** — editar o config não resolve sozinho, é preciso reiniciar a conexão MCP, e isso é ação do usuário.
   - Renovação (só quando o token estiver de fato inválido): `POST https://nr.viridiotech.com.br/auth/token`, form-urlencoded, `client_id=node-red-admin&grant_type=password&scope=*&username=…&password=…`.
   - ⚠️ Em 26/08/2026 esse POST devolveu `403 invalid_grant` com as credenciais do `.env` — elas estão defasadas em relação ao servidor. Diante de 403, **parar e avisar o usuário**; não retentar em laço (risco de lockout).
   - Com o MCP bloqueado e a alteração urgente, a Admin API (`POST /flows` com header `Node-RED-Deployment-Type: nodes`) é o mesmo endpoint que o MCP encapsula e cumpre a regra de "alterar produção, não o arquivo" — mas **confirmar com o usuário antes**, por ser escrita em produção. Fazer backup do `GET /flows` antes e verificar, depois do deploy, que apenas os nós pretendidos mudaram.
