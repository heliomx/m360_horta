# **Documento de Especificação e Detalhamento Técnico da Rede de Nós IoT da Estufa**

**Projeto:** Manejo360 — Extensor Rural Digital

**Versão:** 3.5

**Ambiente de Aplicação:** Instalação Híbrida Expandida (Canteiros A, B, C e Hidroponia Vertical NFT)

**Subsistema:** Infraestrutura de Controle Distribuído e Telemetria (Protocolo MySensors)

## **1\. Visão Geral da Arquitetura**

Esta especificação consolida o planejamento físico, elétrico e lógico do ecossistema **Manejo360** na estufa. A rede utiliza uma topologia de controle distribuído em malha baseada no framework MySensors, isolando os barramentos de sensoriamento analógico das comutações indutivas de potência (relés).

A infraestrutura é dividida em **um nó atuador centralizador** e **quatro nós sensores especializados**, coordenados localmente pelo **Gateway IoT de Borda Resiliente (Wemos D1 Mini / ESP8266)** através de regras JSON locais (rules.json).

                        \[ Gateway MQTT: Wemos D1 Mini \]  
                                        ▲  
                                        │ (Rede Sem Fios nRF24L01+)  
    ┌───────────────────┼───────────────┼───────────────┼─────────────┐  
    ▼                                               ▼                                     ▼                                     ▼                                 ▼  
\[Nó 00: Atuador\]                   \[Nó 01: Solo\]                   \[Nó 02: Solo\]                   \[Nó 03: Hidro\]                  \[Nó 04: Clima\]  
 (Q. Central 5V)                     (3D \- A e B)                    (C \- Modbus)                     (NFT/Solução)              (Painel/Solar)  
 Atuadores Ativos                Alimentado 5V                 Alimentado 5V                  Alimentado 5V               smartSleep()

## **2\. Detalhamento Técnico por Nó**

### **Nó 00: Central de Atuação da Estufa (Concentrador de Potência)**

* **Localização:** Quadro Central de Energia da estufa (220V $\\rightarrow$ 12V, 5V, 3.3V).  
* **Hardware:** Arduino Nano (5V) \+ Multiplexador Digital CD74HC4067 \+ 16 Módulos de Relé 10A com isolamento por optoacoplador \+ Rádio nRF24L01+ (com adaptador de socket regulador de 3,3V).  
* **Operação Lógica:** Receptor semântico puro. Executa comandos diretos enviados pelo Gateway.  
* **Restrição de Concorrência do MUX:** Para proteção das fontes de alimentação e da rede elétrica contra picos de partida, **nenhum dos canais associados ao multiplexador funcionará simultaneamente**.  
* **Modificação Arquitetural Importante (Retirada da Bomba Principal do MUX):** A *Bomba de Circulação Principal da Hidroponia NFT* foi **removida do multiplexador** e transferida para um **Pino Digital Nativo Dedicado (D2)**. Esta alteração quebra a restrição do MUX e permite acionar as rotinas hidropônicas concorrentes exigidas pelo manejo via IA.

#### **Mapeamento dos Canais de Atuação do Nó 00:**

* **Canais via Multiplexador CD74HC4067 (Pino Comum de Sinal SIG em D3):**  
  * Canal 0 (Relé 1\): Solenoide de Irrigação por Gotejamento — Canteiro A  
  * Canal 1 (Relé 2\): Solenoide de Irrigação por Gotejamento — Canteiro B  
  * Canal 2 (Relé 3\): Solenoide de Irrigação por Gotejamento — Canteiro C (Genérico)  
  * Canal 3 (Relé 4\): *Reservado para expansão (Antiga Bomba NFT)*  
  * Canal 4 (Relé 5\): Bomba Peristáltica de Entrada — Suplemento A  
  * Canal 5 (Relé 6\): Bomba Peristáltica de Entrada — Suplemento B  
  * Canal 6 (Relé 7\): Bomba Peristáltica de Entrada — pH \+  
  * Canal 7 (Relé 8\): Bomba Peristáltica de Entrada — pH \-  
  * Canais 8 a 15: Reservados para expansão de atuadores de climatização/iluminação.  
* **Pinos Digitais Nativos Dedicados (Operação Concorrente Permitida):**  
  * **Pino Digital D2 (Relé 4):** Bomba de Circulação Principal — Hidroponia NFT.  
  * **Pino Digital D8 (Relé 9):** Bomba de Oxigenação — Hidroponia NFT.  
  * *Recomendação de Operação:* Os pinos D2 e D8 operam de forma independente e simultânea entre si e em relação ao MUX, viabilizando ciclos intermitentes síncronos de circulação e oxigenação agressiva da solução nutritiva.

