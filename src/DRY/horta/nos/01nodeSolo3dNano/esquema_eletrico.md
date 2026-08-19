# Esquema Elétrico — Nó 1 (Solo3dNano)

Este documento detalha as conexões físicas e lógicas do **Nó 01 (Solo3dNano)** (Monitoramento 3D de Solo, versão Arduino Nano), responsável por ler 6 sensores de umidade diretamente.

---

## Mapeamento de Pinos (Pinout)

### Rádio nRF24L01+
O rádio utiliza a interface SPI padrão do Arduino Nano:

| nRF24L01+ | Arduino Nano | Descrição |
| :--- | :--- | :--- |
| VCC | 3.3V | Alimentação regulada dedicada do rádio |
| GND | GND | Terra comum |
| CE | D9 | Chip Enable (Padrão) |
| CSN | D10 | Chip Select Not (Padrão) |
| SCK | D13 | SPI Clock |
| MOSI | D11 | SPI Master Out |
| MISO | D12 | SPI Slave Out |

---

### Sensores de Solo e Controle de Pull-ups
Para mitigar o efeito de eletrólise nos eletrodos de metal enterrados, a alimentação positiva dos resistores de pull-up é conectada ao pino digital **D3** (`PIN_POWER_SENSORS`), que é mantido em nível `LOW` e ligado apenas no milissegundo em que as medições ocorrem.

| Sensor (Canal) | Pino Analógico Arduino | Pino de Alimentação (Pull-up) |
| :---: | :--- | :--- |
| **Sensor 0 (A_1m_10cm)** | A0 | D3 (VCC Chaveado via resistor 10k) |
| **Sensor 1 (A_1m_30cm)** | A1 | D3 (VCC Chaveado via resistor 10k) |
| **Sensor 2 (A_3m_10cm)** | A2 | D3 (VCC Chaveado via resistor 10k) |
| **Sensor 3 (A_3m_30cm)** | A3 | D3 (VCC Chaveado via resistor 10k) |
| **Sensor 4 (A_5m_10cm)** | A4 | D3 (VCC Chaveado via resistor 10k) |
| **Sensor 5 (A_5m_30cm)** | A5 | D3 (VCC Chaveado via resistor 10k) |

---

## Notas de Montagem e Funcionamento

### 1. Ligação dos Resistores de Pull-up
Cada pino analógico (`A0` a `A5`) deve receber um resistor de **$10\text{ k}\Omega$** conectado ao pino digital **D3**. O eletrodo de medição do solo correspondente é ligado entre o mesmo pino analógico e o **GND**.
*   **Funcionamento:** Quando **D3** está `HIGH` (5V), o resistor de pull-up de 10k e a resistência elétrica do solo formam um divisor de tensão. O pino analógico correspondente lê a queda de tensão resultante.
*   **Mitigação de Eletrólise:** Mantendo **D3** em `LOW` quando não há medição, cessa a circulação de corrente contínua entre os eletrodos de metal no solo úmido, prevenindo o desgaste físico e oxidação acelerada das pontas sensoras.
