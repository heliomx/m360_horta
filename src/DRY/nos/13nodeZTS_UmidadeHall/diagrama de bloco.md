# Diagrama de Bloco: Nó ZTS + Umidade Hall + Selenoide (RS485)

O diagrama abaixo descreve a arquitetura elétrica derivada do firmware `noRS485.cpp`, integrando medição de múltiplos parâmetros de solo via RS485/Modbus, medição local via sensor de efeito Hall e controle de fluxo via válvula solenoide.

```mermaid
---
config:
  layout: elk
---
flowchart BT
  subgraph ZTS["ZTS-3002 Soil Sensor"]
    ZTS_Moist["Umidade"]
    ZTS_Temp["Temperatura"]
    ZTS_Cond["Condutividade"]
    ZTS_pH["pH"]
    ZTS_NPK["N / P / K"]
  end
  subgraph Sensores_Externos["Sensores_Externos"]
    ZTS
    LDR["LDR"]
    Hall["Sensor Linear de Efeito Hall<br>(Umidade via Fluxo Magnético)"]
    Radio["NRF24L01+ PA/LNA<br>(Rádio 2.4GHz)"]
  end
  subgraph Power["Fonte de Energia"]
    PS["Fonte 12V DC"]
    Reg5V@{ shape: rect, label: "Regulador 5V (MCU / MAX485 / Relé /<span style=\"color:\">Adaptador de soquete (NRF24)</span>)" }
    MCU["Arduino Pro Mini<br>(Atmega328P 5V/16MHz)"]
    Divisor["Divisor<br>de tensão<br>"]
    AdaptadorNRF["Adaptador NRF24<br>"]
    Relay["Módulo Relé"]
    MAX["Interface MAX485"]
  end
  MCU <== "SPI (SCK, MISO, MOSI)<br>CE: D6, CSN: D4" ==> AdaptadorNRF
  MCU -- TX (SoftwareSerial): D10 --> MAX
  MCU -- "Digital: D5 (Active Low)" --> Relay
  MCU -- "DE: D7 / RE: D8" --> MAX
  Divisor -- "Analógico: A1" --> MCU
  MAX -- RX (SoftwareSerial): D9 --> MCU
  ZTS --- ZTS_Moist
  ZTS --- ZTS_Temp
  ZTS --- ZTS_pH
  ZTS --- ZTS_NPK
  MAX <== "A (+) / B (-)" ==> ZTS
  Hall -- "Analógico: A0" --> MCU
  LDR --> Divisor
  Relay -- "Switch 12V Power" --> MAX
  PS --> Reg5V
  PS --> Relay
  Reg5V --> MCU
  Reg5V --> AdaptadorNRF
  AdaptadorNRF <==> Radio
```

### Detalhes das Conexões (Pinout)

| Pino Arduino | Componente | Função |
| :--- | :--- | :--- |
| **D5** | Módulo Relé | Controle da Válvula Selenoide (0=ON, 1=OFF) |
| **D7** | MAX485 | Driver Enable (DE) |
| **D8** | MAX485 | Receiver Enable (RE\_N) |
| **D9** | MAX485 | RX (SoftwareSerial: RO) |
| **D10** | MAX485 | TX (SoftwareSerial: DI) |
| **A0** | Hall Sensor | Entrada analógica para sensor de umidade local |
| **A1** | LDR | Entrada analógica para sensor de luminosidade |
| **D11/12/13** | NRF24 | Barramento SPI (MOSI/MISO/SCK) |

> [!NOTE]
> Conforme o `esquemaEletrico.md`, o rádio usa **CE: D6** e **CSN: D4** para evitar conflito com os pinos de RS485 (D9/D10).

### Recomendações de Instalação (RS485)
1. **Topologia Linear**: Não use "T-junctions" longos.
2. **Resistores de Fim de Linha**: 120Ω em ambas as extremidades físicas.
3. **Impedância**: Use cabos de ~120 ohms characteristic impedance.
