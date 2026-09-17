# Esquema Elétrico — Nó 4 (Clima Solar Mini)

## Resumo do Hardware
- **Controlador**: Arduino Pro Mini 5V/16MHz (ATmega328P, Deek-Robot) — alimentado pela bateria **direto no VCC**, com o LED de energia removido
- **Rádio**: nRF24L01+ com regulador de 3.3V dedicado (HT7333)
- **Alimentação**: Painel solar 6V → diodo 1N4007 → TP4056 com DW01 → 18650 (M360_LOW_POWER)
- **Sensores**: 
  - DHT11 (Sensor de Temperatura e Umidade do Ar)
  - DS18B20 (Sensor de Temperatura do Solo)

> **Placa identificada em 10/09/2026:** com o RAW alimentado, o VCC mede 5 V — é a
> versão de 5 V. As ilhas 8M/16M/3V3/5V do verso vieram sem marcação e o ressonador
> não tem valor impresso; o clock de 16 MHz é o do env `ProMini_04noodeSolarMini`
> (`board = pro16MHzatmega328`).

> **Autonomia:** consumo, geração do painel e margem estão em
> [`balanco_energetico.md`](balanco_energetico.md).

---

## Alimentação

```
Painel 6 V (+) ──►|──── IN+ ┐
             1N4007          │ TP4056 + DW01
Painel 6 V (−) ──────── IN− ┘
                             B+ / B− ─── 18650
                             OUT+ ──┬───────────── VCC  Pro Mini (RAW desligado)
                                    └── HT7333 ─── VCC  nRF24L01+ (3,3 V)
                             OUT− ──────────────── GND  comum
```

| Ponto | Por quê |
| :--- | :--- |
| Bateria no **VCC**, não no RAW | O regulador de 5 V da placa, com 3,0–4,2 V na entrada, não regula: fica em dropout, desconta a própria queda e ainda consome corrente quiescente. Pelo VCC a tensão da bateria chega inteira — e é isso que permite medir a bateria sem divisor (§3). |
| Diodo 1N4007 em série no IN+ | O máximo absoluto do TP4056 no VCC é **8 V**. O painel dá ~7,5 V em aberto a 25 °C e sobe ~0,3 %/°C no frio — justamente com a bateria cheia, quando o TP4056 não puxa corrente e a tensão em aberto vai direto ao pino. O diodo tira ~0,6 V. Como o carregador é linear, essa queda não reduz a carga: sai do calor que o chip dissiparia de qualquer jeito. De quebra, protege contra inversão de polaridade. |
| TP4056 **com** DW01 (saída OUT±) | O DW01 desliga a bateria em ~2,4 V. Sem ele, uma sequência de dias nublados descarrega a 18650 até estragá-la. Com o módulo simples (só IN± e BAT±), usar célula 18650 com proteção embutida. |
| R_PROG de fábrica (1,2 kΩ → 1 A) | Um painel de 1–2 W entrega bem menos que 1 A, então o TP4056 fica totalmente aberto e a tensão do painel cai para perto da bateria. Não é perda: num carregador linear a corrente de entrada é a corrente de carga, e perto de 4 V o painel entrega a maior corrente que consegue. Reduzir o R_PROG faria o chip regular e dissipar a sobra de tensão como calor. |
| HT7333 só para o rádio | O nRF24L01+ tem máximo absoluto de **3,6 V** e a bateria chega a 4,2 V. As entradas digitais do nRF24L01+ toleram 5 V, então CE/CSN/SCK/MOSI vindos do ATmega na tensão da bateria dispensam conversor de nível. |

### Modificações na placa Pro Mini
1. **Remover o LED de energia** — dessoldar o resistor `102` (1 kΩ) ao lado dele, logo
   abaixo do regulador. Aceso, o LED consome ~1,5–2 mA contínuos na tensão da bateria:
   10 a 20 vezes o consumo do nó dormindo. Sozinho, esgota uma 18650 de 2500 mAh em ~2 meses.
2. **(Opcional) Remover o regulador de 5 V** (SOT-23-5 ao lado da serigrafia "Pro mini").
   Com a bateria no VCC ele não tem função e fica alimentado pela saída. Só compensa
   depois do item 1.

### Clock de 16 MHz com bateria de lítio
O ATmega328P só é garantido a 16 MHz acima de ~3,8 V. Abaixo disso funciona na prática,
em temperatura ambiente, mas fora da especificação — e o brown-out da placa só atua em
2,7 V. Travamentos ou resets com a bateria baixa e a estufa quente apontam para cá. Na
próxima montagem, preferir o Pro Mini 3.3V/8MHz e trocar o `board` do env para
`pro8MHzatmega328`.

---

## Mapeamento de Pinos (Pinout)

### Rádio nRF24L01+
O rádio utiliza os pinos padrão da interface SPI do Arduino Pro Mini:

| nRF24L01+ | Arduino Pro Mini | Função |
| :--- | :--- | :--- |
| VCC | Saída do HT7333 (3.3V) | Alimentação regulada dedicada do rádio |
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

A bateria não usa pino: é lida pelo bandgap interno (§3). O **A0 fica livre**.

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

### 3. Medição da Bateria (sem divisor)
*   **Funcionamento:** com a bateria no VCC, **VCC = Vbat**. O nó não sobrescreve `M360::readBatteryVoltage()`, e o padrão da lib (`lib/M360-DRY/src/M360Power.cpp`) mede o VCC contra o bandgap interno de 1,1 V: `Vcc = 1,1 × 1023 / leitura`. Nenhum componente externo, nenhum pino.
*   **Por que o divisor 100k/100k saiu:** o código antigo convertia a leitura do A0 como se o AVcc fosse 3,3 V fixo. Com a bateria no VCC, o AVcc é a própria bateria: o divisor entrega Vbat/2 contra uma referência de ≈ Vbat, a leitura crua para em ~511 e o valor reportado travava em ~3,3 V com qualquer carga. O divisor ainda drenava ~20 µA contínuos.
*   **Premissa:** a leitura só vale com a bateria no VCC. Alimentado pelo RAW, o valor sai baixo pela queda do regulador.
*   **Precisão:** o bandgap varia de 1,0 a 1,2 V entre chips (±10 %). Se precisar de precisão, medir o VCC com multímetro e sobrescrever `readBatteryVoltage()` no nó com o mesmo cálculo e a constante calibrada: `1125300 × V_multímetro / V_reportado`.
