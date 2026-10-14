---
trigger: always_on
---

# Node-RED: credenciais e deploy do flows.json

Procedimento completo (SSoT): `src/DRY/horta/nodered/DEPLOY.md`. Ler antes de
tocar em `src/DRY/horta/nodered/flows.json` ou de falar com o servidor
`https://nr.viridiotech.com.br`.

Regras que não se negociam:

1. **Nunca use nem peça token.** A autenticação é por usuário e senha lidos de
   `%USERPROFILE%\.m360\nodered.env`. O login gera um token de curta duração
   só em memória. Não copie credenciais para o repositório, para arquivos de
   configuração de IDE, para o chat ou para commits.
2. **Toda interação com o servidor passa por**
   `python src/DRY/horta/nodered/tools/nr_deploy.py <login|backup|diff|pull|deploy>`.
   Sem `curl` manual com token.
3. **Ordem fixa:** `backup` → `diff` → (se o servidor estiver à frente:
   `pull` e reaplicar a mudança) → `deploy --tab <ID>` só das abas alteradas →
   `diff` de confirmação → commit do `flows.json`.
4. **`deploy --all` é proibido** sem pedido explícito do dono do projeto.
5. **Aba Irrigação** (`tab_irrigacao_m360`) só sobe com Nó 99 e gateway já
   gravados com o failsafe de atuação.
6. Em 2026-09-24 o servidor estava à frente do repositório em todas as abas.
   Faça `pull` antes de editar o `flows.json`.
