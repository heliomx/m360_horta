# Práticas de Campo — M360 Horta

> Conhecimento elétrico e de instalação que não depende de qual nó tem qual número.
> Extraído do antigo *Documento de Especificação e Detalhamento Técnico da Rede de
> Nós IoT da Estufa*, cuja topologia (Nó 00 Atuador, Nó 03 Hidroponia, Nó 02 como
> Canteiro C físico-químico) já não correspondia a nada — a frota real está em
> [`src/DRY/horta/inventario.md`](../src/DRY/horta/inventario.md).
>
> **Este documento não descreve a topologia.** Ele descreve como cabear, alimentar e
> vedar sensores num ambiente de estufa com motores indutivos por perto. Onde o
> firmware atual implementa (ou deixa de implementar) uma recomendação, está dito.

---

## 1. Cabeamento dos eletrodos de solo

### 1.1 Casamento de pares — a regra que evita cross-talk

Cabo Ethernet é trançado para cancelar ruído por indução. Ligar os sensores em vias
avulsas destrói esse cancelamento e cria **cross-talk**: a leitura de um eletrodo
altera a do vizinho dentro do mesmo cabo.

**Regra de ouro:** cada sensor usa **obrigatoriamente as duas vias de um mesmo par
trançado** — o fio colorido para o sinal analógico e o par-branco correspondente
para o GND.

| Par | Uso |
|---|---|
| Azul | Azul → sinal do eletrodo de 10 cm · Branco/Azul → GND |
| Laranja | Laranja → sinal do eletrodo de 20 cm · Branco/Laranja → GND |
| Verde | Verde → sinal do eletrodo de 30 cm · Branco/Verde → GND |
| Marrom | Reserva técnica, ou aterrado no GND do quadro como blindagem extra |

Um cabo de rede leva, portanto, **até 3 eletrodos com folga** — ou 4, gastando a
reserva.

### 1.2 UTP comum vs. FTP/STP blindado

| Tipo | Quando serve | Ressalva |
|---|---|---|
| **UTP** (sem blindagem) | Lances curtos, até ~2 m, longe da fiação de potência | Se passar encostado ou paralelo aos cabos de 220 V das bombas, a indução entra direto na leitura analógica |
| **FTP/STP** (folha de alumínio + fio de dreno) | **Escolha preferida** na estufa | A blindagem só funciona aterrada **em um único ponto** |

**Aterramento da malha:** conecte a folha de alumínio ao **GND do lado da caixa do
Arduino**, e deixe a ponta do lado do solo **isolada**. Aterrar os dois lados fecha
um laço de terra e transforma a blindagem em antena — o oposto do pretendido.

### 1.3 Distância e bitola

- Lances de no máximo **2 m**, o que se consegue fixando o nó no centro geométrico
  do canteiro em vez de na cabeceira.
- Os fios internos são **AWG24 de cobre sólido** e encaixam bem nos bornes Sindal de
  6 mm² na cabeça do sensor. Aperte o parafuso com moderação: excesso de torque
  degola o cobre, e a falha aparece semanas depois como leitura errática.

### 1.4 Geometria dos eletrodos

Varetas TIG de aço inox de **1,6 mm**, espaçamento fixo de **2,0 cm** e paralelismo
rígido entre elas. A leitura é resistiva e **não é calibrada**: qualquer variação de
distância ou ângulo entre as varetas muda a escala daquele ponto e o torna
incomparável com os demais. Ver a escala em
[`inventario.md` §9](../src/DRY/horta/inventario.md).

---

## 2. Alimentação pulsada e eletrólise

O solo adubado é eletrólito. Manter tensão contínua sobre os eletrodos corrói o aço
e envenena a leitura em poucas semanas. Por isso a barra de pull-up (**10 kΩ 1 %**)
é alimentada por um pino digital, energizada só durante a varredura.

Sequência implementada hoje nos nós 1 e 2 (`01nodeSolo3dNano.cpp`):

1. `PIN_POWER_SENSORS` (**D3**) em `HIGH`;
2. `delay(20)` — acomodação das capacitâncias parasitas dos cabos;
3. por canal: `delay(5)` → **uma `analogRead()` descartada** → `delay(3)` → leitura real;
4. `PIN_POWER_SENSORS` em `LOW` ao fim do ciclo.

A leitura descartada existe para purgar a carga acumulada no *sample and hold* do
ADC ao trocar de canal — sem ela, o valor de um canal contamina o seguinte.

> **Recomendação ainda não implementada:** o documento original pedia, além do
> descarte, **8 amostras sequenciais com 2 ms de intervalo e média aritmética**, para
> filtrar o ruído induzido pela proximidade com a fiação. O firmware faz o descarte e
> **uma única** amostra real. Se aparecer ruído nas séries de solo, é aqui que se
> mexe primeiro.

---

## 3. Cargas indutivas — supressão de transientes

O nó de atuação chaveia bombas e solenóides indutivos a poucos centímetros do
barramento lógico do Arduino. Sem supressão, o transiente de desligamento trava o
microcontrolador por EMI — falha que se manifesta como reset aleatório, não como
erro de código.

- **Solenóides e bombas 12 V:** diodo de roda livre em paralelo com a bobina —
  **cátodo (lado da barra) no +12 V, ânodo no GND**. Invertido, ele curto-circuita a
  fonte.
- **Contatos de relé chaveando carga indutiva:** **circuito snubber RC** em paralelo
  com o contato de saída, para absorver o arco.

Ver a pinagem e as cargas reais em
[`inventario.md` §7](../src/DRY/horta/inventario.md) e no
[`esquema_eletrico.md` do Nó 99](../src/DRY/horta/nos/99nodeReles/esquema_eletrico.md).

---

## 4. Vedação das junções

A atmosfera da estufa é corrosiva e permanentemente úmida. Toda junção fio-eletrodo
precisa sair da linha de gotejo com padrão **IP67**:

1. fixe o AWG24 no borne Sindal;
2. **vede com silicone neutro** — silicone acético libera ácido acético na cura e
   corrói o cobre e o inox;
3. feche a cabeça do sensor com **manga termorretrátil com adesivo interno**.

Junção malvedada não falha de vez: ela deriva. O sintoma é um canal que lê
consistentemente mais úmido que os vizinhos, o que é indistinguível de solo
realmente mais úmido até alguém desenterrar o eletrodo.

---

## 5. O que mudou desde o documento original

Registrado para quem for comparar com versões antigas do repositório:

| Documento original | Hoje |
|---|---|
| Um nó com MUX CD74HC4067 e 18 eletrodos cobrindo os canteiros A e B | Dois nós independentes (1 e 2), **6 eletrodos cada, direto nas portas analógicas nativas** A0–A5, sem MUX |
| Profundidades de 10, 20 e 30 cm | 10 e 30 cm, nas distâncias de 1 m, 3 m e 5 m |
| MUX no nó de solo | O MUX existe, mas no **Nó 99**, multiplexando relés de atuação |

O casamento de pares, a blindagem, a alimentação pulsada e a vedação continuam
valendo integralmente — mudou a contagem de canais, não a física.
