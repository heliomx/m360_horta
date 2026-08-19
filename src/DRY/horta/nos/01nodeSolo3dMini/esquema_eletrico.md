# Esquema Elétrico — Nó 01 Pro Mini (Monitoramento 3D de Solo)

Este documento descreve o esquema de ligações e detalhes de hardware para o **Nó 01 (Versão Arduino Pro Mini 5V / 16MHz ou 3.3V / 8MHz)** no modo de alimentação contínua (**ALWAYS_ON**).

## 1. Modificações de Hardware (Alimentação Constante 5V)
Como este nó é alimentado de forma constante com uma fonte de **5V DC** (via RAW ou USB):
1. **Não é necessário realizar modificações físicas** de baixo consumo na placa do Arduino Pro Mini (como dessoldar LEDs ou reguladores internos).
2. O pino **D3** (`PIN_POWER_SENSORS`) funciona como chave de controle de energia dos sensores para **mitigação de eletrólise**, mantendo-se desligado (`LOW`) e ligando apenas durante o momento exato das medições analógicas.

---

## 2. Mapeamento de Pinos (Pinout)

### Conexões Lógicas e RF
| Componente | Arduino Pro Mini | Função |
| :--- | :--- | :--- |
| **Adaptador do NRF24L01+** | **VCC** (Fonte 5V / RAW) | Alimentação do módulo (possui regulador 3.3V on-board) |
| | **GND** | Terra comum |
| **NRF24L01+ (via Adaptador)** | **D9** | CE (Chip Enable) |
| | **D10** | CSN (Chip Select) |
| | **D13** | SCK (SPI Clock) |
| | **D11** | MOSI (SPI Master Out) |
| | **D12** | MISO (SPI Master In) |
| **CD74HC4067 (MUX)** | **VCC** | Alimentação lógica (conectar ao VCC da MCU) |
| | **GND** | Terra comum |
| | **EN** | GND (Habilitado permanente) |
| | **S0** | D4 (Seletor 0) |
| | **S1** | D5 (Seletor 1) |
| | **S2** | D6 (Seletor 2) |
| | **S3** | D7 (Seletor 3) |
| | **SIG** | A0 (Leitura Analógica Multiplexada) |

### Conexões dos Sensores Resistivos (Opção B - Pull-Up Compartilhado)
Para economizar componentes, o nó utiliza um esquema de divisor de tensão com **Pull-up compartilhado** para os canais do multiplexador. A resistência interna de condução ($R_{\text{on}} \approx 70\Omega$) do CD74HC4067 é compensada automaticamente via software.

* **Canais do Multiplexador (Sensores 0 a 15)**: 
  * Um único resistor de **10kΩ** é conectado entre o pino **D3** (`PIN_POWER_SENSORS`) e o pino **A0** (`MUX_PIN_SIG`).
  * Cada canal do MUX ($C_0$ a $C_{15}$) conecta-se diretamente ao eletrodo positivo de seu respectivo sensor.
* **Canais Nativos (Sensores 16 e 17)**:
  * Como não passam pelo MUX, estes dois sensores necessitam de resistores de **10kΩ individuais** conectados entre o pino **D3** e suas respectivas portas (**A1** e **A2**).

| Sensor / Canal | Ponto de Medição (MCU) | Resistor de Pull-Up (10kΩ) | Conexão do Sensor |
| :--- | :--- | :--- | :--- |
| **Sensores 0 a 15** | Pino **A0** (SIG do MUX) | **1 único** conectado entre **A0** e o pino **D3** | Eletrodo (+) direto no canal do MUX ($C_0$-$C_{15}$) |
| **Sensor 16** | Pino **A1** (Nativo) | **1 resistor** conectado entre **A1** e o pino **D3** | Eletrodo (+) conectado ao pino **A1** |
| **Sensor 17** | Pino **A2** (Nativo) | **1 resistor** conectado entre **A2** e o pino **D3** | Eletrodo (+) conectado ao pino **A2** |

*Todos os eletrodos negativos (-) dos sensores no solo conectam-se ao GND comum do circuito.*

### Tabela de Mapeamento Físico dos Eletrodos (Canteiros A e B)

Abaixo está o mapeamento detalhado de cada Sensor ID para sua posição física nos Canteiros A e B:

| Child ID / Sensor ID | Canal físico de Leitura | Canteiro | Posição (Distância / Profundidade) | Label do Sensor |
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

## 3. Circuito de Alimentação (Alimentação Constante 5V - Modo ALWAYS_ON)

O transceptor **NRF24L01+** opera estritamente na faixa de **1.9V a 3.6V** e não tolera tensões de 5V. O uso de um **Módulo Adaptador para o Rádio** (com regulador LDO de 3.3V onboard, tipo AMS1117-3.3) é obrigatório para rebaixar a tensão e filtrar ruídos na linha de alimentação.

* **Conexões de Alimentação**:
  - **Fonte de Alimentação 5V DC**: Conectada ao pino **RAW** (ou **VCC** se a fonte for regulada) do Arduino Pro Mini.
  - **Alimentação do MUX CD74HC4067**: Conectada ao barramento de alimentação de 5V (suporta operação até 6V).
  - **Alimentação do Rádio (via Adaptador)**: O adaptador de soquete recebe os 5V diretamente no pino `VCC` e o regulador on-board reduz para 3.3V estáveis para o NRF24L01+.
  - **Pino D3 (`PIN_POWER_SENSORS`)**: Funciona como chave de energia temporária dos sensores (ligado somente no momento exato da leitura para mitigar eletrólise).

### Esquema de Blocos:
```text
Fonte 5V DC ──┬──> RAW / VCC Arduino Pro Mini (5V)
              ├──> VCC MUX CD74HC4067 (5V)
              ├──> VCC Módulo Adaptador do Rádio ──> [Regulador 3.3V] ──> VCC NRF24L01+
              └──> GND do Sistema (Comum)
```
