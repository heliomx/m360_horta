# Bibliotecas do Projeto M360 Horta

Este diretório contém as bibliotecas personalizadas desenvolvidas para a rede de sensores M360 Horta.

## M360-DRY (LibDRY) 🌿

A biblioteca core que padroniza o comportamento dos nós MySensors, separando a lógica de comunicação do hardware físico.

- [**README da LibDRY**](M360-DRY/README.md)
- [Arquitetura](M360-DRY/ARCHITECTURE.md)
- [Referência da API](M360-DRY/API_REFERENCE.md)

## SU-xxT — Solo e Lisímetro Cerâmico 🌱

Camada física da família de sensores SU-xxT: umidade em duas profundidades,
temperatura da rizosfera, e pH / EC / nível da solução extraída por lisímetro
cerâmico. Orquestra ADS1115, MUX 74HC4051, DS18B20 e as compensações agronômicas.

> **Status:** implementada e compilando em AVR, ESP8266 e ESP32 — ainda **não
> executada em hardware**. Os defaults de calibração são teóricos, não medidos.

- [**README da SU-xxT**](SU-xxT/README.md)
- [Arquitetura e decisões de projeto](SU-xxT/ARCHITECTURE.md)
- [Referência da API](SU-xxT/API_REFERENCE.md)
- [Especificação de hardware](../hardware/SU-xxT/README.md)

---
*Documentação da LibDRY gerada autonomamente pela Paige.*


