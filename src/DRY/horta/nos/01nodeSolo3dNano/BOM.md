# Lista de Materiais (BOM) — Nó 1 (Solo3dNano)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 01 (Solo3dNano)**, versão Arduino Nano para monitoramento de solo com 6 sensores diretos no sistema M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Nano (5V / 16MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Sensores** | Eletrodos de aço inox / placas para medição resistiva no solo | 6 | Sensores resistivos de umidade de solo |

---

## 2. Componentes Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 4 | **Resistores (Pull-Up)** | Resistores de pull-up para os barramentos analógicos de medição | 6 | Resistor $10\text{ k}\Omega$ (1/4W, 1% de precisão recomendado) |
| 5 | **Fonte de Alimentação** | Fonte de alimentação contínua bivolt para o nó | 1 | Fonte chaveada 5V DC / 1A a 2A |
| 6 | **Capacitor (Filtro)** | Capacitor eletrolítico para filtro de alimentação do rádio | 1 | Capacitor $10\,\mu\text{F}$ a $47\,\mu\text{F}$ (16V+) |
| 7 | **Fiação** | Fiação e cabos de sinal para conexão das pontas no canteiro | - | Cabos AWG 24 / 22 |