### **Nó 01: Monitoramento 3D de Solo (Canteiros A e B)**

* **Hardware:** Arduino Nano (5V) \+ Multiplexador Analógico CD74HC4067 \+ Rádio nRF24L01+.  
* **Sensores de Campo:** 18 sensores resistivos passivos (varetas TIG de aço inox de 1,6 mm em blocos Sindal de 6mm²). Distância geométrica fixa de 2,0 cm e paralelismo rígido.  
* **Cabeamento Otimizado (Substituição por Cabos Ethernet):** Utilização de **cabos de rede Ethernet Cat5e/Cat6 FTP (Blindados)** em substituição aos cabos blindados convencionais de estúdio.  
  * *Critério de Projeto:* Cada cabo de rede possui 4 pares trançados. Respeitando o cancelamento de ruído (*cross-talk*), **cada sensor utiliza estritamente as duas vias de um mesmo par trançado** (Fio colorido para sinal analógico e Fio par-branco para GND). A blindagem de folha de alumínio externa do cabo de rede deve ser aterrada ao GND **apenas do lado da caixa do Arduino Nano**.  
  * Distâncias máximas limitadas a **2 metros** através da fixação física do nó no centro geométrico do canteiro (marca de 3 metros).  
* **Distribuição dos 18 Canais:**  
  * **Canteiro A (9 sensores):** Canais analógicos C0 a C8 do multiplexador. Coleta a 1m, 3m e 5m nas profundidades de 10cm, 20cm e 30cm.  
  * **Canteiro B (9 sensores):** Canais analógicos do MUX C9 a C15 (7 sensores) \+ **Pinos analógicos nativos A1 e A2 do Arduino Nano** (2 sensores).  
* **Gerenciamento Elétrico:** Barra comum de resistores fixos de pull-up de $10\\text{ k}\\Omega$ (1%) alimentada pelo pino digital **D3** a 5V. O firmware executa a rotina intermitente (D3 em HIGH $\\rightarrow$ Estabilização $\\rightarrow$ Varredura $\\rightarrow$ D3 em LOW), mitigando o processo químico de eletrólise no solo adubado.

Para os canteiros da estufa, a implementação dos cabos de rede deve seguir os critérios técnicos abaixo:

### **O Segredo do Casamento de Pares (Evitando o Cross-talk)**

O cabo Ethernet é trançado para cancelar ruídos eletromagnéticos externos. Se você ligar os sensores de forma aleatória, criará um fenômeno chamado *cross-talk* (interferência mútua), onde a leitura de um sensor altera o valor do sensor vizinho dentro do mesmo cabo.

**A Regra de Ouro:** Cada sensor deve usar **obrigatoriamente as duas vias de um mesmo par trançado** (Fio Colorido \+ Fio Par-Branco correspondente).

#### **Mapeamento de 1 Cabo de Rede para cada Grupo de Profundidade (Exemplo para 3 sensores):**

* **Par Azul:**  
  * Fio Azul $\\rightarrow$ Sinal do Sensor de 10 cm (vai para o MUX)  
  * Fio Branco/Azul $\\rightarrow$ GND  
* **Par Laranja:**  
  * Fio Laranja $\\rightarrow$ Sinal do Sensor de 20 cm (vai para o MUX)  
  * Fio Branco/Laranja $\\rightarrow$ GND  
* **Par Verde:**  
  * Fio Verde $\\rightarrow$ Sinal do Sensor de 30 cm (vai para o MUX)  
  * Fio Branco/Verde $\\rightarrow$ GND  
* **Par Marrom:** Sobra como reserva técnica ou blindagem extra (pode ser aterrado no GND do quadro).

###  **Organização dos Cabos no Nó 01 (Canteiros A e B)**

Como o Nó 01 (Arduino Nano 5V) está fixado no centro da estufa (marca de 3 metros) e gerencia 18 sensores no total, você organizará a distribuição usando apenas **5 cabos de rede** no canteiro:

* **Cabo de Rede 1 (Comprimento \~2m):** Vai para o Grupo Lateral de 1 m do **Canteiro A** (Leva os sinais de 10cm, 20cm, 30cm).  
* **Cabo de Rede 2 (Comprimento \~30cm):** Vai para o Grupo Central de 3 m do **Canteiro A** (Leva os sinais de 10cm, 20cm, 30cm).  
* **Cabo de Rede 3 (Comprimento \~2m):** Vai para o Grupo Lateral de 5 m do **Canteiro A** (Leva os sinais de 10cm, 20cm, 30cm).  
* **Cabo de Rede 4 (Comprimento \~2m):** Vai para o Grupo Lateral de 1 m do **Canteiro B** (Leva os sinais de 10cm, 20cm, 30cm).  
* **Cabo de Rede 5 (Comprimento \~2m):** Vai para os Grupos de 3m e 5m do **Canteiro B** (Como cada cabo suporta até 4 sensores, você pode derivar as fatias restantes do Canteiro B otimizando a passagem).

