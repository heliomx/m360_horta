# Nó 80 - Monitoramento de Caixa de Água (pH, EC, Nível e Vazão)

Este diretório contém a documentação e o código para o **Nó 80 (80nodeAqua)**. Ele é projetado para monitorar parâmetros críticos de qualidade da água, nível do reservatório e taxas de fluxo em múltiplos pontos de distribuição da horta. 

O nó é configurado para funcionar em modo de alimentação permanente (`M360_ALWAYS_ON`) usando energia contínua de 5V DC proveniente do painel elétrico, garantindo que o rádio nRF24L01+ esteja sempre pronto para receber comandos e que as contagens de pulsos de fluxo não sejam perdidas.

---

## 🛠️ Resumo de Hardware e Conexões

- **Controlador:** Arduino Nano (ATmega328P, 5V / 16MHz)
- **Rádio:** nRF24L01+ conectado na interface SPI padrão do Nano
- **Alimentação:** 5V contínuo regulado (ALWAYS_ON)
- **Gestão de Energia:** O pino digital **D7** (`PIN_POWER_SENSORS`) funciona como um barramento chaveado de alimentação para os sensores de Nível, pH, EC e Temperatura, reduzindo o desgaste elétrico e eletrólise nas sondas. Os sensores de vazão utilizam alimentação direta de **5V** para monitoramento em tempo real sem interrupções.

Para uma visão detalhada das conexões elétricas e especificações, consulte o arquivo [esquema_eletrico.md](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/esquema_eletrico.md).

---

## 📊 Estrutura de Itens e Sensores (CHILD IDs)

O nó monitora e apresenta 8 itens diferentes para o gateway MySensors:

| ID | Nome do Item | Tipo de Sensor (S_) | Tipo de Variável (V_) | Pino de Sinal | Pino de Energia |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | Nivel Caixa | `S_MOISTURE` | `V_LEVEL` | D4 (Trig) / D5 (Echo) | D7 (Chaveado) |
| **1** | pH | `S_WATER_QUALITY` | `V_PH` | A0 (Analógico) | D7 (Chaveado) |
| **2** | EC | `S_WATER_QUALITY` | `V_EC` | A1 (Analógico) | D7 (Chaveado) |
| **3** | Vazao 1 | `S_WATER` | `V_FLOW` | D2 (INT0) | 5V (Direto) |
| **4** | Temp Agua | `S_TEMP` | `V_TEMP` | D6 (OneWire) | D7 (Chaveado) |
| **5** | Vazao A | `S_WATER` | `V_FLOW` | D3 (INT1) | 5V (Direto) |
| **6** | Vazao B | `S_WATER` | `V_FLOW` | D8 (PCINT0) | 5V (Direto) |
| **7** | Vazao C | `S_WATER` | `V_FLOW` | A2 (PCINT10) | 5V (Direto) |

---

## ⚡ Detalhes dos Drivers e Calibração

### 1. Nível (Sensor Ultrassônico HC-SR04)
Mede a distância da água até o sensor (instalado sob a tampa da caixa d'agua). A conversão em percentual de 0% a 100% baseia-se nos limites físicos do reservatório:
*   **Caixa Vazia (0%):** 100 cm de distância.
*   **Caixa Cheia (100%):** 10 cm de distância.
*   *Fórmula:* `levelPercent = (100.0 - distância) * 100.0 / 90.0`

### 2. Sensor de pH (Condicionador pH-4502C)
Realiza uma média de 10 amostras analógicas na porta **A0**:
*   *Fórmula padrão:* `pH = -5.7 * Tensão + 21.34`
*   *Calibração física:* Ajuste o potenciômetro de offset no condicionador para que a saída seja 2.5V quando imerso em solução tampão pH 7.0.

### 3. Sensor de EC (Condutividade Elétrica)
Coleta a média de 10 amostras analógicas na porta **A1**:
*   *Fórmula:* `EC = 1.0 * Tensão` (Ajustar a inclinação e o offset conforme a calibração com soluções padrão).


### 4. Temperatura da Água (Dallas DS18B20)
Sensor digital no barramento OneWire conectado ao pino **D6**.
*   > [!IMPORTANT]
    > Exige um resistor pull-up físico de 4.7kΩ conectado entre a linha de dados (D6) e o VCC chaveado (D7).


### As amostras de temperatura, pH e EC são tiradas a cada 10 minutos e depois de 2 minutos apos a Vazão H for acionada e enquanto ela estiver ligada, a cada 2 minutos 

### 5. Sensores de Vazão (YF-S201)
Mede o fluxo em litros por segundo (L/s). Toda contagem é feita por interrupções físicas de hardware para evitar perdas de pulso:
*   **Vazão H (D2):** Interrupção externa `INT0`
*   **Vazão A (D3):** Interrupção externa `INT1`
*   **Vazão B (D8):** Interrupção por mudança de estado `PCINT0` (Port B)
*   **Vazão C (A2):** Interrupção por mudança de estado `PCINT10` (Port C)
*   *Fórmula:* `Q (L/s) = Frequência (Hz) / 450.0`


## 📁 Estrutura de Arquivos do Diretório

O código está estruturado em conformidade com as diretrizes da **LibDRY**:

- **[esquema_eletrico.md](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/esquema_eletrico.md):** Mapeamento físico dos pinos, notas elétricas de montagem e calibração.
- **[withLibDRY/](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/withLibDRY/):** Pasta contendo o firmware integrado com a LibDRY.
  - **[80nodeAqua.cpp](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/withLibDRY/80nodeAqua.cpp):** Código principal do nó MySensors, define as propriedades do motor M360Node e os hooks do MySensors (`before`, `presentation`, `setup`, `loop`, `receive`).
  - **[sensorDrivers.h](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/withLibDRY/sensorDrivers.h):** Interface de controle dos sensores e mapeamento dos pinos.
  - **[sensorDrivers.cpp](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/withLibDRY/sensorDrivers.cpp):** Lógica de baixo nível para leitura do ADC, cálculo de vazão baseada em contagem de pulsos e rotinas de controle de VCC chaveado.

---

## 🎨 Diagramas e Modelos Auxiliares

Além do esquema elétrico em markdown, este diretório contém os seguintes arquivos visuais de suporte:
- **[diagrama_blocos.svg](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/diagrama_blocos.svg):** Diagrama de blocos contendo a representação visual do fluxo de sinal e alimentação do nó.
- **[Camara para sondas .png](file:///c:/Users/jmarc/Documents/PlatformIO/Projects/m360_horta/src/DRY/nos/80nodeAqua/Camara%20para%20sondas%20.png):** Imagem/desenho conceitual da câmara de acomodação física das sondas de pH/EC e temperatura da água.

