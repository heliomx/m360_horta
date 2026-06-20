A sua skill já está boa, mas tem redundâncias, falta definição clara dos critérios de saída e não separa bem as responsabilidades entre elicitação, análise e geração. Também corrigi alguns pontos para ficarem alinhados com o padrão M360-DRY e MySensors.

---

# Skill: Criar Novo Nó da Rede M360 Horta

## Objetivo

Esta skill orienta a criação completa de um novo nó da rede **M360 Horta**, garantindo conformidade com:

* Arquitetura da biblioteca `M360-DRY`
* Padrões MySensors
* Estrutura do projeto PlatformIO
* Diagramas de documentação
* Configuração automática do `platformio.ini`

O agente deve atuar como:

```text
m360_horta.agent\skills\bmad-agent-dev
```

Sempre que disponível:

* Utilizar MCP MySensors
* Utilizar a biblioteca `lib/M360-DRY`
* Utilizar como referência os nós existentes do projeto

---

# Regras Obrigatórias

### Regra 1 — Macros MySensors

Todas as macros:

```cpp
MY_*
```

devem existir exclusivamente no:

```ini
platformio.ini
```

É proibido declarar:

```cpp
#define MY_NODE_ID
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN
#define MY_RF24_CS_PIN
```

em arquivos `.h` ou `.cpp`.

---

### Regra 2 — Biblioteca Base

Todo nó deve utilizar:

```cpp
#include <M360.h>
```

e seguir o modelo:

```text
lib/M360-DRY/examples/NodeTemplate/*.*
```

---

### Regra 3 — STOP & WAIT

O agente nunca deve gerar código sem antes coletar todos os requisitos do Passo 1.

Após apresentar as perguntas, deve aguardar a resposta do usuário.

---

# Passo 1 — Elicitação de Requisitos (STOP & WAIT)

Solicitar ao usuário:

## 0. Tipo de placa

```text
Arduino Nano
Arduino Pro Mini
Arduino Mega
Arduino Uno
Arduino Nano Every
Arduino Nano 33 IoT
Arduino Nano RP2040 Connect
Arduino Nano ESP32
Arduino Nano ESP32-S3
Arduino Nano ESP32-S3-MINI
Arduino Nano ESP32-S3-EYE
Arduino Nano ESP32-C6
Arduino Nano ESP32-C6-MINI
Arduino Nano ESP32-C6-EYE
```

## 1. Identificação

```text
Nome do Sketch:
MY_NODE_ID:
Descrição:
```

---

## 2. Perfil de Energia

Escolher:

```text
ALWAYS_ON (Default  )
LOW_POWER
PASSIVE
```

### Definições

ALWAYS_ON

```text
Alimentação contínua
(ex.: fonte 12V)
```

LOW_POWER

```text
Bateria
envio periódico
sleep programado
```

PASSIVE

```text
Bateria
acorda somente por evento
```

---

## 3. Rádio

```text
CE:
CSN:
Potência RF:
```

Exemplos:

```text
LOW
MEDIUM
HIGH (Default)
MIN
MAX
```

---

## 4. Timeout de Rádio

```text
Timeout (ms):
```

Padrão:

```text
500
```

---

## 5. Log de RSSI

```text
Habilitar RSSI?
(sim/nao)
```

---

## 6. Gestão de Alimentação dos Sensores

```text
Existe pino VCC controlado?
(sim/nao)
```

Se sim:

```text
Qual pino?
```

Exemplo:

```text
D4
4
A1
```

---

## 7. Sensores e Atuadores

Para cada item solicitar:

```text
Child ID:
Label:
```

Voce deve escolher o Tipo e o Valor MySensors que melhor atenda o sensor/atuador:
Exemplo:
 
```text
S_*
```

Tipo de valor:

```text
V_*
```
Mostre as escolhas para a confirmação do usuário 
Voce deve escolher o Pino:

```text
D2
A0
etc
```
Mostre as escolhas para a confirmação do usuário

Opções:

```text
Multiplicar por 100 ao enviar?
(sim/nao)

wakeOnRadio?
(sim/nao)
```

---

⚠️ Não prosseguir enquanto todas as informações não forem fornecidas.

