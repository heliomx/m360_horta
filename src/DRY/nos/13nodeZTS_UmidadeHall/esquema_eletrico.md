# Esquema Elétrico — Nó 13 (ZTS_UmidadeHall)

Este documento detalha as conexões pino a pino para a montagem de hardware do **Nó 13 (ZTS_UmidadeHall)**, responsável por ler o sensor modbus ZTS-3002, sensores locais (Hall e LDR) e acionar o módulo relé de válvula solenoide.

---

## 1. Núcleo (Arduino Pro Mini)

| Pino Arduino | Componente | Sinal / Função |
| :---: | :--- | :--- |
| **VCC** | Barramento 3.3V | Alimentação principal regulada (saída do regulador LDO 3.3V) |
| **GND** | Barramento GND | Comum a todos os módulos e componentes |
| **RAW** | Fonte 12V | Entrada de alimentação não regulada se utilizar regulador integrado |

---

## 2. Comunicação RS485 (Módulo MAX485)

O módulo MAX485 permite a leitura do sensor ZTS-3002 via protocolo Modbus.

| Pino MAX485 | Pino Arduino | Descrição |
| :--- | :---: | :--- |
| **VCC** | 5V / 3.3V | Alimentação do driver (MAX485 pode rodar em 5V ou 3.3V dependendo do módulo) |
| **GND** | GND | Referência |
| **RO** | **D2** | Receive Out (RX do SoftwareSerial) |
| **DI** | **D3** | Driver In (TX do SoftwareSerial) |
| **RE / DE** | **D7 / D8** | Controle de fluxo (DE: D7, RE: D8) |

### 2.1 Melhores Práticas RS485 (Físico)

| Item | Recomendação | Descrição |
| :--- | :--- | :--- |
| **Topologia** | **Daisy-Chain** | Conecte os nós em linha. Evite topologia em estrela. |
| **Terminação** | **Resistor 120Ω** | Instale um resistor de 120Ω entre as linhas A e B em cada ponta do barramento. |
| **Cabo** | **STP (Shielded)** | Use cabo de par trançado blindado (ex: Belden 9841). |
| **Blindagem** | **GND Único** | Conecte a blindagem ao GND em apenas um ponto (preferencialmente no Gateway). |
| **Stubs** | **&lt; 30cm** | Derivações para cada sensor devem ser o mais curtas possível. |

> [!TIP]
> No firmware, DE está em D7 e RE em D8. RO/DI foram movidos para D2/D3 para liberar os pinos D9/D10 para uso do rádio nRF24L01+.

---

## 3. Rádio MySensors (nRF24L01+)

O rádio utiliza a interface SPI padrão do Arduino Pro Mini, mantendo CE e CSN nos pinos padrão:

| Pino nRF24 | Pino Arduino | Descrição |
| :--- | :---: | :--- |
| **VCC** | **3.3V** | Alimentação do rádio (recomenda-se capacitor de 10µF a 47µF) |
| **GND** | GND | Referência |
| **CE** | **D9** | Chip Enable (padrão MySensors) |
| **CSN** | **D10** | SPI Chip Select (padrão MySensors) |
| **SCK** | **D13** | SPI Clock |
| **MISO** | **D12** | SPI Master In Slave Out |
| **MOSI** | **D11** | SPI Master Out Slave In |

---

## 4. Sensores e Atuadores Locais

| Componente | Pino Arduino | Detalhe da Conexão |
| :--- | :---: | :--- |
| **Sensor Umidade (Hall)** | **A0** | Sensor Linear (ex: SS495A) -> Saída para A0 (Analógico) |
| **Sensor de Luz (LDR)** | **A1** | Divisor de tensão com LDR -> Saída para A1 (Analógico) |
| **Módulo Relé** | **D5** | Pino de Sinal IN -> D5 (Chaveia alimentação 12V do ZTS / Válvula) |

---

## 5. Conexão do Sensor ZTS-3002 (Modbus via Relé)

O sensor ZTS-3002 é alimentado apenas quando o relé é acionado (D5 LOW), prolongando a vida útil da bateria.

| Fio Sensor | Conexão | Função |
| :--- | :--- | :--- |
| **Vermelho** (VCC) | **Terminal NO do Relé** | Alimentação positiva (Chaveada via 12V da fonte) |
| **Preto** (GND) | **GND** | Comum (Referência) |
| **Amarelo** (A) | **Terminal A do MAX485** | RS485 A (+) |
| **Azul** (B) | **Terminal B do MAX485** | RS485 B (-) |

---

## 6. Fluxograma de Alimentação

1. **Fonte Externa (12V DC)** --> Entrada do regulador de tensão chaveado (Step-down para 5V ou LDO para 3.3V).
2. **Saída 5V** --> Alimenta **MAX485 (VCC)** e **Relé (VCC)**.
3. **Saída 3.3V** --> Alimenta **Arduino (VCC)**, **nRF24L01+ (VCC)** e **Hall Sensor (VCC)**.
4. **Fonte Externa (12V DC)** --> Terminal Comum (COM) do Relé.
5. **Relé (NO)** --> Fio Vermelho (VCC) do Sensor ZTS-3002 e Alimentação da Válvula.
