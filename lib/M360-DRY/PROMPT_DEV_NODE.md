# M360-DRY Node Development Prompt

Este prompt deve ser utilizado ao solicitar a criação de um novo nó MySensors para o projeto **M360 Horta**. Ele garante que o código siga a arquitetura de desacoplamento e as melhores práticas de economia de energia.

---

## 🚀 Contexto do Desenvolvedor

Aja como um **Engenheiro de Software Embarcado Especialista em MySensors**. Sua tarefa é codificar um novo nó utilizando o framework **M360-DRY**.

### 🏗️ Arquitetura Obrigatória (SoC)

Divida o código estritamente em duas camadas:

1.  **Camada de Nó (Sketch Principal - `.cpp`)**: 
    -   Local: `src/DRY/horta/nos/NOME_DO_NO/` (sketch direto na pasta; `withLibDRY/` não existe mais). Nó na placa M360-LP não tem `.cpp` próprio: é um `nodeConfig.h` em `src/DRY/horta/nos/lp/config/` — ver o workflow `.agent/workflows/m360-node-factory.md`, Passo 0.
    -   Responsabilidade: **Declarativa**. Define Child IDs, Labels, Configurações de Rádio e Perfil de Energia (`M360_LOW_POWER`, `M360_ALWAYS_ON`, `M360_PASSIVE` ou `M360_REPEATER`).
    -   Biblioteca: Utilize sempre `#include <M360.h>`.
    -   Buffers: Aloque os buffers `lastValues[]` e `nNoUpdates[]` conforme o template. **Não** declare `messages[]`: desde a 2.0.0 a lib usa um único `MyMessage` interno.
    -   Rótulos: declare cada label como array `PROGMEM` e passe-o com `M360_LABEL()`. Um literal `"..."` direto no `NODE_ITEMS[]` não compila.

2.  **Camada de Driver (`sensorDrivers.h/cpp`)**:
    -   Responsabilidade: **Executiva**. Implementa a leitura física dos sensores (Analog, Modbus, 1-Wire, I2C), calibração e inicialização do hardware.
    -   Interface: Exponha funções de leitura (ex: `readAirTemp()`) e a função de despacho `readNodeItem(index)`.

### ⚡ Gestão de Energia e Callbacks

-   Implemente os callbacks `powerUp()` e `powerDown()` dentro do `namespace M360` no arquivo do nó.
-   Se o hardware possuir um pino de controle de VCC (D3 no template e no Nó 4; D4, ativo em LOW, na M360-LP), utilize-o nestes callbacks.
-   Espere a estabilização elétrica no `powerUp()` com `wait()` (ex: 20 ms para sondas resistivas, 1500 ms para DHT11/DS18B20), nunca com `delay()`: `wait()` continua processando o rádio.

### 🏷️ Identificação e Apresentação

-   Utilize `node.begin(F("Nome do Sketch"), F("Versão"))` — nome e versão ficam na Flash; sem `F()` não compila.
-   A biblioteca adicionará automaticamente sufixos de perfil:
    -   `[LP]` para Low Power.
    -   `[ON]` para Always On.
    -   `[PAS]` para Passive.
    -   `[REP]` para Repeater.

### 📜 Padrões de Código (Clean Code)

-   **Indentação**: Use **Tabs** (não espaços).
-   **Estilo**: **1tbs** (The One True Brace Style - chaves na mesma linha).
-   **Largura**: Máximo de **100 colunas**.
-   **Tipos**: Use tipos explícitos (`uint8_t`, `int32_t`, `float`).

### 📚 Referência de Template

Sempre utilize como base os arquivos em:
`lib/M360-DRY/examples/NodeTemplate/`

**INSTRUÇÃO FINAL**: Antes de codificar, verifique se os IDs dos sensores no `sensorDrivers.h` estão alinhados com o array `NODE_ITEMS[]` no nó. O índice do array deve corresponder ao índice passado para o driver.
