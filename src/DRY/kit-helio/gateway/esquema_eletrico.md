# Esquema Elétrico — Gateway MQTT (M360-DRY)

O diagrama a seguir detalha as conexões físicas e lógicas do **Gateway MQTT**, responsável por fazer a ponte de comunicação entre os nós sensores/atuadores da rede MySensors (via rádio nRF24L01+) e o servidor central M360 (via Wi-Fi e MQTT).

---

## Diagrama de Conexões (Mermaid)

```mermaid
graph TD
    %% Fonte de Alimentação
    Fonte[Fonte 5V DC]
    
    %% Regulador Dedicado
    LDO[Regulador LDO AMS1117-3.3V<br/>+ Capacitores 10uF / 100nF]
    
    %% Microcontrolador e Rádio
    Wemos[ESP8266 Wemos D1 Mini<br/>U1 Girado 180°]
    NRF[Módulo nRF24L01+<br/>nRF24 + PA/LNA]
    
    %% Entradas e Saídas
    R4[Resistor R4 10k<br/>Pull-Up Obrigatório A0]
    BtnSW1[Botão SW1<br/>Modo AP / Reset Config]
    
    LedVermelho[LED Vermelho (1º)<br/>D2 / GPIO4 - Erro / AP]
    LedVerde[LED Verde (2º)<br/>D1 / GPIO5 - Wi-Fi/MQTT OK]
    LedAmarelo[LED Amarelo (3º)<br/>D0 / GPIO16 - TX/RX MQTT]
    
    %% Nuvem / Rede
    Router[Roteador Wi-Fi 2.4GHz]
    Broker[Broker MQTT M360]
    
    %% Distribuição de Alimentação
    Fonte -->|5V DC| Wemos
    Fonte -->|5V DC| LDO
    LDO -->|+3V3_RF Exclusivo| NRF
    Wemos -->|3.3V Logic| R4
    
    %% Conexões SPI (Trilhas Curtas ~25mm)
    Wemos -->|D7 MOSI, D6 MISO, D5 SCK| NRF
    Wemos -->|D4 CE, D8 CSN| NRF
    
    %% Conexões Entradas e Saídas
    R4 -->|Pull-up| Wemos
    BtnSW1 -->|A0 - Puxa GND| Wemos
    
    Wemos -->|D2| LedVermelho
    Wemos -->|D1| LedVerde
    Wemos -->|D0| LedAmarelo
    
    %% Rede Sem Fio
    Wemos -.->|Wi-Fi| Router
    Router -.->|TCP/IP| Broker
    
    classDef power fill:#fff8e1,stroke:#f9a825,stroke-width:2px;
    classDef mcu fill:#e3f0ff,stroke:#0055aa,stroke-width:2px;
    classDef radio fill:#e8f5e9,stroke:#1a7a3a,stroke-width:2px;
    classDef components fill:#fff3cd,stroke:#c77a00,stroke-width:2px;
    classDef cloud fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    
    class Fonte,LDO power;
    class Wemos mcu;
    class NRF radio;
    class LedVermelho,LedAmarelo,LedVerde,BtnSW1,R4 components;
    class Router,Broker cloud;
```

---

## Tabela de Pinagem (Pinout)

### NRF24L01+ (Comunicação SPI)
| Pino Módulo | Pino D1 Mini | Descrição |
| :--- | :--- | :--- |
| **VCC** | **+3V3_RF** | Alimentação fornecida pelo LDO dedicado AMS1117-3.3V |
| **GND** | GND | Referência de Terra |
| **CE** | D4 (GPIO2) | Chip Enable (Gerenciado via firmware, sem pull-up externo) |
| **CSN** | D8 (GPIO15) | Chip Select Not |
| **SCK** | D5 (GPIO14) | Serial Clock (Trilha ~25 mm com U1 girado 180°) |
| **MOSI** | D7 (GPIO13) | Master Out Slave In (Trilha ~25 mm com U1 girado 180°) |
| **MISO** | D6 (GPIO12) | Master In Slave Out (Trilha ~25 mm com U1 girado 180°) |

---

### LEDs de Status (Painel Frontal)
A disposição física no painel segue a ordem **Vermelho · Verde · Amarelo** para evitar cruzamento de trilhas no layout da placa.

| Posição no Painel | Componente | Pino D1 Mini | Descrição |
| :---: | :--- | :--- | :--- |
| **1º (Esquerda)** | **LED Vermelho** | D2 (GPIO4) | Erro de rede ou Operando em Modo AP (Configuração) |
| **2º (Centro)** | **LED Verde** | D1 (GPIO5) | Conectado com sucesso (Wi-Fi e MQTT OK) e Rádio OK |
| **3º (Direita)** | **LED Amarelo** | D0 (GPIO16) | Transmitindo/Recebendo dados MQTT ou Inicializando |

---

### Botão de Manutenção e Pino A0
| Componente | Pino D1 Mini | Descrição |
| :--- | :--- | :--- |
| **Resistor R4 (10 kΩ)** | A0 → 3V3 | **Pull-up obrigatório.** Estabiliza A0 em repouso (≈1023 / HIGH), evitando entrada indevida em modo AP por flutuação no divisor interno. |
| **Botão SW1** | A0 → GND | Ao ser pressionado no boot, puxa A0 para LOW (0V), forçando o gateway a entrar no Modo Access Point (AP). |
| **Pino RST** | **NC (No-Connect)** | Omitido na placa. O D1 Mini já possui pull-up e botão de reset no próprio PCB, acessíveis por cima do soquete. |

