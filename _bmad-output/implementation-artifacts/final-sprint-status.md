# Relatório Final de Sprint: Nó de Monitoramento de Horta (m360_horta)

**Data:** 2026-04-01
**Status do Projeto:** ✅ CONCLUÍDO (Pronto para Upload)
**Arquiteto:** Antigravity (IA)
**Cliente:** Marcelo

## 🎯 Escopo do Sprint
O objetivo deste sprint foi criar o firmware para um nó MySensors (Arduino Pro Mini) capaz de monitorar temperatura/umidade do ar, temperatura do solo e umidade redundante do solo, com foco em extrema economia de energia e compatibilidade com o gateway MQTT Manejo360.

## 📦 Entregas Realizadas

### 1. Camada de Abstração de Hardware (`sensorDrivers.h/cpp`)
- Implementação de drivers reais para **DHT11** (D2) e **DS18B20** (D3).
- Algoritmo de mapeamento de umidade de solo (A0/A1) convertendo raw (1023-300) para escala percentual (0-100%).
- Função de autodiagnóstico (`isnan`) para evitar envio de dados lixo ao gateway.

### 2. Gestão de Energia Avançada (Power Management)
- **VCC Chaveado (D4):** Sensores só ligam durante a leitura, economizando bateria e protegendo contra corrosão.
- **Deep Sleep Progressivo:** Nó entra em sono profundo via `sleep()` após o envio.
- **Monitoramento de Bateria:** Implementado reporte automático de porcentagem baseado na tensão real de entrada.

### 3. Integração e Protocolo
- Compatibilidade total com `newGatewayMqtt.cpp`.
- Suporte a comandos remotos: **FORCE_UPDATE** e **SET_INTERVAL** (via rádio).
- Implementação robusta seguindo as "Best Practices" da MySensors V2.

## 📐 Pinagem Final do Protótipo
| Sensor | Pino Arduino | Função |
| :--- | :--- | :--- |
| **DHT11** | D2 | Dados (Ar) |
| **DS18B20** | D3 | Dados (Solo) |
| **VCC Aux** | D4 | Alimentação dos Sensores |
| **Moisture 1** | A0 | Umidade Solo A |
| **Moisture 2** | A1 | Umidade Solo B |
| **RF24 CE** | D9 | Rádio |
| **RF24 CSN** | D10 | Rádio |

## 🚀 Recomendações Pós-Sprint
- **Calibração:** Se os sensores de solo forem de marcas diferentes, os valores de `SOIL_DRY` (1023) e `SOIL_WET` (300) no `sensorDrivers.cpp` podem precisar de ajuste fino.
- **Vedação:** Garantir que o DS18B20 e os sensores de solo estejam bem protegidos contra infiltração de água.

## 🏁 Encerramento
O firmware atingiu todos os critérios de aceitação e está alinhado à arquitetura DRY do projeto M360.

---
*Assinado: Antigravity - BMAD Dev Team*
