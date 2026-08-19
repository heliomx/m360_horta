# Esquema Elétrico — Nó 05 (Monitoramento 3D de Solo Unificado)

Este documento descreve o esquema elétrico, fiação física e pinagem detalhada do **Nó 05 (05nodeSolo3dNanoMux)** utilizando o Arduino Nano.

---

## 1. Mapeamento de Pinos (Pinout)

### Conexões SPI (Arduino Nano -> Rádio RF24L01+)
O rádio opera estritamente com **3.3V**. Recomenda-se utilizar um módulo adaptador de soquete com regulador de 3.3V on-board conectado à fonte de 5V.

| Pino do Rádio | Pino Arduino Nano | Descrição |
| :--- | :--- | :--- |
| **VCC** | Regulador 3.3V (AMS1117) | Alimentação estável do rádio |
| **GND** | GND Comum | Referência de Terra |
| **CE** | D9 | Chip Enable (Padrão do código) |
| **CSN** | D10 | Chip Select Not (Padrão do código) |
| **SCK** | D13 | Serial Clock |
| **MOSI** | D11 | Master Out Slave In |
| **MISO** | D12 | Master In Slave Out |

### Conexões de Controle e Sinal (Arduino Nano -> MUX CD74HC4067)
| Pino MUX CD74HC4067 | Pino Arduino Nano | Descrição |
| :--- | :--- | :--- |
| **VCC** | 5V | Alimentação lógica do CI multiplexador |
| **GND** | GND Comum | Referência de Terra |
| **EN** | GND | Pino Enable (Mantido conectado ao terra para sempre habilitado) |
| **SIG** | A0 | Canal comum de sinal multiplexado |
| **S0** | D4 | Seletor de canal - Bit 0 |
| **S1** | D5 | Seletor de canal - Bit 1 |
| **S2** | D6 | Seletor de canal - Bit 2 |
| **S3** | D7 | Seletor de canal - Bit 3 |

### Pinos Nativos de Leitura Analógica (Arduino Nano)
| Sensor | Pino Arduino Nano | Descrição |
| :--- | :--- | :--- |
| **Sensor 16** | A1 | Canal nativo analógico direto (sem MUX) |
| **Sensor 17** | A2 | Canal nativo analógico direto (sem MUX) |

### Controle de Alimentação de Sensores (Arduino Nano)
| Recurso | Pino Arduino Nano | Descrição |
| :--- | :--- | :--- |
| **PIN_POWER_SENSORS** | D3 | Chave de alimentação dos pull-ups (ativo apenas durante a leitura) |

---

## 2. Conexões dos Sensores Resistivos (Divisor de Tensão)

Para simplificar a montagem e economizar componentes, o nó adota um esquema com **Pull-up Único Compartilhado** para os sensores do multiplexador:

* **Sensores 0 a 15 (Chaveados via MUX):**
  * Conecte um único resistor de **10kΩ** entre o pino **D3** (`PIN_POWER_SENSORS`) e o pino **A0** (`MUX_PIN_SIG`).
  * Cada canal físico do multiplexador ($C_0$ a $C_{15}$) conecta-se diretamente ao eletrodo positivo (+) de seu respectivo sensor.
* **Sensores 16 e 17 (Nativos):**
  * Como não passam pelo multiplexador, estes dois sensores precisam de resistores de **10kΩ individuais** dedicados.
  * Conecte um resistor de **10kΩ** entre o pino **D3** e o pino **A1**.
  * Conecte um resistor de **10kΩ** entre o pino **D3** e o pino **A2**.
  * Conecte o eletrodo positivo (+) do sensor 16 ao pino **A1**.
  * Conecte o eletrodo positivo (+) do sensor 17 ao pino **A2**.

*Todos os eletrodos negativos (-) dos 18 sensores devem ser unidos ao GND (Terra comum) do circuito.*

### Esquema de Ligações dos Sensores

