# Ecossistema de Nós M360 Horta

Este diretório contém a implementação dos nós sensores e atuadores da rede **M360 Horta**, todos padronizados através da biblioteca core **[LibDRY](file:///d:/Meu%20Drive/Meus%20Documentos/Projetos/PlatformIO/Projects/m360_horta/lib/M360-DRY/)**.

## 🛠️ Como Criar um Novo Nó

Para garantir a consistência de rádio, bateria e padrões de código, utilize sempre a ferramenta de automação interativa via **Prompt do BMad**:

> [!TIP]
> **Slash Command:** `/m360-node-factory`
>
> Este comando iniciará um questionário interativo perguntando:
> 1. `NodeID` e Nome do Sketch.
> 2. Perfil de Energia (`ON`, `LP`, `PAS`).
> 3. Controle de VCC (Pino 4) e Lista de Canais (Sensores/Atuadores).
>
> O assistente gerará automaticamente o driver (`sensorDrivers.h/cpp`) e o nó (`node.cpp`) baseados no **[NodeTemplate](file:///d:/Meu%20Drive/Meus%20Documentos/Projetos/PlatformIO/Projects/m360_horta/lib/M360-DRY/examples/NodeTemplate/)**.

---

## ⚡ Perfis de Energia (M360PowerProfile)

A biblioteca **LibDRY** suporta três modos de operação distintos, cada um otimizado para uma fonte de energia específica:

| Perfil | Sigla | Nome | Comportamento | Uso Ideal |
| :--- | :--- | :--- | :--- | :--- |
| **`ALWAYS_ON`** | **`[ON]`** | Sempre Ativo | Rádio em escuta constante. Não dorme. | Nós em fonte fixa (12V) que precisam de resposta imediata (< 10ms). |
| **`LOW_POWER`** | **`[LP]`** | Baixa Energia | **Proativo**. Dorme, acorda sozinho periodicamente, lê e envia dados. | Sensores de bateria comuns (Temperatura, Umidade de Solo) que geram gráficos automáticos. |
| **`PASSIVE`** | **`[PAS]`** | Modo Passivo | **Reativo**. Dorme sempre. Só lê ou atua quando o Gateway envia um comando. | Atuadores (Bomba) ou Sensores de alto consumo (Modbus) alimentados por bateria. |

---

## 🔋 Análise Técnica de Bateria (Modo PASSIVE)

O modo **`PASSIVE`** utiliza o mecanismo de **Mailbox** (`smartSleep`) do MySensors para permitir comunicação em nós de bateria.

### 1. O Mecanismo de Escuta
- O nó entra em `smartSleep(intervalo)`. O rádio fica em sono profundo.
- Quando o timer vence (ex: a cada 60s), o nó acorda e abre uma **janela de escuta de 500ms** para ouvir o Gateway.
- Se houver comandos (`V_STATUS`, `V_CUSTOM`), o nó os executa. Se não, volta a dormir em 0.5s.

### 2. Custo de Energia por "Check"
Utilizando um rádio **NRF24L01+** em um Arduino Pro Mini de 3.3V:
- **Corrente em Escuta:** ~18 mA.
- **Duração da Janela:** 0.5 segundos.
- **Custo Energético:** `18mA * 0.5s = 9 mAs` ≈ **0.0025 mAh por acordada.**

### 3. Latência vs. Autonomia (Exemplo Bateria 2000mAh)

| Intervalo de Check | Latência Máxima | Estimativa de Vida |
| :--- | :--- | :--- |
| **10 segundos** | Até 10s para ligar | **~3 meses** |
| **1 minuto** | Até 60s para ligar | **~1.5 ano** |
| **10 minutos** | Até 10 min p/ ligar | **~10 anos+** |

---

## 📁 Estrutura de Pastas de cada Nó
Seguindo as melhores práticas da **LibDRY**, cada nó deve estar organizado assim:
```text
nomeDoNo/
├── esquemaEletrico.md       # Opcional: Diagrama de conexões
├── sensorDrivers.h/cpp      # Camada de Driver (Lógica física)
└── withLibDRY/
    └── nomeDoNo.cpp        # Camada de Nó (Configuração e MySensors)
```
