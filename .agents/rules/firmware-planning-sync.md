# Regra Obrigatória — Sincronismo entre Firmware (`src/DRY/` e `lib/M360-DRY`) e Artefatos de Planejamento (`_bmad-output/planning-artifacts/`)

1. **Paridade Rigorosa entre Código C++ e Especificações BMAD:**
   - Toda e qualquer alteração realizada no firmware dos nós ou gateways em `src/DRY/` ou na biblioteca compartilhada `lib/M360-DRY/` **deve ser refletida imediatamente** nos artefatos de planejamento localizados em `_bmad-output/planning-artifacts/` (`prd.md`, `epics.md`, histórias de usuário individuais).
   - Nenhuma entrega que altere comportamento, novos nós, pinagens, mensagens ou perfis de energia é considerada concluída sem que a documentação técnica e os épicos/histórias correspondentes estejam perfeitamente alinhados.

2. **Fluxo de Trabalho Bidirecional:**
   - **Do Código para o Planejamento:** Ao refatorar ou adicionar lógica em C++, atualizar os requisitos funcionais, critérios de aceitação e casos de uso no PRD e Épicos.
   - **Do Planejamento para o Código:** Ao definir novas histórias ou épicos pelo fluxo BMAD, a implementação em C++ deve seguir estritamente o que foi especificado e acordado.

3. **Gatilhos de Atualização Obrigatória:**
   - Adição, remoção ou reconfiguração de nós em `src/DRY/horta/nos/` ou `src/DRY/kit-helio/nos/`.
   - Modificações nas classes centrais de `lib/M360-DRY/` (`M360Node`, `M360Gateway`, `M360Registry`, etc.).
   - Mudanças nas constantes, child IDs reservados ou tempos de ciclo/sleep.
