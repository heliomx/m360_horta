# Esquema Elétrico — Nó 04 (Clima Geral)

## Resumo do Hardware
- **Controlador**: Arduino Pro Mini (ATmega328P, 3.3V / 8MHz)
- **Rádio**: NRF24L01+ 
- **Alimentação Autônoma**: Painel Solar 6V/100mA + Módulo Carregador TP4056 + Bateria 18650 de Li-Ion.
- **Sensores**: 
  - DHT22 (Temperatura e Umidade)
  - LDR (Fotoresistor) em Divisor de Tensão com Resistor de 10kΩ

## Mapeamento de Pinos (Pinout)

### Rádio NRF24L01+
| NRF24L01+ | Arduino Pro Mini | Função |
| :--- | :--- | :--- |
| VCC | VCC (3.3V) | Alimentação da linha regulada do Pro Mini |
| GND | GND | Terra comum |
| CE | D9 | Chip Enable |
| CSN | D10 | Chip Select Not |
| SCK | D13 | SPI Clock |
| MOSI | D11 | SPI Master Out Slave In |
| MISO | D12 | SPI Master In Slave Out |
| IRQ | Não Conectado | Interrupção não usada (smartSleep por tempo) |

### Sensores e Controle de Energia (Mitigação Parasita)
Para garantir autonomia infinita da bateria suportada pelo pequeno painel solar, o hardware fica em _smartSleep()_ a maior parte do tempo. Durante o sleep, a energia dos sensores é 100% cortada utilizando o pino **D3** como "chave eletrônica" do VCC dos periféricos.

| Sensor | Pino de Sinal | Pino de Energia (VCC) | Pino de GND |
| :--- | :--- | :--- | :--- |
| **DHT22** | D4 | D3 (VCC Chaveado) | GND |
| **LDR** (Divisor) | A0 | D3 (VCC Chaveado) | GND |

> **Detalhe da Montagem do LDR (Divisor de Tensão)**: 
> - O LDR possui uma perna ligada ao pino **D3** e a outra ligada ao pino **A0**.
> - O Resistor de 10kΩ possui uma perna ligada ao pino **A0** e a outra ligada ao **GND**.

## Diagrama da Bateria e Painel Solar
O Arduino Pro Mini (modelo 3.3V) suporta tensões de entrada no pino **RAW** de até 12V. O regulador linear LDO (MIC5205 ou similar) embutido no Pro Mini converte a tensão flutuante da bateria de lítio (variando entre 3.0V e 4.2V) para exatos 3.3V.

```text
 [Painel Solar 6V] (+) ──> (IN+) [Módulo TP4056] (B+) ──> (+) [Bateria Li-Ion 18650]
                   (-) ──> (IN-)                 (B-) ──> (-) 
                                                 (OUT+) ──> [Pino RAW] (Pro Mini)
                                                 (OUT-) ──> [Pino GND] (Pro Mini)
```
*(Nota de Engenharia: O rádio NRF24L01+ pode drenar picos de 15mA a 115mA. A corrente será suprida pelo pino VCC do Pro Mini de 3.3V, que suporta até 150mA pelo LDO)*
