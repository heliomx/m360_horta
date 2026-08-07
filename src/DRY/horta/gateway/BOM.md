# Lista de Materiais (BOM) — Gateway MQTT (M360-DRY)

Este documento descreve todos os componentes necessários para a montagem física do **Gateway MQTT**, responsável por receber os pacotes via rádio nRF24L01+ e encaminhá-los via Wi-Fi para o Broker MQTT.

---

## 1. Componentes Principais e Módulos

| Item | Componente | Descrição / Função | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Microcontrolador (U1)** | Módulo ESP8266 com Wi-Fi (Girado 180° no layout para otimização SPI) | 1 | Wemos D1 Mini (ESP8266) |
| 2 | **Módulo de Rádio** | Transceptor RF 2.4 GHz (nRF24L01+ padrão ou PA/LNA com antena externa) | 1 | nRF24L01+ (≈15,3 × 29 mm) |
| 3 | **Regulador LDO Dedicado** | Regulador de tensão 3.3V exclusivo para a linha `+3V3_RF` (Picos > 100 mA) | 1 | AMS1117-3.3V (SOT-223 ou Módulo THT) |

---

## 2. Componentes de Interface e Sinalização Painel

| Item | Componente | Descrição / Posição no Painel | Qtd | Referência / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 4 | **LED Vermelho** | Status 1: Erro de Rede / Modo Access Point (AP) Ativo (Conectado a D2) | 1 | LED Difuso 5mm |
| 5 | **LED Verde** | Status 2: Sistema Conectado e Operativo (Wi-Fi + MQTT OK) (Conectado a D1) | 1 | LED Difuso 5mm |
| 6 | **LED Amarelo** | Status 3: Atividade MQTT / Heartbeat / Inicialização (Conectado a D0) | 1 | LED Difuso 5mm |
| 7 | **Botão (SW1)** | Botão de Manutenção (Modo AP / Factory Reset no Boot ligado a A0) | 1 | Push Button Tact Switch 6x6x5mm |

---

## 3. Componentes Passivos, Filtragem e Acessórios

| Item | Componente | Descrição / Função | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 8 | **Resistor R4 (A0 Pull-up)** | Pull-up OBRIGATÓRIO de A0 para 3V3 (evita ADC ≈ 0 em repouso) | 1 | Resistor 10 kΩ (1/4W, 5% THT) |
| 9 | **Resistores (LEDs)** | Resistores de limitação de corrente para os LEDs de sinalização | 3 | Resistor 330 Ω ou 220 Ω (1/4W, 5% THT) |
| 10 | **Capacitores de Desacoplamento** | Filtragem de ruído na linha `+3V3_RF` do rádio nRF24 | 2 | 1x 10 µF Eletrolítico + 1x 100 nF Cerâmico |
| 11 | **Soquetes Fêmea (U1)** | Soquetes para encaixe e remoção do Wemos D1 Mini | 2 | Barra de Soquete Fêmea 1x8 pinos (Passo 2.54 mm) |
| 12 | **Soquete Fêmea (Rádio)** | Soquete para encaixe do módulo nRF24L01+ | 1 | Conector Fêmea 2x4 pinos (Passo 2.54 mm) |
| 13 | **Placa de Circuito Impresso (PCI)** | PCI customizada de 2 camadas com plano de terra (GND) e recorte na antena | 1 | Placa PCI 100 × 70 mm (FR4, 2 Camadas) |
| 14 | **Fonte de Alimentação** | Fonte externa contínua USB para alimentação do gateway | 1 | Fonte USB 5V DC / 1A a 2A |

---

## 4. Notas de Omissão de Componentes
* **Botão/Resistor de Reset (RST):** Omitidos deliberadamente da placa-mãe. O D1 Mini já possui pull-up e botão de reset embutidos em seu PCB, acessíveis por cima com o módulo em soquete. Pino RST marcado como No-Connect (NC).
* **Resistor de Pull-up no Pino CE:** Omitido deliberadamente, pois o D1 Mini gerencia o sinal CE diretamente via firmware.

