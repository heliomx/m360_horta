# Esquema Elétrico — Nó 4 (Clima Solar Mini)

## Resumo do Hardware
- **Controlador**: Arduino Pro Mini (ATmega328P, 3.3V / 8MHz)
- **Rádio**: nRF24L01+ 
- **Alimentação**: Bateria + Painel Solar / Regulador (M360_LOW_POWER)
- **Sensores**: 
  - DHT11 (Sensor de Temperatura e Umidade)

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

### Sensor DHT11 e Controle de VCC Chaveado
Para maximizar a economia de energia da bateria, a linha de alimentação (VCC) do sensor DHT11 é conectada a um pino digital do microcontrolador (D3), permitindo desligá-lo completamente durante os ciclos de repouso (smartSleep).

| Sensor | Pino de Sinal | Pino de Energia (VCC) | Pino de GND |
| :--- | :--- | :--- | :--- |
| **DHT11 (Temp/Hum)** | D4 (Dados) | D3 (VCC Chaveado) | GND |

---

## Notas de Montagem e Funcionamento

### 1. Inicialização e Estabilização
*   O pino **D3** é mantido em nível baixo (`LOW`) durante o período em que o nó está dormindo.
*   Ao acordar, o firmware coloca o pino **D3** em nível alto (`HIGH`), energizando o DHT11.
*   **Tempo de Estabilização:** O DHT11 precisa de pelo menos 1 a 1.5 segundos após ser alimentado para que suas leituras estejam estáveis. O driver executa um `wait(1500)` para garantir este intervalo antes de efetuar a leitura física dos dados no pino **D4**.
*   Após a leitura de todos os canais de sensores, o pino **D3** retorna para `LOW` e o nó entra em smartSleep via rádio.

### 2. Barramento de Dados (D4)
*   *Nota: O pino **D4** de comunicação do DHT11 requer um resistor de pull-up físico de 4.7kΩ a 10kΩ conectado entre o sinal (D4) e a linha de alimentação controlada (D3).*
