# Lista de Materiais (BOM) — Nó 4 (Monitoramento Solar de Clima)

Este documento lista todos os componentes de hardware necessários para a montagem do **Nó 04 (SolarMini)**, incluindo o circuito de baixo consumo e o divisor de medição de bateria.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento compacta ATmega328P (3.3V / 8MHz) | 1 | Arduino Pro Mini 3.3V |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz de baixo consumo | 1 | nRF24L01+ (Antena integrada) |
| 3 | **Sensor de Clima** | Sensor digital de temperatura e umidade relativa | 1 | DHT11 |

---

## 2. Circuito de Alimentação e Energia

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 4 | **Carregador de Bateria** | Shield com suporte para célula 18650, carregador USB-C e proteção | 1 | 18650 Battery Shield V3 (TP4056 + DW01) |
| 5 | **Bateria** | Célula recarregável de íons de lítio 3.7V (nominal) / 4.2V (máx) | 1 | Célula 18650 Li-ion |
| 6 | **Painel Solar** | Placa solar fotovoltaica para recarga externa da bateria | 1 | Painel Solar 6V (1W a 2W) |
| 7 | **Regulador de Tensão** | Regulador de baixa queda de tensão (LDO) para VCC estável de 3.3V | 1 | HT7333-A ou MCP1700-3302E/TO |

---

## 3. Componentes Eletrônicos Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 8 | **Resistor (Pull-Up)** | Resistor de pull-up para o barramento de dados (D4) do DHT11 | 1 | Resistor $10\text{ k}\Omega$ (1/4W, 5%) |
| 9 | **Resistores (Divisor)** | Divisor de tensão resistivo (2x) para leitura de bateria no pino `A0` | 2 | Resistor $100\text{ k}\Omega$ (1/4W, 1% de precisão recomendado) |
| 10 | **Capacitor (Filtro)** | Capacitor de desacoplamento de alimentação próximo ao rádio nRF24L01+ | 1 | Capacitor Eletrolítico $10\,\mu\text{F}$ a $47\,\mu\text{F}$ (16V+) |
| 11 | **Capacitor (Estabilizador)** | Capacitor em paralelo com resistor do divisor para estabilizar o ADC | 1 | Capacitor Cerâmico $100\text{ nF}$ (opcional) |
| 12 | **Conectores** | Barra de pinos e cabos jumper para conexões internas | - | Pinos modulares passo 2.54mm |