### **Atenção Crítica: Cabo Comum (UTP) vs. Cabo Blindado (FTP/STP)**

Existem dois tipos principais de cabo de rede no mercado. Para o ambiente de estufa comercial do **Manejo360**, a escolha muda a confiabilidade:

1. **Cabo UTP Comum (Sem Blindagem):** É o cabo de rede azul padrão de escritório. Ele funcionará bem nas linhas de 2 metros porque a nossa amostragem no firmware implementa uma janela de acomodação (POWER\_SETTLE\_MS \= 20) e descarta a primeira leitura analógica. No entanto, por não ter proteção externa, se ele passar encostado ou paralelo aos fios de alimentação de 220V das bombas de 10A do Nó 00, o ruído de indução passará para a leitura analógica.  
2. **Cabo FTP/STP (Blindado):** É um cabo de rede ligeiramente mais rígido que possui uma **folha de alumínio** envolvendo os 4 pares e um fio de dreno de metal. **Esta é a escolha ideal.** Você conecta a malha de alumínio externa (blindagem) ao pino **GND** exclusivamente do lado da caixa do Arduino Nano, deixando o lado do solo isolado. Isso cria uma gaiola de Faraday perfeita contra o ruído dos motores da estufa.

### **Cuidados com a Bitola (AWG24)**

Os fios internos do cabo de rede são finos (geralmente bitola 24 AWG, de cobre sólido). Eles se encaixam perfeitamente nos parafusos dos bornes tipo Sindal de 6mm² que planejamos na cabeça dos sensores. Apenas aperte o parafuso com cuidado para não degolar o fio de cobre por excesso de força mecânica.

Após a fixação, aplique o protocolo do documento técnico: vede com silicone neutro e aplique o espaguete termorretor com cola para garantir a estanqueidade IP67 na linha de gotejo.

### **Nó 02: Monitoramento Físico-Químico (Canteiro C \- Genérico)**

* **Hardware:** Arduino Nano (5V) \+ Módulo Conversor RS485 para TTL (MAX485) \+ Rádio nRF24L01+.  
* **Sensores de Campo:**  
  1. **Sensor Multi-parâmetro de Solo (ZTS-3002-TR):** Interface industrial diferencial **Modbus-RTU / RS485**. Mede Umidade, Temperatura do solo e Condutividade Elétrica (EC).  
  2. **Sonda Analógica de pH de Solo:** Conectada a uma entrada analógica local através de placa condicionadora de sinal.  
* **Recomendação Técnica:** Este nó deve ser mantido **energizado continuamente** (sem chaveamento de alimentação). Isso garante o tempo de aquecimento elétrico (*warm-up*) estável para calibração do pH e impede erros de sincronismo temporais no barramento diferencial (silêncio de 3,5 caracteres do Modbus).

### **Nó 03: Telemetria da Solução Nutritiva (Hidroponia NFT)**

* **Localização:** Próximo ao reservatório da hidroponia (a 4 metros de distância física do Quadro Central).  
* **Hardware:** Arduino Nano alimentado por fiação estável vindo do quadro \+ Rádio nRF24L01+.  
* **Sensores de Campo:**  
  1. **Sensor Ultrassônico de Nível Impermeável (JSN-SR04T):** Rastreamento de volume bruto para alarmes de vazamento (Failsafe local na borda).  
  2. **Sensor de Temperatura da Água (DS18B20):** Capsula metálica de inox à prova d'água operando em barramento OneWire.  
  3. **Sensor de Oxigênio Dissolvido (OD):** Sonda industrial integrada para monitorar o enriquecimento de $O\_2$ na solução.  
  4. **Sonda de pH da Água \+ Sonda de Condutividade Elétrica (EC):** Ligadas diretamente às entradas analógicas locais do Nano para cálculo do consumo mineral de NPK.

### **Nó 04: Clima Geral da Estufa (Nó Autônomo)**

