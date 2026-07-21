# Lista de Materiais (BOM) — Gateway MQTT (M360-DRY)

Este documento descreve todos os componentes necessários para a montagem física do **Gateway MQTT**, responsável por receber os pacotes via rádio NRF24L01+ e encaminhá-los via Wi-Fi para o Broker MQTT.

---

## 1. Componentes Principais

| Item | Componente | Descrição | Qtd | Referência / Modelo |
| :---: | :--- | :--- | :---: | :--- |
| 1 | **Placa de Controle** | Microcontrolador com Wi-Fi integrado (ESP8266) | 1 | NodeMCU ESP8266 ou Wemos D1 Mini |
| 2 | **Módulo de Rádio** | Transceptor de Rádio Frequência 2.4 GHz | 1 | nRF24L01+ |

---

## 2. Componentes de Interface e Sinalização

| Item | Componente | Descrição | Qtd | Referência / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 3 | **LED Verde** | Sinalização de status: Conectado com sucesso ao Wi-Fi e MQTT | 1 | LED Difuso 5mm |
| 4 | **LED Amarelo** | Sinalização de status: Envio/recebimento de mensagens MQTT | 1 | LED Difuso 5mm |
| 5 | **LED Vermelho** | Sinalização de status: Erro de rede ou Modo Access Point (AP) ativo | 1 | LED Difuso 5mm |
| 6 | **Botão (Push Button)** | Botão de reset de credenciais (Modo AP) no boot | 1 | Push Button Tact Switch 6x6x5mm |

---

## 3. Componentes Passivos e Acessórios

| Item | Componente | Descrição | Qtd | Valor / Especificação |
| :---: | :--- | :--- | :---: | :--- |
| 7 | **Resistores (LEDs)** | Resistores de limitação de corrente para os LEDs de sinalização | 3 | Resistor $330\text{ }\Omega$ ou $220\text{ }\Omega$ (1/4W, 5%) |
| 8 | **Fonte de Alimentação** | Adaptador de tomada USB para alimentação contínua do gateway | 1 | Fonte USB 5V DC / 1A a 2A |
| 9 | **Cabo de Alimentação** | Cabo para alimentação e programação do microcontrolador | 1 | Cabo Micro-USB ou USB-C (conforme a placa de controle) |
| 10 | **Regulador de Tensão (Opcional)** | Adaptador regulador de tensão exclusivo para o rádio se houver instabilidade | 1 | Placa adaptadora para nRF24L01 com LDO 3.3V integrado |
| 11 | **Protoboard / PCI** | Placa de ensaio ou placa de circuito impresso universal para soldagem | 1 | Placa perfurada de fenolite ou Protoboard |
