# Lista de Materiais (BOM) — miniDHT (Nó 11)

**Projeto:** M360 Horta — Sub-projeto Kit Hélio  
**Aplicação:** Sensor Clima / Temperatura e Umidade do Ar  

---

## Componentes Eletrônicos e Módulos

| Item | Componente / Módulo | Descrição / Função | Qtd | Especificações Técnicas |
|---|---|---|---|---|
| 1 | **Microcontrolador** | Arduino Pro Mini | 1 | ATmega328P, 5V DC, 16MHz |
| 2 | **Transceptor RF** | Módulo nRF24L01+ | 1 | 2.4GHz, SPI, Antena PCB |
| 3 | **Sensor de Temperatura/Umidade** | Módulo ou Sensor DHT11 | 1 | Medição 0-50°C (±2°C), 20-90% RH (±5%) |
| 4 | **Resistor Pull-Up** | Resistor 10k Ohm | 1 | 1/4W, 5% (entre VCC e Sinal de Dados D4) |
| 5 | **Regulador de Tensão (opcional)** | Módulo Regulador LDO 3.3V | 1 | Regulador de 3.3V dedicado para alimentação do nRF24L01+ |
| 6 | **Fonte de Alimentação** | Fonte Chaveada 5V DC | 1 | Entrada 100-240V AC, Saída 5V 1A DC |

---

## Conexões e Suporte

| Item | Descrição | Qtd | Observação |
|---|---|---|---|
| 7 | **Protoboard / Placa Perfurada** | 1 | Para montagem do protótipo |
| 8 | **Jumpers Dupont (F-F / M-F)** | 10 | Conexão entre o Arduino, nRF24L01+ e DHT11 |
