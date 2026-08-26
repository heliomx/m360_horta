# Regra Obrigatória — Gestão de Fluxos Node-RED e Backup

1. **Servidor em Produção é a Fonte Primária (SSoT Ativa):**
   - Qualquer alteração em fluxos, nós, configurações ou lógica do Node-RED **deve ser executada diretamente no servidor via ferramentas MCP Node-RED** (`update-flow`, `update-flows`, `create-flow`, etc.).
   - **NUNCA** editar manualmente o arquivo `src/DRY/horta/nodered/flows.json` como fonte de desenvolvimento ou aplicação primária.

2. **Arquivo `flows.json` é Estritamente Cópia de Backup Versionada:**
   - O arquivo `src/DRY/horta/nodered/flows.json` serve exclusivamente como backup versionado no repositório Git.
   - **Após qualquer alteração realizada no servidor via MCP**, o arquivo `flows.json` deve ser imediatamente atualizado para refletir o estado exato de produção no Git.

3. **Renovação Automática de Autenticação (HTTP 401 Unauthorized):**
   - Sempre que ocorrer o erro `Node-RED API error: Unauthorized (HTTP 401)` em chamadas MCP, executar imediatamente as etapas de obtenção de novo token via credenciais de `src/DRY/horta/nodered/.env` (linha 1 = usuário, linha 2 = senha) no endpoint `https://nr.viridiotech.com.br/auth/token`, atualizar a variável `NODE_RED_TOKEN` em `C:\Users\jmarc\.gemini\config\mcp_config.json` e prosseguir com a operação.
