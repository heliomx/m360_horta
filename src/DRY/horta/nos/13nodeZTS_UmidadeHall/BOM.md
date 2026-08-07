# Lista de Materiais (BOM) — Nó 13 (ZTS_UmidadeHall)

Este documento descreve todos os componentes necessários para a montagem física do **Nó 13 (ZTS_UmidadeHall)**, responsável por ler o sensor modbus ZTS-3002, sensores locais (Hall e LDR) e acionar a válvula solenoide no sistema M360.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador** | Placa de desenvolvimento com microcontrolador ATmega328P | 1 | Arduino Pro Mini (3.3V / 8MHz) |
| 2 | **Transceptor de Rádio** | Módulo de comunicação RF 2.4 GHz | 1 | nRF24L01+ |
| 3 | **Sensor de Solo Modbus** | Sensor de umidade, temperatura, condutividade, pH e NPK | 1 | ZTS-3002 |
| 4 | **Conversor TTL-RS485** | Módulo conversor de sinal serial TTL para barramento RS485 | 1 | Módulo MAX485 |
| 5 | **Sensor de Umidade local** | Sensor de umidade de solo baseado em circuito oscilador Hall | 1 | Sensor de Umidade Hall |
| 6 | **Sensor de Luminosidade** | Sensor de luz ambiente LDR | 1 | LDR 5mm |
| 7 | **Módulo Relé** | Módulo de relé optoacoplado para chaveamento de 12V (solenoide) | 1 | Módulo Relé 5V / 10A (Active Low) |
| 8 | **Válvula** | Válvula solenoide para liberação de fluxo de água de irrigação | 1 | Solenoide 12V DC (Normalmente Fechada) |

---

## 2. Componentes de Alimentação e Acessórios

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 9 | **Fonte de Alimentação** | Fonte chaveada 12V para relés/solenoide e sensor ZTS | 1 | Fonte chaveada 12V DC / 2A |
| 10 | **Regulador de Tensão** | Regulador de tensão LDO para alimentar microcontrolador e rádio | 1 | Regulador HT7333-A ou MCP1700 (3.3V) |
| 11 | **Resistor (Pull-Up)** | Resistor de pull-up para leitura analógica estável do LDR | 1 | Resistor $10\text{ k}\Omega$ (1/4W, 5%) |
| 12 | **Capacitor (Filtro)** | Capacitor eletrolítico para estabilização de VCC do rádio | 1 | Capacitor $47\,\mu\text{F}$ (16V+) |
| 13 | **Cabos / Bornes** | Fiação modbus blindada e conectores modulares | - | Bornes KRE / Cabo blindado RS485 |
