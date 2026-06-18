# Documentação e Diagramas de Blocos — sensores.cpp

Este documento descreve a arquitetura de hardware e o fluxo de software do módulo de teste para sensores de umidade do solo, implementado no arquivo [sensores.cpp](file:///j:/Meu%20Drive/GDrive%20Meus%20Documentos/Projetos%20(1)/PlatformIO/Projects/m360_horta/src/test/Sensores%20de%20Umidade/sensores.cpp).

O principal objetivo deste firmware de teste é realizar leituras sequenciais calibradas de 4 sensores de umidade, mitigando o efeito de eletrólise nas sondas através do chaveamento dinâmico da alimentação (VCC) via pino digital.

---

## 1. Conexão de Hardware (Mapeamento de Pinos)

| Componente / Recurso | Tipo de Sinal | Pino Arduino | Descrição / Função |
| :--- | :--- | :--- | :--- |
| **Controle VCC** | Digital (Saída) | `D3` (GPIO 3) | Controla a alimentação da barra de resistores de 10kΩ dos sensores para evitar corrosão galvânica (eletrólise). |
| **Sensor de Umidade 1** | Analógico (Entrada) | `A0` | Canal de leitura do primeiro sensor de solo. |
| **Sensor de Umidade 2** | Analógico (Entrada) | `A1` | Canal de leitura do segundo sensor de solo. |
| **Sensor de Umidade 3** | Analógico (Entrada) | `A2` | Canal de leitura do terceiro sensor de solo. |
| **Sensor de Umidade 4** | Analógico (Entrada) | `A3` | Canal de leitura do quarto sensor de solo. |

### Diagrama de Conexão Física (Blocos)

```mermaid
graph TD
    subgraph Arduino ["Arduino Nano / Uno"]
        D3["Pino D3 (VCC Control)"]
        A0["Pino A0 (Sensor 1 Analog)"]
        A1["Pino A1 (Sensor 2 Analog)"]
        A2["Pino A2 (Sensor 3 Analog)"]
        A3["Pino A3 (Sensor 4 Analog)"]
    end

    subgraph SensorArray ["Rede de Sensores"]
        R_ARRAY["Barra de Resistores (Pull-down de 10kΩ)"]
        S1["Sonda de Umidade 1"]
        S2["Sonda de Umidade 2"]
        S3["Sonda de Umidade 3"]
        S4["Sonda de Umidade 4"]
    end

    D3 -->|Alimentação Chaveada| R_ARRAY
    R_ARRAY --> S1 & S2 & S3 & S4
    
    S1 -->|Sinal Analógico| A0
    S2 -->|Sinal Analógico| A1
    S3 -->|Sinal Analógico| A2
    S4 -->|Sinal Analógico| A3
```

---

## 2. Fluxograma de Software

O algoritmo realiza amostragens periódicas acumulando as leituras. A cada 4 ciclos de amostragem, calcula a média móvel, converte o valor bruto do conversor analógico-digital (ADC) para porcentagem (baseado na calibração) e exibe os resultados na Serial.

```mermaid
flowchart TD
    Start([Início]) --> Setup["Setup:
- Inicializa Serial (115200 bps)
- Configura D3 como OUTPUT e LOW"]
    
    Setup --> Loop[Loop Principal]
    
    %% Ciclo de Amostragem Rápida e Chaveada
    Loop --> PowerOn["Ligar VCC (D3 = HIGH)"]
    PowerOn --> DelayVcc["Aguardar 20ms (Estabilização da tensão)"]
    
    DelayVcc --> LoopSensors["Para cada Sensor i de 0 a 3"]
    LoopSensors --> ReadADC["Ler Pino Analógico correspondente (A0 a A3)"]
    ReadADC --> DelayADC["Aguardar 5ms (Estabilização do multiplexador ADC)"]
    DelayADC --> NextSensor{"Mais sensores?"}
    NextSensor -- Sim --> LoopSensors
    NextSensor -- Não --> PowerOff["Desligar VCC (D3 = LOW)"]
    
    %% Acumulador e Filtro de Média
    PowerOff --> Accumulate["Acumular leituras em somaLeituras[i]"]
    Accumulate --> IncCount["Incrementar contadorLeituras"]
    
    IncCount --> CheckCounter{"contadorLeituras >= 4?"}
    
    %% Se não atingiu 4 leituras, apenas espera
    CheckCounter -- Não --> DelayLoop["Aguardar 500ms (Amostragem)"]
    
    %% Se atingiu 4 leituras, faz o reporte
    CheckCounter -- Sim --> LoopReport["Para cada Sensor i de 0 a 3"]
    LoopReport --> CalcAver["Calcular ADC Médio = somaLeituras[i] / 4"]
    CalcAver --> CalcPercent["Calcular Umidade % = calcularPorcentagemUmidade(ADC Médio)"]
    CalcPercent --> PrintSerial["Imprimir na Serial no formato: [Umidade%](ADC Médio)"]
    PrintSerial --> ResetSum["Zerar somaLeituras[i]"]
    ResetSum --> NextReport{"Mais sensores?"}
    NextReport -- Sim --> LoopReport
    NextReport -- Não --> ResetCounter["Zerar contadorLeituras"]
    ResetCounter --> DelayLoop
    
    DelayLoop --> Loop
```

---

## 3. Lógica de Calibração e Conversão

A função `calcularPorcentagemUmidade` mapeia linearmente o valor medido no ADC para uma escala de 0% (seco) a 100% (totalmente úmido).

### Limites Calibrados
- **Solo Seco (0% Umidade):** `VALOR_SECO = 1020`
- **Solo Úmido (100% Umidade):** `VALOR_UMIDO = 450`

```cpp
int calcularPorcentagemUmidade(int valorLido) {
  if (VALOR_SECO == VALOR_UMIDO) {
    return 0;
  }
  int umidade = map(valorLido, VALOR_SECO, VALOR_UMIDO, 0, 100);
  return constrain(umidade, 0, 100); // Garante o intervalo de 0 a 100%
}
```

---

## 4. Estratégias de Confiabilidade do Sinal

1. **Mitigação de Eletrólise:** A alimentação das sondas via pino `D3` fica ativa apenas por `20ms + (4 * 5ms) = 40ms` a cada ciclo de amostragem de `540ms` (~7.4% de duty cycle), reduzindo drasticamente o desgaste por corrosão galvânica das sondas metálicas no solo.
2. **Estabilização do Multiplexador ADC:** O delay de `5ms` inserido entre a leitura de canais analógicos adjacentes permite a descarga adequada do capacitor interno de *sample and hold* do microcontrolador, evitando que o valor lido em um sensor influencie o canal seguinte (crosstalk).
3. **Média Móvel de Amostragem (Filtro Passa-Baixas):** Ao calcular a média a cada 4 leituras brutas antes do envio, são filtrados ruídos de alta frequência induzidos na fiação dos sensores.
