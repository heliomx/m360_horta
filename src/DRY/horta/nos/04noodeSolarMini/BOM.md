# Lista de Materiais (BOM) — Nó 4 (Monitoramento Solar de Clima)

Este documento lista todos os componentes de hardware necessários para a montagem do **Nó 04 (SolarMini)**, incluindo o circuito de carga solar e as modificações de baixo consumo na placa. Ligações e justificativas em [`esquema_eletrico.md`](esquema_eletrico.md); dimensionamento de painel e bateria em [`balanco_energetico.md`](balanco_energetico.md).

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa ATmega328P 5V/16MHz, alimentada pela bateria direto no VCC (ver §4) | 1 | Arduino Pro Mini 5V/16MHz (Deek-Robot) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz de baixo consumo | 1 | nRF24L01+ (Antena integrada) |
| 3 | **Sensor de Clima** | Sensor digital de temperatura e umidade relativa | 1 | DHT11 |
| 4 | **Sensor Temp. Solo** | Sensor digital de temperatura para solo/líquidos (à prova d'água) | 1 | DS18B20 (Sonda metálica com cabo) |

---

## 2. Circuito de Alimentação e Energia

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 5 | **Carregador de Bateria** | Carregador linear de Li-ion com proteção contra sobredescarga (saída OUT±). Manter o R_PROG de fábrica (1,2 kΩ → 1 A) | 1 | Módulo TP4056 + DW01 + 8205A |
| 6 | **Bateria** | Célula Li-ion 3.7V (nominal) / 4.2V (máx.). Com proteção embutida se o carregador não tiver DW01 | 1 | Célula 18650 Li-ion |
| 7 | **Painel Solar** | Placa fotovoltaica 6V nominal, ~7,5V em aberto | 1 | Painel Solar 6V (1W a 2W) |
| 8 | **Diodo de Entrada** | Em série no IN+ do TP4056: mantém a tensão do painel abaixo dos 8V máximos do TP4056 e bloqueia inversão de polaridade | 1 | 1N4007 |
| 9 | **Regulador do Rádio** | LDO de baixa corrente quiescente que alimenta **somente** o nRF24L01+ (máx. 3.6V) a partir da bateria | 1 | HT7333-A ou MCP1700-3302E/TO |

---

## 3. Componentes Eletrônicos Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 10 | **Resistor (Pull-Up DHT)** | Resistor de pull-up para o barramento de dados (D4) do DHT11 | 1 | Resistor 10 kΩ (1/4W, 5%) |
| 11 | **Resistor (Pull-Up OneWire)** | Resistor de pull-up para o barramento de dados (D5) do DS18B20 | 1 | Resistor 4.7 kΩ (1/4W, 5%) |
| 12 | **Capacitor (Filtro)** | Capacitor de desacoplamento de alimentação próximo ao rádio nRF24L01+ | 1 | Capacitor Eletrolítico 10 µF a 47 µF (16V+) |
| 13 | **Conectores** | Barra de pinos e cabos jumper para conexões internas | - | Pinos modulares passo 2.54mm |

> **Removidos nesta revisão:** o divisor 100k/100k do A0 e o capacitor de 100 nF do ADC.
> A bateria agora é lida pelo bandgap interno — ver `esquema_eletrico.md` §3.

---

## 4. Modificações na Placa Pro Mini

| Modificação | Como | Por quê |
| :--- | :--- | :--- |
| **Remover o LED de energia** | Dessoldar o resistor `102` (1 kΩ) ao lado do LED, abaixo do regulador | ~1,5–2 mA contínuos: sozinho, esgota a 18650 em ~2 meses |
| **Bateria no VCC, RAW desligado** | OUT+ do TP4056 no pino VCC | O regulador de 5V não regula com 3,0–4,2V na entrada, e a medição da bateria depende de VCC = Vbat |
| (Opcional) Remover o regulador de 5V | SOT-23-5 ao lado da serigrafia "Pro mini" | Sem função com a bateria no VCC; ganho pequeno, só depois do LED |
