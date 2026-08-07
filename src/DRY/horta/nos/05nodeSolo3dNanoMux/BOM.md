# Lista de Materiais (BOM) — Nó 5 (Solo3dNanoMux)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 05 (Solo3dNanoMux)**, versão unificada baseada em Arduino Nano e Multiplexador para monitoramento de 18 sensores de solo (Canteiros A e B) no sistema M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Nano (5V / 16MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Adaptador de Rádio** | Soquete adaptador para rádio RF24 com regulador LDO 3.3V integrado | 1 | Adaptador de Soquete com AMS1117-3.3 |
| 4 | **Multiplexador** | Módulo MUX Analógico de 16 canais | 1 | CD74HC4067 |
| 5 | **Sensores** | Eletrodos de aço inox / placas para medição resistiva no solo | 18 | Sensores resistivos de umidade de solo |

---

## 2. Componentes Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 6 | **Resistores (Pull-Up)** | Resistores de pull-up para as medições analógicas (1 no SIG MUX, 2 nos pinos nativos) | 3 | Resistor $10\text{ k}\Omega$ (1/4W, 1% de precisão recomendado) |
| 7 | **Fonte de Alimentação** | Fonte de alimentação contínua bivolt para o nó | 1 | Fonte chaveada 5V DC / 1A a 2A |
| 8 | **Conectores** | Barra de bornes ou terminais modulares para cabo dos sensores | 18 | Bornes de parafuso passo 3.5mm ou 5.08mm |
| 9 | **Fiação** | Cabos blindados de sinal (par trançado recomendado) para os canteiros | - | Cabos AWG 24 / 22 |
