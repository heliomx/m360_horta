# Diagrama de Blocos — Nó 05 (Monitoramento 3D de Solo Unificado)

Este arquivo descreve a arquitetura de blocos funcionais do **Nó 05 (05nodeSolo3dNanoMux)**, detalhando o fluxo de alimentação, sinais de controle e leitura analógica.

## Arquitetura de Blocos (Mermaid)

```mermaid
graph TD
    %% Fontes de Alimentação e Distribuição
    Fonte[Fonte 5V DC Regulada] -->|5V DC| Nano[Arduino Nano ATmega328P]
    Fonte -->|5V DC| MUX[Multiplexador CD74HC4067]
    Fonte -->|5V DC| AdaptadorRadio[Módulo Adaptador de Soquete]
    AdaptadorRadio -->|Regulador LDO 3.3V| NRF[Transceptor RF24L01+]

    %% Controle e Sinais (MCU -> Rádio / MUX)
    Nano -->|D9 CE, D10 CSN| NRF
    Nano -->|D11 MOSI, D12 MISO, D13 SCK| NRF
    Nano -->|D4..D7 Seleção S0..S3| MUX
    Nano -->|D3 PIN_POWER_SENSORS| BarraPullUp[Barra de Resistores Pull-Up 10k]
    
    %% Barra de Energia de Pull-up (Mitigação de Eletrólise)
    BarraPullUp -->|1 Resistor 10k Central| A0[Pino A0 - Mux SIG]
    BarraPullUp -->|1 Resistor 10k Dedicado| A1[Pino A1 - Canal Nativo 16]
    BarraPullUp -->|1 Resistor 10k Dedicado| A2[Pino A2 - Canal Nativo 17]

    %% Sinais Analógicos dos Sensores
    MUX -->|Canal Comum SIG| A0
    MUX -->|Canais C0..C15| SensoresMUX[Sensores de Solo 0 a 15<br/>Canteiro A: 0..8<br/>Canteiro B: 9..15]
    
    A1 -->|Eletrodo Positivo| Sensor16[Sensor de Solo 16<br/>Canteiro B: 5m 20cm]
    A2 -->|Eletrodo Positivo| Sensor17[Sensor de Solo 17<br/>Canteiro B: 5m 30cm]

    %% Referência de Terra Comum
    SensoresMUX -->|Eletrodo Negativo| GND[Terra Comum GND]
    Sensor16 -->|Eletrodo Negativo| GND
    Sensor17 -->|Eletrodo Negativo| GND
    Nano --> GND
    MUX --> GND
    NRF --> GND
    Fonte --> GND

    %% Estilos e Cores
    classDef power fill:#fff8e1,stroke:#f9a825,stroke-width:2px;
    classDef mcu fill:#e3f0ff,stroke:#0055aa,stroke-width:2px;
    classDef radio fill:#e8f5e9,stroke:#1a7a3a,stroke-width:2px;
    classDef signal fill:#fff3cd,stroke:#c77a00,stroke-width:2px;
    classDef sensor fill:#e0f2f1,stroke:#00796b,stroke-width:2px;
    classDef mux fill:#f3e5f5,stroke:#6f42c1,stroke-width:2px;
    
    class Fonte,AdaptadorRadio,BarraPullUp,GND power;
    class Nano mcu;
    class NRF radio;
    class MUX mux;
    class A0,A1,A2 signal;
    class SensoresMUX,Sensor16,Sensor17 sensor;
```

---

## Descrição dos Blocos

1. **Alimentação:** A fonte primária de 5V alimenta diretamente o Arduino Nano e o circuito integrado multiplexador CD74HC4067. Para o rádio NRF24L01+, é empregado um módulo adaptador de soquete contendo um regulador de tensão linear (LDO) de 3.3V (AMS1117-3.3) para reduzir e estabilizar a tensão, eliminando ruídos da linha que degradam o tráfego RF.
2. **Mitigação de Eletrólise (Pino D3):** A alimentação dos resistores de pull-up é chaveada digitalmente pelo pino **D3** (`PIN_POWER_SENSORS`). Em repouso (entre leituras), este pino permanece em `LOW` (0V), extinguindo correntes galvânicas no solo para prolongar a vida útil dos eletrodos de aço inox dos sensores.
3. **Barra de Divisores de Tensão:**
   * **Canais do MUX (0 a 15):** Utiliza-se a configuração de pull-up único centralizado. Um único resistor de **10kΩ** conecta o barramento de alimentação de sensores (pino **D3**) diretamente ao pino comum de sinal do MUX (**A0/SIG**).
   * **Canais Nativos (16 e 17):** Como estes sinais não passam pelo chaveador do MUX, eles possuem resistores individuais de **10kΩ** ligados entre o pino **D3** e suas respectivas portas (**A1** e **A2**).
4. **Comutação Analógica:** O chip CD74HC4067 atua como uma chave seletora analógica de 16 posições. A MCU seleciona qual sensor do barramento deseja ler aplicando o código binário do canal (0 a 15) nos pinos seletores `S0-S3`. O sinal do sensor correspondente é fechado com o pino `SIG`, gerando a leitura do divisor de tensão através da porta analógica `A0`.
