# Lista de Materiais (BOM) — Nó 99 (NodeReles)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 99 (NodeReles)**, responsável por controlar atuadores nativos (circulação e oxigenação NFT) e multiplexados (válvulas solenoides e dosadores peristálticos) no quadro central M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Nano (5V / 16MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Multiplexador** | Módulo MUX Analógico/Digital de 16 canais | 1 | CD74HC4067 |
| 4 | **Módulos de Relé** | Módulos de relés optoacoplados de alta potência (Active LOW) | 9 | Módulos Relé 5V DC / 10A SPDT |
| 5 | **Sensor de Vazão** | Sensor de vazão de água por efeito Hall para a horta | 1 | YF-S201 (rosca 1/2") |
| 6 | **Sensor de Quadro** | Sensor de temperatura e umidade para monitorar o quadro elétrico | 1 | DHT11 |

---

## 2. Componentes de Alimentação e Regulação

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 7 | **Fonte Principal** | Fonte chaveada 12V DC para alimentação dos atuadores de 12V | 1 | Fonte chaveada 12V DC / 5A a 10A |
| 8 | **Redutor 5V** | Conversor Buck Step-down para fornecer 5V DC limpos | 1 | Módulo Step-Down LM2596 ou regulador fixo |
| 9 | **Redutor 3.3V** | Regulador de tensão LDO linear para fornecer 3.3V ao rádio | 1 | AMS1117-3.3 ou similar |

---

## 3. Atuadores (Cargas Acopladas)

| Item | Componente | Descrição | Qtd | Especificação de Alimentação |
| :---: | :--- | :--- | :---: | :--- |
| 10 | **Bomba NFT** | Bomba centrífuga de circulação de água para calhas NFT | 1 | Bomba de água submersa 220V AC |
| 11 | **Bomba de Oxi** | Compressor/bomba de ar para oxigenação do reservatório | 1 | Bomba de aeração 220V AC |
| 12 | **Válvulas Solenoides** | Válvulas para abertura automática da irrigação dos canteiros A, M e C | 3 | Válvula Solenoide 12V DC / ~0.5A |
| 13 | **Peristálticas** | Mini bombas dosadoras de suplementação (Nutrientes e pH) | 4 | Motores peristálticos 12V DC / ~0.5A |

---

## 4. Componentes Passivos e Conectores

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 14 | **Resistores (Pull-Up)** | Resistores de pull-up (1 para DHT11, 1 para vazão e MUX se necessário) | 2 | Resistor $10\text{ k}\Omega$ ou $4.7\text{ k}\Omega$ (1/4W, 5%) |
| 15 | **Capacitor (Filtro)** | Capacitor eletrolítico colocado no VCC/GND do rádio | 1 | Capacitor $100\,\mu\text{F}$ (16V+) |
| 16 | **Barramento e Bornes** | Terminais modulares de parafuso para conexões seguras no quadro | - | Bornes KRE passo 5.08mm / Trilho DIN |
