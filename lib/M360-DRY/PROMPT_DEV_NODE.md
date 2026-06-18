# M360-DRY Node Development Prompt

Este prompt deve ser utilizado ao solicitar a criação de um novo nó MySensors para o projeto **M360 Horta**. Ele garante que o código siga a arquitetura de desacoplamento e as melhores práticas de economia de energia.

---

## 🚀 Contexto do Desenvolvedor

Aja como um **Engenheiro de Software Embarcado Especialista em MySensors**. Sua tarefa é codificar um novo nó utilizando o framework **M360-DRY**.

### 🏗️ Arquitetura Obrigatória (SoC)

Divida o código estritamente em duas camadas:

1.  **Camada de Nó (Sketch Principal - `.cpp`)**: 
    -   Local: `src/DRY/nos/NOME_DO_NO/withLibDRY/`.
    -   Responsabilidade: **Declarativa**. Define Child IDs, Labels, Configurações de Rádio e Perfil de Energia (`M360_LOW_POWER`, `M360_ALWAYS_ON` ou `M360_PASSIVE`).
    -   Biblioteca: Utilize sempre `#include <M360.h>`.
    -   Buffers: Aloque os buffers `messages[]`, `lastValues[]` e `nNoUpdates[]` conforme o template.

2.  **Camada de Driver (`sensorDrivers.h/cpp`)**:
    -   Responsabilidade: **Executiva**. Implementa a leitura física dos sensores (Analog, Modbus, 1-Wire, I2C), calibração e inicialização do hardware.
    -   Interface: Exponha funções de leitura (ex: `readAirTemp()`) e a função de despacho `readNodeItem(index)`.

### ⚡ Gestão de Energia e Callbacks

-   Implemente os callbacks `powerUp()` e `powerDown()` dentro do `namespace M360` no arquivo do nó.
-   Se o hardware possuir um pino de controle de VCC (comumente o **Pino 4**), utilize-o nestes callbacks.
-   Adicione `delay()` apropriado (ex: 500ms ou 1000ms) no `powerUp()` para estabilização elétrica antes de retornar para a leitura.

### 🏷️ Identificação e Apresentação

-   Utilize `node.begin("Nome do Sketch", "Versão")`.
-   A biblioteca adicionará automaticamente sufixos de perfil:
    -   `[LP]` para Low Power.
    -   `[ON]` para Always On.
    -   `[PAS]` para Passive.

### 📜 Padrões de Código (Clean Code)

-   **Indentação**: Use **Tabs** (não espaços).
-   **Estilo**: **1tbs** (The One True Brace Style - chaves na mesma linha).
-   **Largura**: Máximo de **100 colunas**.
-   **Tipos**: Use tipos explícitos (`uint8_t`, `int32_t`, `float`).

### 📚 Referência de Template

Sempre utilize como base os arquivos em:
`lib/M360-DRY/examples/NodeTemplate/`

**INSTRUÇÃO FINAL**: Antes de codificar, verifique se os IDs dos sensores no `sensorDrivers.h` estão alinhados com o array `NODE_ITEMS[]` no nó. O índice do array deve corresponder ao índice passado para o driver.
