# Lista de Materiais (BOM) — Nó 80 (Caixa D'água, pH, EC e Vazão)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 80 (Aqua)**, responsável pelo monitoramento do nível do reservatório, sensores físico-químicos (pH e condutividade elétrica) e medição de vazão e fluxo de água no sistema M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Nano (5V / 16MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Sensor de Nível** | Sensor de distância por ultrassom para medir nível do reservatório | 1 | HC-SR04 |
| 4 | **Sensor de pH** | Eletrodo de pH com placa condicionadora de sinal analógico | 1 | pH-4502C (Eletrodo + Placa) |
| 5 | **Sensor de EC** | Placa de medição de Condutividade Elétrica com sonda de metal | 1 | Módulo EC Sensor |
| 6 | **Sensores de Vazão** | Sensores de fluxo de água de efeito Hall | 4 | YF-S201 (Conexão roscável 1/2") |
| 7 | **Sensor de Temp.** | Sensor digital de temperatura para líquidos (à prova d'água) | 1 | DS18B20 (Sonda metálica com cabo) |

---

## 2. Componentes Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 8 | **Resistor (Pull-Up)** | Resistor de pull-up para barramento digital OneWire do DS18B20 | 1 | Resistor $4.7\text{ k}\Omega$ (1/4W, 5%) |
| 9 | **Fonte de Alimentação** | Fonte de alimentação contínua para o nó (geralmente do painel central) | 1 | Fonte chaveada 5V DC / 2A |
| 10 | **Capacitor (Filtro)** | Capacitor eletrolítico para estabilização de VCC do rádio | 1 | Capacitor $47\,\mu\text{F}$ (16V+) |
| 11 | **Placa e Bornes** | Bornes para conexão de sensores e montagem da placa central | - | Bornes KRE / Barra de pinos modulares |
