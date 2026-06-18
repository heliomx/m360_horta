# Esquema Elétrico — Nó 01 Pro Mini (Monitoramento 3D de Solo)

Este documento descreve o esquema de ligações e detalhes de hardware para o **Nó 01 (Versão Arduino Pro Mini 3.3V / 8MHz)** em modo de baixo consumo.

## 1. Modificações de Baixo Consumo no Arduino Pro Mini
Para atingir uma corrente de sleep de microamperes (idealmente ~5µA a ~15µA), faça as seguintes alterações físicas na placa do Arduino Pro Mini:
1. **Remover o LED de Power (PWR)**: Dessolde ou corte a trilha do LED indicador de energia integrado.
2. **Remover/Bypass do Regulador de Tensão Onboard**: Dessolde o regulador linear interno da placa se for alimentar a MCU diretamente pela bateria no pino VCC.

---

## 2. Mapeamento de Pinos (Pinout)

### Conexões Lógicas e RF
| Componente | Arduino Pro Mini | Função |
| :--- | :--- | :--- |
| **Adaptador do NRF24L01+** | **VCC** (Bateria / OUT+) | Alimentação do módulo (possui regulador 3.3V on-board) |
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

---

## 3. Circuito de Alimentação (Bateria e Placa Solar)

O transceptor **NRF24L01+** opera na faixa de **1.9V a 3.6V** e não tolera tensões superiores. Para alimentar o circuito com segurança usando uma bateria de Li-ion (3.0V a 4.2V) e placa solar, a seguinte topologia é implementada:

### Componentes de Potência
1. **Controlador de Carga (TP4056 ou similar)**: Gerencia o carregamento da bateria por meio do painel solar.
2. **Módulo Adaptador para o Rádio**: Um adaptador de soquete de 8 pinos para o NRF24L01+ que possui um regulador LDO de 3.3V on-board (normalmente o chip AMS1117-3.3) e capacitores de filtragem integrados.

### Conexões de Alimentação
- **Painel Solar (5V-6V)**: Conectado às portas de entrada `IN+` e `IN-` do controlador de carga.
- **Bateria Li-ion (18650 ou similar)**: Conectada às portas `BAT+` e `BAT-` do controlador.
- **Linha de Distribuição VCC (3.0V - 4.2V)**: Conectada à saída `OUT+` do controlador de carga. Alimenta em paralelo:
  1. O pino **VCC** do Arduino Pro Mini (alimentação direta da MCU para permitir a leitura correta e direta da voltagem da bateria via ADC de 1.1V interno).
  2. O pino **VCC** do módulo adaptador do rádio (o regulador do adaptador se encarregará de rebaixar a tensão para os 3.3V adequados ao NRF24L01+).
  3. O pino **VCC** do multiplexador CD74HC4067 (suporta operação estável de 2V a 6V).
- **Linha GND (Terra)**: Conectada à saída `OUT-` do controlador, unificando o terra de todo o sistema.

### Esquema de Blocos da Alimentação:
```text
Painel Solar (5V-6V) ───> [ IN+ ]  Controlador  [ OUT+ ] ──┬──> VCC Arduino Pro Mini (3.0V - 4.2V)
                     ───> [ IN- ]   de Carga    [ OUT- ] ──┼──> VCC MUX CD74HC4067 (3.0V - 4.2V)
                                                           ├──> VCC Módulo Adaptador do Rádio ──> [Regulador 3.3V] ──> VCC NRF24L01+
                                                           └──> GND do Sistema (Comum)
```
