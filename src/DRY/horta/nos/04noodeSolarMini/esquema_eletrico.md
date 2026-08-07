# Esquema Elétrico — Nó 4 (Clima Solar Mini)

## Resumo do Hardware
- **Controlador**: Arduino Pro Mini (ATmega328P, 3.3V / 8MHz)
- **Rádio**: nRF24L01+ 
- **Alimentação**: Bateria + Painel Solar / Regulador (M360_LOW_POWER)
- **Sensores**: 
  - DHT11 (Sensor de Temperatura e Umidade do Ar)
  - DS18B20 (Sensor de Temperatura do Solo)

---

## Mapeamento de Pinos (Pinout)

### Rádio nRF24L01+
O rádio utiliza os pinos padrão da interface SPI do Arduino Pro Mini:

| nRF24L01+ | Arduino Pro Mini | Função |
| :--- | :--- | :--- |
| VCC | VCC (3.3V) | Alimentação regulada dedicada do rádio |
| GND | GND | Terra comum |
| CE | D9 | Chip Enable (Padrão) |
| CSN | D10 | Chip Select Not (Padrão) |
| SCK | D13 | SPI Clock |
| MOSI | D11 | SPI Master Out Slave In |
| MISO | D12 | SPI Master In Slave Out |
| IRQ | Não Conectado | Interrupção não usada |

### Sensores e Controle de VCC Chaveado
Para maximizar a economia de energia da bateria, a linha de alimentação (VCC) dos sensores DHT11 e DS18B20 é conectada ao pino digital do microcontrolador (**D3**), permitindo desligá-los completamente durante os ciclos de repouso (smartSleep).

| Sensor / Medição | Pino de Sinal / Leitura | Pino de Energia (VCC) | Pino de GND |
| :--- | :--- | :--- | :--- |
| **DHT11 (Temp/Hum Ar)** | D4 (Dados) | D3 (VCC Chaveado) | GND |
| **DS18B20 (Temp Solo)** | D5 (OneWire Dados) | D3 (VCC Chaveado) | GND |
| **Leitura da Bateria** | A0 (Analógico) | Polo (+) da Bateria (antes do LDO) | GND (via resistor) |

---

## Notas de Montagem e Funcionamento

### 1. Inicialização e Estabilização
*   O pino **D3** é mantido em nível baixo (`LOW`) durante o período em que o nó está dormindo.
*   Ao acordar, o firmware coloca o pino **D3** em nível alto (`HIGH`), energizando o DHT11 e o DS18B20.
*   **Tempo de Estabilização:** O DHT11 precisa de 1 a 1.5s após ser alimentado e o DS18B20 requer ~750ms para a conversão de 12 bits. O driver executa um `wait(1500)` para garantir ambos os intervalos antes de efetuar a leitura física dos dados nos pinos **D4** e **D5**.
*   Após a leitura de todos os canais de sensores, o pino **D3** retorna para `LOW` e o nó entra em smartSleep via rádio.

### 2. Barramento de Dados DHT11 (D4) e OneWire DS18B20 (D5)
*   *Nota DHT11: O pino **D4** requer um resistor de pull-up físico de 4.7kΩ a 10kΩ conectado entre o sinal (D4) e a linha de alimentação controlada (D3).*
*   *Nota DS18B20: O pino **D5** (barramento OneWire) requer um resistor de pull-up de 4.7kΩ conectado entre o sinal de dados (D5) e a linha de alimentação controlada (D3).*

### 3. Circuito de Medição da Bateria (Divisor de Tensão)
*   **Divisor de Tensão:** Para medir a tensão real da bateria (variável entre 3.0V e 4.2V) sem que o regulador LDO de 3.3V mascare a leitura com uma saída constante, instala-se um divisor de tensão composto por dois resistores de **100kΩ** em série.
*   **Esquema de Ligação:**
    - O topo do divisor é conectado ao polo positivo (+) da bateria (antes da entrada do regulador LDO 3.3V).
    - O ponto central (entre os dois resistores) é conectado ao pino analógico **A0** do Arduino Pro Mini.
    - A base do divisor é ligada ao terra comum (**GND**).
*   **Funcionamento:** Como os resistores são iguais, a tensão é dividida exatamente por 2. Quando a bateria estiver totalmente carregada em 4.2V, o pino **A0** receberá 2.1V (nível seguro, inferior ao limite de 3.3V do pino analógico). O firmware faz a leitura analógica e multiplica o valor lido por `(6.6V / 1023)` para recompor a leitura real da bateria em Volts.