```text
               D3 (PIN_POWER_SENSORS)
               │
      ┌────────┴────────┬────────┐
      │                 │        │
    [10k]             [10k]    [10k]  <-- Resistores de Pull-up (Estabilização de Sinal)
      │                 │        │
      ├──> A0 (SIG Mux) ├──> A1  ├──> A2
      │                 │        │
   [ MUX ]              │        │
   C0..C15              │        │
      │                 │        │
    (S0..15)          (S16)    (S17)
      │                 │        │
    [Solo]            [Solo]   [Solo]  <-- Resistência variável de umidade do solo
      │                 │        │
     GND               GND      GND
```

---

## 3. Tabela de Mapeamento Físico e Lógico dos Eletrodos

O nó unificado divide as leituras em dois grupos de 9 sensores para os Canteiros A e B:

| Child ID | Canal de Leitura | Canteiro | Posição (Profundidade / Distância) | Label no Home Assistant |
| :---: | :--- | :---: | :---: | :--- |
| **0** | MUX $C_0$ | Canteiro A | 1m de distância, 10cm de profundidade | `A_1m_10cm` |
| **1** | MUX $C_1$ | Canteiro A | 1m de distância, 20cm de profundidade | `A_1m_20cm` |
| **2** | MUX $C_2$ | Canteiro A | 1m de distância, 30cm de profundidade | `A_1m_30cm` |
| **3** | MUX $C_3$ | Canteiro A | 3m de distância, 10cm de profundidade | `A_3m_10cm` |
| **4** | MUX $C_4$ | Canteiro A | 3m de distância, 20cm de profundidade | `A_3m_20cm` |
| **5** | MUX $C_5$ | Canteiro A | 3m de distância, 30cm de profundidade | `A_3m_30cm` |
| **6** | MUX $C_6$ | Canteiro A | 5m de distância, 10cm de profundidade | `A_5m_10cm` |
| **7** | MUX $C_7$ | Canteiro A | 5m de distância, 20cm de profundidade | `A_5m_20cm` |
| **8** | MUX $C_8$ | Canteiro A | 5m de distância, 30cm de profundidade | `A_5m_30cm` |
| **9** | MUX $C_9$ | Canteiro B | 1m de distância, 10cm de profundidade | `B_1m_10cm` |
| **10** | MUX $C_{10}$ | Canteiro B | 1m de distância, 20cm de profundidade | `B_1m_20cm` |
| **11** | MUX $C_{11}$ | Canteiro B | 1m de distância, 30cm de profundidade | `B_1m_30cm` |
| **12** | MUX $C_{12}$ | Canteiro B | 3m de distância, 10cm de profundidade | `B_3m_10cm` |
| **13** | MUX $C_{13}$ | Canteiro B | 3m de distância, 20cm de profundidade | `B_3m_20cm` |
| **14** | MUX $C_{14}$ | Canteiro B | 3m de distância, 30cm de profundidade | `B_3m_30cm` |
| **15** | MUX $C_{15}$ | Canteiro B | 5m de distância, 10cm de profundidade | `B_5m_10cm` |
| **16** | Porta Nativa A1 | Canteiro B | 5m de distância, 20cm de profundidade | `B_5m_20cm` |
| **17** | Porta Nativa A2 | Canteiro B | 5m de distância, 30cm de profundidade | `B_5m_30cm` |

---

## 4. Recomendações e Cuidados de Instalação

1. **GND Unificado:** É indispensável interligar todos os pinos de GND (Terra) da fonte, do multiplexador, do rádio e do Arduino. A falta de referência de terra comum provoca oscilações drásticas nos valores obtidos pelas portas analógicas.
2. **Capacitor de Desacoplamento no Rádio:** Adicione um capacitor eletrolítico de **10µF a 47µF** (em paralelo com um capacitor cerâmico de **100nF**) diretamente nos pinos de alimentação do NRF24L01+. Os picos de corrente na transmissão RF geram quedas de tensão instantâneas que travam o circuito integrado do rádio ou causam perda de pacotes.
3. **Impedância dos Cabos:** Como os eletrodos de inox são ligados por cabos longos (normalmente pares trançados de cabo de rede Cat5e/Cat6), as capacitâncias parasitas do cabo podem distorcer o sinal lido. A lógica de leitura do firmware inclui um atraso de estabilização elétrica de **5ms** a cada mudança de canal do MUX e realiza uma **leitura analógica de descarte** antes do cálculo real, purgando o capacitor interno do ADC da MCU.