* **Hardware:** Arduino Pro Mini (3,3V / 8MHz) \+ Rádio nRF24L01+.  
* **Alimentação:** Placa Solar 6V/100mA \+ Módulo TP4056 \+ 1 Bateria Li-Ion 18650\.  
* **Sensores:** Sensor de Temperatura e Umidade Relativa do Ar (DHT22) \+ Sensor de Luminosidade Digital (BH1750 via I2C).  
* **Recomendação Técnica:** Opera obrigatoriamente em **smartSleep()** cíclico de 15 minutos, minimizando o consumo estático e mantendo a autossuficiência do painel solar.

## **3\. Firmware Homologado: Nó 00 (Atuador Central)**

O código abaixo implementa o controle do **Nó 00** em C++, tratando os canais multiplexados em exclusão mútua e os pinos nativos em modo concorrente/simultâneo:

C++  
\#define MY\_RADIO\_RF24  
\#define MY\_RADIO\_RF24\_CE\_PIN 9  
\#define MY\_RADIO\_RF24\_CS\_PIN 10  
\#include \<MySensors.h\>

// Endereçamento Digital do Multiplexador CD74HC4067  
static const uint8\_t PIN\_MUX\_S0 \= 4;  
static const uint8\_t PIN\_MUX\_S1 \= 5;  
static const uint8\_t PIN\_MUX\_S2 \= 6;  
static const uint8\_t PIN\_MUX\_S3 \= 7;

// Pino Comum de Controle para Cargas Multiplexadas  
static const uint8\_t PIN\_MUX\_SIG \= 3;

// Pinos Digitais Nativos Dedicados (Atuação Simultânea Concorrente)  
static const uint8\_t PIN\_BOMBA\_NFT\_PRINCIPAL \= 2;   
static const uint8\_t PIN\_BOMBA\_OXIGENACAO    \= 8; 

// IDs Semânticos para os Pinos Nativos na rede MySensors  
static const uint8\_t CHILD\_ID\_MAIN\_PUMP \= 3;  
static const uint8\_t CHILD\_ID\_OXY\_PUMP  \= 9;

struct RelayChannel {  
  uint8\_t childId;  
  uint8\_t muxChannel;  
  const char\* label;  
  bool state;  
};

// Matriz de Atuadores do Multiplexador (Exclusão Mútua Rígida)  
RelayChannel actuators\[\] \= {  
  {0, 0, "Solenoide\_Gotejo\_CantA", false},  
  {1, 1, "Solenoide\_Gotejo\_CantB", false},  
  {2, 2, "Solenoide\_Gotejo\_CantC", false},  
  {4, 4, "Peristaltica\_Suplem\_A",  false},  
  {5, 5, "Peristaltica\_Suplem\_B",  false},  
  {6, 6, "Peristaltica\_pH\_Mais",   false},  
  {7, 7, "Peristaltica\_pH\_Menos",  false}  
};  
const uint8\_t ACTUATOR\_COUNT \= sizeof(actuators) / sizeof(actuators\[0\]);

MyMessage msgFeedback(0, V\_STATUS);

void setMuxAddress(uint8\_t channel) {  
  digitalWrite(PIN\_MUX\_S0, bitRead(channel, 0));  
  digitalWrite(PIN\_MUX\_S1, bitRead(channel, 1));  
  digitalWrite(PIN\_MUX\_S2, bitRead(channel, 2));  
  digitalWrite(PIN\_MUX\_S3, bitRead(channel, 3));  
}

void turnOffAllMuxRelays() {  
  // Módulos de relé optoacoplados utilizam lógica inversa (Active-LOW).  
  // Escrever HIGH no pino comum corta a condução de todos os canais do MUX.  
  digitalWrite(PIN\_MUX\_SIG, HIGH);   
  for(uint8\_t i \= 0; i \< ACTUATOR\_COUNT; i++) {  
    actuators\[i\].state \= false;  
  }  
}

void setup() {  
  pinMode(PIN\_MUX\_S0, OUTPUT);  
  pinMode(PIN\_MUX\_S1, OUTPUT);  
  pinMode(PIN\_MUX\_S2, OUTPUT);  
  pinMode(PIN\_MUX\_S3, OUTPUT);  
    
  pinMode(PIN\_MUX\_SIG, OUTPUT);  
  pinMode(PIN\_BOMBA\_NFT\_PRINCIPAL, OUTPUT);  
  pinMode(PIN\_BOMBA\_OXIGENACAO, OUTPUT);  
    
  // Inicialização em Estado Seguro (Cargas Desligadas em Lógica Active-LOW)  
  digitalWrite(PIN\_MUX\_SIG, HIGH);   
  digitalWrite(PIN\_BOMBA\_NFT\_PRINCIPAL, HIGH);   
  digitalWrite(PIN\_BOMBA\_OXIGENACAO, HIGH);   
}

