---
description: Fábrica de Nós M360-DRY (Criação Interativa)
---

Este workflow orienta a criação de um novo nó para a rede **M360 Horta**, garantindo que todos os requisitos de hardware e padrões de software sejam atendidos.


## atuar como o J:\Meu Drive\GDrive Meus Documentos\Projetos (1)\PlatformIO\Projects\m360_horta\.agent\skills\bmad-agent-dev

## utilizar, se disponivel, o mcp mysensors

## utilizar a biblioteca  lib\M360-DRY

##Todas as macros MY_* devem constar somente no platformio.ini

## Passo 1: Elicitação de Requisitos (STOP & WAIT)

O assistente deve **PARAR** e solicitar, uma a uma, ao usuário as seguintes informações:

1.  **Identidade do Nó**: Nome do sketch e `MY_NODE_ID`.
2.  **Perfil de Energia**: `ALWAYS_ON` (12V), `LOW_POWER` (Bateria proativo) ou `PASSIVE` (Bateria reativo)?
3.  **Configuração de Rádio**: Pinos CE/CSN padrão (9/10) ou customizados?
4.  **Gestão de VCC**: Existe um pino para ligar/desligar sensores (ex: Pino 4)?
5.  **Lista de Sensores/Atuadores**:
    - Child ID, Tipo MySensors (S_..., V_...), Label.
    - Pino de sinal (se houver).
    - Opções: Precisa multiplicar por 100 ao enviar (flag 0x01)? Para atuadores, precisa de `wakeOnRadio`?
6.  **Log de RSSI**: Habilitar registro periódico de RSSI? (sim/nao)
7.  **Timeout de Rádio**: Valor (ms) para timeout de operações de rádio (default 500).
8.  **Potência de Transmissão**: Nível ou valor (ex.: LOW, MEDIUM, HIGH ou mW)
9.  **Gestão de VCC**: Existe um pino para ligar/desligar sensores (ex: Pino 4)?
10.  **Lista de Sensores/Atuadores**:
    - Child ID, Tipo MySensors (S_..., V_...), Label.
    - Pino de sinal (se houver).
    - Opções: Precisa multiplicar por 100 ao enviar (flag 0x01)? Para atuadores, precisa de `wakeOnRadio`?
11.  **Gestão de VCC**: Existe um pino para ligar/desligar sensores (ex: Pino 4)?
12.  **Lista de Sensores/Atuadores**:
    - Child ID, Tipo MySensors (S_..., V_...), Label.
    - Pino de sinal (se houver).
    - Opções: Precisa multiplicar por 100 ao enviar (flag 0x01)? Para atuadores, precisa de `wakeOnRadio`?

**⚠️ AGUARDE A RESPOSTA DO USUÁRIO ANTES DE PROSSEGUIR.**

---

## Passo 2: Análise de Artefatos Locais

Após coletar os dados, o assistente deve:
1.  Verificar se existe um `esquemaEletrico.md` ou `diagrama*.md` no diretório de destino para validar pinos e IDs.
2.  Verificar se os arquivos `sensorDrivers.h` e `sensorDrivers.cpp` já possuem a lógica de leitura implementada ou precisam ser criados do zero.

---

## Passo 3: Geração do Código (Driver)

Implementar ou atualizar o driver de hardware:
-   **sensorDrivers.h**: Definir pinagem e IDs conforme Passo 1.
-   **sensorDrivers.cpp**: Implementar `readNodeItem(nodeIndex)` e funções de inicialização `initSensors()`. Garantir que o driver exponha o que o motor LibDRY precisa.

---

## Passo 4: Geração do Código (Nó LibDRY)

Criar o arquivo principal do nó baseando-se no **[NodeTemplate.cpp](file:///d:/Meu%20Drive/Meus%20Documentos/Projetos/PlatformIO/Projects/m360_horta/lib/M360-DRY/examples/NodeTemplate/NodeTemplate.cpp)**:

1.  Utilizar `#include <M360.h>`.
2.  Preencher `NODE_ITEMS[]` com as definições detalhadas no Passo 1.
3.  Implementar `M360::powerUp()` e `M360::powerDown()` se houver controle de VCC.
4.  No `presentation()`, garantir o nome amigável com a versão.

---

## Passo 5: Validação e Formatação // turbo

1.  Executar `astyle` nos arquivos criados para garantir o padrão **Tabs** e **1tbs**.
2.  Realizar uma checagem mental de conflitos de pinos e IDs.
3.  Apresentar um **Walkthrough** com os links dos arquivos gerados.

## Passo 6: Configuração do arquivo platformIO.ini

1. incluir no platformio.ini o ambiente de compilação do arquivo recem criado.

## Passo 7: criação dos diagramas

1. criar arquivo de diagrama de blocos (vide modelo  src\DRY\nos\nodePump\diagrama_blocos.svg)

2. criar arquivo do esquema elétrico (vide modelo  src\DRY\nos\nodePump\esquema_eletrico.md)