---

# Passo 2 — Análise dos Artefatos Locais

Após receber os requisitos:

## Verificar documentação existente

Procurar em src\DRY\nos por exemplos  de arquivos:

```text
esquemaEletrico.md
esquema_eletrico.md
```

e

```text
diagrama*.md
diagrama*.svg
```
gerar estes arquivos baseado nos arquivos do novo nó:


---

## Verificar implementação existente

Localizar:

```text
sensorDrivers.h
sensorDrivers.cpp
```

Determinar:

### Cenário A

Já existe implementação.

Resultado:

```text
Atualizar arquivos.
```

### Cenário B

Não existe implementação.

Resultado:

```text
Criar arquivos novos.
```

---

## Validar

Conferir:

* Child IDs duplicados
* Pinos duplicados
* Conflitos com rádio
* Conflitos com Serial
* Conflitos com interrupções

Gerar relatório antes da implementação.

---

# Passo 3 — Geração do Driver

## Arquivo

```text
sensorDrivers.h
```

Criar:

* Constantes
* IDs
* Pinagem
* Protótipos

---

## Arquivo

```text
sensorDrivers.cpp
```

Implementar:

```cpp
void initSensors();
```

e

```cpp
bool readNodeItem(uint8_t nodeIndex);
```

Garantir compatibilidade com:

```cpp
NODE_ITEMS[]
```

da biblioteca M360.

---

# Passo 4 — Geração do Nó

Criar o sketch principal baseado em:

```text
lib/M360-DRY/examples/NodeTemplate/NodeTemplate.cpp
```

---

## Requisitos

### Incluir

```cpp
#include <M360.h>
```

---

### Definir

```cpp
NODE_ITEMS[]
```

conforme os sensores e atuadores informados.

---

### Implementar

Se houver VCC controlado:

```cpp
void M360::powerUp();
void M360::powerDown();
```

---

### Presentation

Garantir:

```cpp
Nome amigável
Versão
Tipo do nó
```

---

# Passo 5 — Atualização do platformio.ini

Adicionar ambiente de compilação.

Exemplo:

```ini
[env:nodeSoil]
platform = atmelavr
board = nanoatmega328
framework = arduino
```

Incluir:

```ini
build_flags =
    -DMY_NODE_ID=XX
    -DMY_RF24_CE_PIN=9
    -DMY_RF24_CS_PIN=10
```

Todas as macros MySensors devem ficar neste arquivo.

---

# Passo 6 — Criação dos Diagramas

## Diagrama de Blocos

Criar:

```text
diagrama_blocos.svg
```

Base:

```text
src/DRY/nos/nodePump/diagrama_blocos.svg
```

Representar:

* MCU
* Rádio
* Sensores
* Atuadores
* Alimentação

---

## Esquema Elétrico

Criar:

```text
esquema_eletrico.md
```

Base:

```text
src/DRY/nos/nodePump/esquema_eletrico.md
```

Documentar:

* Componentes
* Pinagem
* Alimentação
* Child IDs
* Observações

---

# Passo 7 — Validação Final (turbo)

Executar validações:

### Estrutura

* Arquivos criados
* Includes válidos
* Compatibilidade M360

### Hardware

* Pinos duplicados
* Child IDs duplicados
* Consumo energético

### MySensors

* Macros apenas no platformio.ini
* Presentation consistente
* Mensagens compatíveis

### Formatação

Executar:

```bash
astyle --style=1tbs --indent=tab
```

---

# Entregáveis Obrigatórios

Ao final apresentar:

## Arquivos criados

```text
src/<Node>.cpp
src/sensorDrivers.h
src/sensorDrivers.cpp
platformio.ini
diagrama_blocos.svg
esquema_eletrico.md
```

## Resumo

```text
✓ Perfil de energia
✓ Sensores
✓ Atuadores
✓ Child IDs
✓ Pinagem
✓ Configuração RF
✓ Timeout
✓ RSSI
```

## Walkthrough

Explicar:

1. Arquitetura do nó
2. Fluxo de inicialização
3. Fluxo de leitura
4. Fluxo de transmissão
5. Estratégia de energia
6. Integração com M360-DRY


