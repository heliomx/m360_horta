# Esquema Elétrico — Nó 01 (Monitoramento 3D de Solo)

## Resumo do Hardware
- **Controlador**: Arduino Nano (ATmega328P, 5V)
- **Rádio**: NRF24L01+ (com adaptador base 3.3V)
- **Multiplexador Analógico**: CD74HC4067 (16 Canais)
- **Sensores**: 18 Sensores de Umidade de Solo Resistivos (Eletrodos de Aço Inox).

## Mapeamento de Pinos (Pinout)

### Rádio NRF24L01+
| NRF24L01+ | Arduino Nano | Função |
| :--- | :--- | :--- |
| VCC | 3.3V (Regulador) | Alimentação Dedicada |
| GND | GND | Terra comum |
| CE | D9 | Chip Enable |
| CSN | D10 | Chip Select Not |
| SCK | D13 | SPI Clock |
| MOSI | D11 | SPI Master Out Slave In |
| MISO | D12 | SPI Master In Slave Out |
| IRQ | Não Conectado | Não utilizado no perfil ALWAYS_ON |

### Multiplexador CD74HC4067
| MUX | Arduino Nano | Função |
| :--- | :--- | :--- |
| VCC | 5V | Alimentação lógica |
| GND | GND | Terra |
| EN | GND | Enable (Sempre Ativo / LOW) |
| S0 | D4 | Seletor Lógico 0 |
| S1 | D5 | Seletor Lógico 1 |
| S2 | D6 | Seletor Lógico 2 |
| S3 | D7 | Seletor Lógico 3 |
| SIG | A0 | Sinal Analógico Multiplexado |
| C0-C15 | - | Conectados aos sinais dos Sensores 0 a 15 |

### Sensores Resistivos (Conexão via Cabos de Rede Ethernet)
**Importante (Eletrólise)**: A barra de resistores de Pull-up de 10kΩ **NÃO** é conectada aos 5V fixos, mas sim ao pino **D3**. O `D3` fornece 5V apenas durante os breves milissegundos da leitura (varredura), evitando a degradação galvânica dos eletrodos de aço inox e protegendo a vida útil dos mesmos.

| Sensor | Canal de Leitura | Resistor Pull-up |
| :--- | :--- | :--- |
| **Sensores 0 a 15 (Canteiro A e B)** | Entradas C0 a C15 do MUX | Conectado entre o Canal do MUX (C0-C15) e o pino **D3** (5V Temporário) |
| **Sensor 16 (Canteiro B)** | Pino Analógico Nativo **A1** | Conectado entre **A1** e o pino **D3** (5V Temporário) |
| **Sensor 17 (Canteiro B)** | Pino Analógico Nativo **A2** | Conectado entre **A2** e o pino **D3** (5V Temporário) |

> **Nota de Montagem e Cabos**: Cada cabo de rede UTP/FTP (Cat5e/Cat6) possui 4 pares trançados. Para anular ruídos de crosstalk, para cada par de eletrodos, utiliza-se exatamente as duas vias de um único par trançado. O fio colorido transmite o sinal analógico, e o fio par-branco conecta-se ao GND comum. A malha de blindagem metálica (no caso de cabos FTP) deve ser aterrada (GND) apenas do lado da caixa plástica do Arduino para evitar loops de terra (ground loops).

## Esquemático de Prevenção de Eletrólise
```text
           D3 (Pino Digital, fornece 5V temporariamente)
           │
           ├──────── 10kΩ ────────┐
           │                      │
           ├──────── 10kΩ ──────┐ │
           ...                  │ │
                             A1 │ A0 (SIG MUX <- C0)
                                │ │
                                │ │ (Cabo de Rede Par 1)
                                │ └─────────────────── Eletrodo (+) (Solo)
                                │
                                └───────────────────── Eletrodo (+) (Solo)
      
GND (Arduino) ─────────────────┬────────────────────── Eletrodo (-) (Solo)
                               │
                               └────────────────────── Eletrodo (-) (Solo)
```
