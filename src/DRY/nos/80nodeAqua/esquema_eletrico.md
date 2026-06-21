# Esquema Elétrico — Nó 80 (Caixa D'água, pH, EC e Vazão)

## Resumo do Hardware
- **Controlador**: Arduino Nano (ATmega328P, 5V / 16MHz)
- **Rádio**: nRF24L01+ 
- **Alimentação**: Fonte 5V DC direta (Painel Elétrico) ou USB (ALWAYS_ON)
- **Sensores**: 
  - HC-SR04 (Ultrassônico de Nível)
  - pH-4502C (Sensor de pH)
  - EC (Sensor de Condutividade Elétrica)
  - YF-S201 (Sensor de Vazão de Efeito Hall)

---

## Mapeamento de Pinos (Pinout)

### Rádio nRF24L01+
O rádio utiliza os pinos padrão da interface SPI do Arduino Nano:

| nRF24L01+ | Arduino Nano | Função |
| :--- | :--- | :--- |
| VCC | 3.3V | Alimentação regulada dedicada do rádio |
| GND | GND | Terra comum |
| CE | D9 | Chip Enable (Padrão) |
| CSN | D10 | Chip Select Not (Padrão) |
| SCK | D13 | SPI Clock |
| MOSI | D11 | SPI Master Out Slave In |
| MISO | D12 | SPI Master In Slave Out |
| IRQ | Não Conectado | Interrupção não usada |

### Sensores e Controle de VCC Chaveado
Para possibilitar reinicialização de hardware dos sensores ou desligamento temporário, o pino digital **D7** funciona como chave de alimentação dos sensores de Nível, pH, EC e Temperatura. Os sensores de Vazão são alimentados diretamente pelo barramento de **5V** para evitar perda de pulsos de interrupção.

| Sensor | Pino de Sinal | Pino de Energia (VCC) | Pino de GND |
| :--- | :--- | :--- | :--- |
| **HC-SR04 (Nível)** | D4 (Trigger), D5 (Echo) | D7 (VCC Chaveado) | GND |
| **pH-4502C (pH)** | A0 (Leitura Analógica) | D7 (VCC Chaveado) | GND |
| **EC (Condutividade)** | A1 (Leitura Analógica) | D7 (VCC Chaveado) | GND |
| **YF-S201 (Vazão 1)** | D2 (Pulso / INT0) | 5V (Direto) | GND |
| **DS18B20 (Temp Água)** | D6 (OneWire) | D7 (VCC Chaveado) | GND |
| **YF-S201 (Vazão A)** | D3 (Pulso / INT1) | 5V (Direto) | GND |
| **YF-S201 (Vazão B)** | D8 (Pulso / PCINT0) | 5V (Direto) | GND |
| **YF-S201 (Vazão C)** | A2 (Pulso / PCINT10) | 5V (Direto) | GND |

---

## Notas de Montagem e Calibração

### 1. Sensor Ultrassônico (HC-SR04)
*   **Trigger (D4)** envia um pulso de 10us para iniciar a medição.
*   **Echo (D5)** mede a largura do pulso de retorno.
*   *Nota: A caixa d'água possui calibração teórica para 100 cm de altura máxima (caixa vazia) e 10 cm de altura mínima (caixa cheia).*

### 2. Sensor de pH (pH-4502C)
*   Opera com alimentação de 5V. O pino analógico **A0** lê a tensão de saída do condicionador de sinal.
*   *Fórmula padrão: pH = -5.7 * Tensão + 21.34 (Ajustar potenciômetro de offset físico em pH 7.0 para saída de 2.5V).*

### 3. Sensor de EC
*   Opera com alimentação de 5V. O pino analógico **A1** lê a tensão proporcional à condutividade elétrica do líquido.
*   *Fórmula padrão: EC = 1.0 * Tensão (Ajustar conforme calibração de condutividade em solução padrão).*

### 4. Sensores de Vazão (YF-S201)
*   **Vazão 1**: Conectado em **D2** utilizando a interrupção externa do ATmega328P (`INT0`).
*   **Vazão A**: Conectado em **D3** utilizando a interrupção externa do ATmega328P (`INT1`).
*   **Vazão B**: Conectado em **D8** utilizando a interrupção por mudança de estado (`PCINT0` no Port B).
*   **Vazão C**: Conectado em **A2** utilizando a interrupção por mudança de estado (`PCINT10` no Port C).
*   *Nota: Todos são configurados como `INPUT_PULLUP` no firmware para evitar flutuações e ruídos na leitura do sensor Hall.*

### 5. Sensor de Temperatura da Água (DS18B20)
*   Opera em barramento OneWire no pino **D6**.
*   *Nota: Requer um resistor de pull-up físico de 4.7kΩ conectado entre a linha de sinal (D6) e o VCC chaveado (D7).*
