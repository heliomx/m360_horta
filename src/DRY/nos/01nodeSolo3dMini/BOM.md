# Lista de Materiais (BOM) — Nó 1 (Solo3dMini)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 01 (Solo3dMini)**, responsável pelo monitoramento 3D de umidade do solo com 18 sensores de canteiro no sistema M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Pro Mini (5V / 16MHz ou 3.3V / 8MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Multiplexador** | Módulo MUX Analógico de 16 canais | 1 | CD74HC4067 |
| 4 | **Sensores** | Eletrodos de aço inox / placas para medição resistiva no solo | 18 | Sensores resistivos de umidade de solo |

---

## 2. Componentes Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 5 | **Resistores (Pull-Up)** | Resistores de pull-up para os barramentos analógicos de medição | 3 | Resistor $10\text{ k}\Omega$ (1/4W, 1% de precisão recomendado) |
| 6 | **Fonte de Alimentação** | Fonte de alimentação contínua bivolt para o nó | 1 | Fonte chaveada 5V DC / 1A a 2A |
| 7 | **Capacitor (Filtro)** | Capacitor eletrolítico para filtro de alimentação do rádio | 1 | Capacitor $10\,\mu\text{F}$ a $47\,\mu\text{F}$ (16V+) |
| 8 | **Conectores** | Barra de bornes ou terminais modulares para cabo dos sensores | 18 | Bornes de parafuso passo 3.5mm ou 5.08mm |
| 9 | **Fiação** | Cabos blindados de sinal (par trançado recomendado) para os canteiros | - | Cabos AWG 24 / 22 |