void presentation() {  
  sendSketchInfo("Estufa\_Actuator\_Node00", "3.5.0");  
    
  // Apresentação dos Canais do Multiplexador  
  for (uint8\_t i \= 0; i \< ACTUATOR\_COUNT; i++) {  
    present(actuators\[i\].childId, S\_BINARY, actuators\[i\].label);  
    wait(30);  
  }  
  // Apresentação dos Pinos Nativos Livres  
  present(CHILD\_ID\_MAIN\_PUMP, S\_BINARY, "Bomba\_Principal\_NFT");  
  wait(30);  
  present(CHILD\_ID\_OXY\_PUMP, S\_BINARY, "Bomba\_Oxigenacao\_NFT");  
}

void receive(const MyMessage \&message) {  
  if (message.type \!= V\_STATUS) return;  
    
  uint8\_t targetChild \= message.sensor;  
  bool requestedState \= message.getBool();  
    
  // TRATAMENTO CONCORRENTE: Bomba Principal NFT (Pino Nativo D2)  
  if (targetChild \== CHILD\_ID\_MAIN\_PUMP) {  
    digitalWrite(PIN\_BOMBA\_NFT\_PRINCIPAL, requestedState ? LOW : HIGH);  
    send(msgFeedback.setSensor(CHILD\_ID\_MAIN\_PUMP).set(requestedState));  
    return;  
  }  
    
  // TRATAMENTO CONCORRENTE: Bomba de Oxigenação (Pino Nativo D8)  
  if (targetChild \== CHILD\_ID\_OXY\_PUMP) {  
    digitalWrite(PIN\_BOMBA\_OXIGENACAO, requestedState ? LOW : HIGH);  
    send(msgFeedback.setSensor(CHILD\_ID\_OXY\_PUMP).set(requestedState));  
    return;  
  }  
    
  // TRATAMENTO MULTIPLEXADO: Lógica de Não-Simultaneidade Estrita (Canais MUX)  
  for (uint8\_t i \= 0; i \< ACTUATOR\_COUNT; i++) {  
    if (actuators\[i\].childId \== targetChild) {  
      if (requestedState) {  
        // Interrompe qualquer outra carga ativa no barramento do multiplexador  
        turnOffAllMuxRelays();  
          
        // Aloca o endereço físico e aguarda acomodação do integrado  
        setMuxAddress(actuators\[i\].muxChannel);  
        wait(8);   
          
        // Ativa a condução fechando o circuito em nível lógico LOW  
        digitalWrite(PIN\_MUX\_SIG, LOW);  
        actuators\[i\].state \= true;  
        send(msgFeedback.setSensor(actuators\[i\].childId).set(true));  
      } else {  
        if (actuators\[i\].state) {  
          digitalWrite(PIN\_MUX\_SIG, HIGH);  
          actuators\[i\].state \= false;  
          send(msgFeedback.setSensor(actuators\[i\].childId).set(false));  
        }  
      }  
      break;  
    }  
  }  
}

void loop() {  
  // Quadro de comutação fixo. Permanece em escuta contínua de RF.  
}

## **4\. Recomendações Técnicas de Campo Imperativas**

1. **Acomodação Analógica no Nó 01 (Oversampling):** O firmware do Nó 01 deve implementar uma leitura analógica *dummy* de descarte ao comutar os canais, seguida de **8 amostras sequenciais com atraso de 2ms**, utilizando a média aritmética para filtrar ruídos induzidos pela proximidade com os cabos de rede Ethernet.  
2. **Proteção Contra Transientes Indutivos (Snubber):** Como o Nó 00 chaveia motores indutivos pesados (bombas e solenoides) muito próximos ao barramento lógico, torna-se obrigatória a instalação de **circuitos supressores RC** em paralelo com os contatos ,em paralelo, de saída das bobinas do solenoide para eliminar o travamento por interferência eletromagnética (EMI) no Arduino Nano.  
   Posição do diodo:  
   	Cátodo (barra do diodo): ligado ao \+12V (cátodo para o positivo)  
   	Anodo (sem barra): ligado ao GND (anodo para o negativo)  
3. **Isolamento de Junções nos Sensores:** Os fios AWG24 extraídos dos cabos Ethernet FTP e conectados aos parafusos das barras de Sindal devem ser vedados contra a atmosfera corrosiva da estufa. Aplique **silicone neutro** e feche a cabeça do sensor com **manga termorretor com adesivo (cola interna)** garantindo vedação com padrão industrial de campo.

