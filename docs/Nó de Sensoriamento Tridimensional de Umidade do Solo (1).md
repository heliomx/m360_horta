# **Documento de Especificação Técnica Consolidado: Nó de Sensoriamento Tridimensional de Umidade do Solo**

**Projeto:** Manejo360 — Extensor Rural Digital  
**Versão:** 2.5  
**Ambiente de Aplicação:** Canteiro de Estufa (6,00 m × 1,00 m)  
**Subsistema:** Rede Local de Sensoriamento Distribuído (Protocolo MySensors)

## 

## **1\. Visão Geral**

Este documento consolida a arquitetura física, elétrica e lógica de um nó MySensors destinado ao monitoramento tridimensional da umidade do solo em um canteiro de estufa de 6 x 1 m, usando 9 pontos de leitura distribuídos em 3 posições horizontais (1 m, 3 m e 5 m) e 3 profundidades (10 cm, 20 cm e 30 cm). A estratégia central adotada é concentrar todos os 9 sensores em um único nó com Arduino Pro Mini 3,3 V / 8 MHz, rádio nRF24L01+ e multiplexador analógico CD74HC4067, com a caixa posicionada no centro do canteiro para reduzir o comprimento máximo dos cabos para aproximadamente 2 m.  
O arranjo usa eletrodos resistivos passivos em aço inox, sem placas LM393 intermediárias, formando divisores de tensão individuais por canal com resistores fixos de 10 kΩ alimentados apenas durante a janela de leitura. Essa abordagem reduz consumo, elimina pontos de falha de módulos comerciais e mitiga eletrólise, preservando vida útil dos eletrodos em ambiente com fertirrigação e alta umidade.

## 

## **2\. Objetivo Agronômico**

O objetivo do sistema é mapear a dinâmica da água no perfil do solo de forma espacialmente organizada, capturando comportamento de umidade na zona radicular e em profundidades mais baixas, úteis para detectar percolação profunda. A distribuição 3 x 3 permite comparar longitudinalmente o canteiro e verticalmente o perfil de infiltração, criando uma malha mínima, porém informativa, para decisões de manejo hídrico.  
A decisão de inserir os eletrodos horizontalmente, e não verticalmente, é tecnicamente apropriada para isolar a leitura de cada camada específica. Na prática, isso evita que uma única sonda integre várias faixas do perfil e melhora a interpretação agronômica de cada profundidade medida.

## 

## **3\. Topologia de Campo**

A caixa de comando deve ser instalada na posição central do canteiro, na marca de 3 m, para encurtar a distância elétrica até os grupos de 1 m e 5 m. Essa decisão reduz o impacto de ruído, capacitância parasita e degradação do sinal analógico que seriam mais críticos em cabos de 5 m se o nó ficasse na cabeceira.  
Os sensores devem ser montados em três conjuntos horizontais: grupo 1 em 1 m, grupo 2 em 3 m e grupo 3 em 5 m; cada grupo contém leituras em 10 cm, 20 cm e 30 cm. Como o grupo central está praticamente ao lado da caixa e os laterais ficam a cerca de 2 m, a calibração por canal continua importante, mas o arranjo fica dentro de uma faixa prática segura para leitura analógica em ambiente agrícola.

## 

## **4\. Arquitetura de Hardware**

A unidade central usa Arduino Pro Mini 3,3 V / 8 MHz como MCU, rádio nRF24L01+ para comunicação MySensors e CD74HC4067 para multiplexar os 9 sensores em uma única entrada analógica A0. O multiplexador usa quatro linhas digitais de seleção e mantém o pino EN aterrado, permanecendo sempre habilitado.  
Os sensores de campo são eletrodos passivos montados com varetas TIG de aço inox de 1,6 mm, ligados mecanicamente a fios de cobre por blocos Sindal cortados em grupos de três bornes, com o borne central vazio funcionando como espaçador. Essa solução mantém paralelismo e uma distância mecânica fixa próxima de 2,0 cm entre hastes, o que melhora repetibilidade geométrica entre sensores.

### 

### **4.1. Motivos para Remover o LM393**

A remoção das placas comparadoras LM393 é tecnicamente correta neste contexto porque o projeto precisa de leitura analógica calibrável por software e não apenas de um limiar digital. Las placas intermediárias também introduzem trimpots expostos, LEDs de consumo contínuo e mais pontos de falha mecânica em ambiente úmido.  
Sem essas placas, cada canal passa a ser somente um divisor de tensão simples formado por resistor fixo e resistência variável do solo entre as hastes. Isso favorece leitura bruta reprodutível, calibração individual por canal e melhor integração com o firmware do nó.

### 

### **4.2. Circuito de Cada Sensor**

Cada sensor é composto por duas hastes de inox não polarizadas. Uma haste fica no nó de sinal do canal; a outra vai para GND. Entre VCC\_SENSORS e esse nó de sinal há um resistor fixo de 10 kΩ, formando o divisor de tensão que será lido pelo canal correspondente do CD74HC4067.  
Quando o solo está mais seco, a resistência entre as hastes cresce e a tensão medida tende a um extremo; quando o solo está mais úmido, a resistência cai e a tensão desloca a leitura para o outro extremo. O firmware precisa, portanto, registrar valores secos e molhados reais por sensor para converter a leitura bruta em porcentagem útil.

### 

### **4.3. Materiais e Robustez Mecânica**

O uso de varetas TIG de aço inox de 1,6 mm é adequado para o cenário de estufa porque combina resistência química, boa rigidez e baixo custo. A ligação com borne tipo Sindal evita a dificuldade de soldar estanho diretamente no inox e simplifica manutenção e substituição em campo.  
A vedação da cabeça do sensor com silicone neutro e manga termorretrátil adesivada é essencial para impedir caminhos de fuga por água superficial e corrosão da junção cobre-inox. Em ambiente de irrigação por gotejo, essa proteção deixa de ser opcional e passa a ser parte estrutural do sensor.

### 

### **4.4. Mitigação de Eletrólise**

A estratégia elétrica adotada consiste em ligar a barra VCC\_SENSORS ao pino D3 do Arduino Pro Mini e ativá-la apenas durante a amostragem. O firmware deve elevar D3 para HIGH, aguardar o tempo de estabilização do canal selecionado, executar a leitura analógica e depois retornar D3 para LOW antes de entrar em smartSleep().  
Esse chaveamento intermitente reduz fortemente o processo de eletrólise e também elimina consumo estático contínuo no arranjo dos sensores. Em um nó agrícola com leituras espaçadas em minutos, essa é uma decisão de projeto muito superior a manter as sondas permanentemente polarizadas.

## 

## **5\. Pinagem Consolidada**

### **Rádio nRF24L01+**

| Sinal | Arduino Pro Mini | Observação |
| :---- | :---- | :---- |
| CE | D9 | Controle do rádio. |
| CSN | D10 | Chip select SPI. |
| MOSI | D11 | SPI padrão. |
| MISO | D12 | SPI padrão. |
| SCK | D13 | SPI padrão. |
| VCC | 3.3 V | Nunca ligar em 5 V; usar capacitor local. |
| GND | GND | Terra comum. |

É recomendada a instalação de um capacitor eletrolítico entre 10 µF e 47 µF diretamente nos pinos de alimentação do rádio para reduzir instabilidades de transmissão. Em nós compactos com Pro Mini 3,3 V e nRF24L01+, esse detalhe costuma ser decisivo para confiabilidade do enlace.

### **Multiplexador CD74HC4067**

| Sinal | Arduino Pro Mini | Observação |
| :---- | :---- | :---- |
| SIG | A0 | Saída analógica do canal selecionado. |
| S0 | D4 | Seleção binária do canal. |
| S1 | D5 | Seleção binária do canal. |
| S2 | D6 | Seleção binária do canal. |
| S3 | D7 | Seleção binária do canal. |
| EN | GND | Sempre habilitado. |
| VCC | 3.3 V | Alimentação do MUX. |
| GND | GND | Terra comum. |

###   **Controle dos sensores**

| Função | Arduino Pro Mini | Observação |
| :---- | :---- | :---- |
| VCC\_SENSORS | D3 | Alimenta a barra dos resistores de pull-up somente durante leitura. |
| C0 a C8 | Entradas do CD74HC4067 | Um canal por sensor de umidade. |

**Mapeamento dos 9 canais**O padrão de IDs por dezena para posição e unidade para profundidade organiza bem o backend e facilita interpretação humana do dado. Ele também se encaixa bem na lógica de serialização JSON no gateway, onde cada child pode ser descrito com metadados fixos de posição e profundidade.

## **6\. Estratégia de Calibração**

Cada sensor deve ser calibrado individualmente, porque pequenas diferenças de cabo, geometry, solo adjacente e contato mecânico alteram o valor bruto lido. O mínimo necessário é registrar dois pontos por canal: rawDry para solo seco de referência e rawWet para solo úmido ou capacidade de campo.  
A conversão de leitura bruta para porcentagem pode ser feita por interpolação linear limitada entre 0 e 100\. Dependendo do sentido observado na prática, a fórmula deve ser invertida, já que alguns arranjos resistivos resultam em leitura maior no seco e outros no molhado por causa da geometria elétrica adotada.

## **7\. Firmware Consolidado**

A seguir está um esqueleto completo de firmware em C++ para o nó, compatível com o arranjo descrito e preparado para integração com gateway que converte mensagens MySensors em JSON no Wemos D1 Mini ou camada equivalente.

C++  
\#define MY\_RADIO\_RF24  
\#define MY\_RF24\_CE\_PIN 9  
\#define MY\_RF24\_CS\_PIN 10  
\#include \<MySensors.h\>

static const uint8\_t PIN\_VCC\_SENSORS \= 3;  
static const uint8\_t PIN\_MUX\_SIG \= A0;  
static const uint8\_t PIN\_MUX\_S0 \= 4;  
static const uint8\_t PIN\_MUX\_S1 \= 5;  
static const uint8\_t PIN\_MUX\_S2 \= 6;  
static const uint8\_t PIN\_MUX\_S3 \= 7;

static const uint8\_t CHILD\_ID\_CTRL \= 1;  
static const uint32\_t SLEEP\_INTERVAL\_MS \= 15UL \* 60UL \* 1000UL;

static const uint8\_t RAW\_SAMPLES \= 8;  
static const uint8\_t RAW\_DISCARD\_FIRST \= 1;  
static const uint16\_t POWER\_SETTLE\_MS \= 15;  
static const uint16\_t CHANNEL\_SETTLE\_MS \= 8;  
static const uint16\_t BETWEEN\_SEND\_MS \= 80;

struct MoistureChannel {  
  uint8\_t childId;  
  uint8\_t muxChannel;  
  const char \*label;  
  uint16\_t rawDry;  
  uint16\_t rawWet;  
  bool invert;  
};

MoistureChannel channels\[\] \= {  
  {11, 0, "Moist\_1m\_10cm", 820, 320, true},  
  {12, 1, "Moist\_1m\_20cm", 820, 320, true},  
  {13, 2, "Moist\_1m\_30cm", 820, 320, true},  
  {31, 3, "Moist\_3m\_10cm", 820, 320, true},  
  {32, 4, "Moist\_3m\_20cm", 820, 320, true},  
  {33, 5, "Moist\_3m\_30cm", 820, 320, true},  
  {51, 6, "Moist\_5m\_10cm", 820, 320, true},  
  {52, 7, "Moist\_5m\_20cm", 820, 320, true},  
  {53, 8, "Moist\_5m\_30cm", 820, 320, true}  
};

const uint8\_t CHANNEL\_COUNT \= sizeof(channels) / sizeof(channels\[0\]);

MyMessage msgLevel(0, V\_LEVEL);  
MyMessage msgRaw(0, V\_VAR1);  
MyMessage msgCtrl(CHILD\_ID\_CTRL, V\_TEXT);

enum CalMode {  
  CAL\_NONE,  
  CAL\_DRY,  
  CAL\_WET  
};

CalMode calMode \= CAL\_NONE;

void setMuxChannel(uint8\_t channel) {  
  digitalWrite(PIN\_MUX\_S0, bitRead(channel, 0));  
  digitalWrite(PIN\_MUX\_S1, bitRead(channel, 1));  
  digitalWrite(PIN\_MUX\_S2, bitRead(channel, 2));  
  digitalWrite(PIN\_MUX\_S3, bitRead(channel, 3));  
}

void setupSensorHardware() {  
  pinMode(PIN\_MUX\_S0, OUTPUT);  
  pinMode(PIN\_MUX\_S1, OUTPUT);  
  pinMode(PIN\_MUX\_S2, OUTPUT);  
  pinMode(PIN\_MUX\_S3, OUTPUT);  
  pinMode(PIN\_VCC\_SENSORS, OUTPUT);  
  digitalWrite(PIN\_VCC\_SENSORS, LOW);  
}

uint16\_t readSensorArrayChannel(uint8\_t channel) {  
  digitalWrite(PIN\_VCC\_SENSORS, HIGH);  
  wait(POWER\_SETTLE\_MS);  
  setMuxChannel(channel);  
  wait(CHANNEL\_SETTLE\_MS);  
    
  // Leitura dummy para estabilização do ADC interno  
  analogRead(PIN\_MUX\_SIG);  
    
  uint32\_t acc \= 0;  
  uint8\_t validSamples \= 0;  
    
  for (uint8\_t i \= 0; i \< RAW\_SAMPLES; i++) {  
    uint16\_t v \= analogRead(PIN\_MUX\_SIG);  
    if (i \>= RAW\_DISCARD\_FIRST) {  
      acc \+= v;  
      validSamples++;  
    }  
    wait(2);  
  }  
    
  digitalWrite(PIN\_VCC\_SENSORS, LOW);  
  if (validSamples \== 0\) return 0;  
  return (uint16\_t)(acc / validSamples);  
}

uint8\_t rawToPercent(uint16\_t raw, const MoistureChannel \&c) {  
  int32\_t span;  
  int32\_t pct;  
    
  if (c.invert) {  
    span \= (int32\_t)c.rawDry \- (int32\_t)c.rawWet;  
    if (span \== 0\) return 0;  
    pct \= ((int32\_t)c.rawDry \- (int32\_t)raw) \* 100L / span;  
  } else {  
    span \= (int32\_t)c.rawWet \- (int32\_t)c.rawDry;  
    if (span \== 0\) return 0;  
    pct \= ((int32\_t)raw \- (int32\_t)c.rawDry) \* 100L / span;  
  }  
    
  if (pct \< 0\) pct \= 0;  
  if (pct \> 100\) pct \= 100;  
  return (uint8\_t)pct;  
}

void presentation() {  
  sendSketchInfo("Soil3D\_Node\_0001", "1.0.0");  
  present(CHILD\_ID\_CTRL, S\_INFO, "NodeCtrl");  
  for (uint8\_t i \= 0; i \< CHANNEL\_COUNT; i++) {  
    present(channels\[i\].childId, S\_MOISTURE, channels\[i\].label);  
  }  
}

void setup() {  
  setupSensorHardware();  
}

void sendNormalReadings() {  
  for (uint8\_t i \= 0; i \< CHANNEL\_COUNT; i++) {  
    uint16\_t raw \= readSensorArrayChannel(channels\[i\].muxChannel);  
    uint8\_t pct \= rawToPercent(raw, channels\[i\]);  
    send(msgLevel.setSensor(channels\[i\].childId).set((uint8\_t)pct));  
    wait(BETWEEN\_SEND\_MS);  
  }  
}

void sendCalibrationReadings(CalMode mode) {  
  for (uint8\_t i \= 0; i \< CHANNEL\_COUNT; i++) {  
    uint16\_t raw \= readSensorArrayChannel(channels\[i\].muxChannel);  
    if (mode \== CAL\_DRY) channels\[i\].rawDry \= raw;  
    if (mode \== CAL\_WET) channels\[i\].rawWet \= raw;  
    send(msgRaw.setSensor(channels\[i\].childId).set((uint16\_t)raw));  
    wait(BETWEEN\_SEND\_MS);  
  }  
    
  if (mode \== CAL\_DRY) {  
    send(msgCtrl.set("CAL\_DONE\_DRY"));  
  } else if (mode \== CAL\_WET) {  
    send(msgCtrl.set("CAL\_DONE\_WET"));  
  }  
}

void receive(const MyMessage \&message) {  
  if (message.sensor \!= CHILD\_ID\_CTRL) return;  
  if (message.type \!= V\_TEXT) return;  
    
  String cmd \= message.getString();  
  cmd.trim();  
    
  if (cmd \== "CAL\_START\_DRY") {  
    calMode \= CAL\_DRY;  
  } else if (cmd \== "CAL\_START\_WET") {  
    calMode \= CAL\_WET;  
  } else if (cmd \== "CAL\_CANCEL") {  
    calMode \= CAL\_NONE;  
  }  
}

void loop() {  
  if (calMode \== CAL\_DRY || calMode \== CAL\_WET) {  
    sendCalibrationReadings(calMode);  
    calMode \= CAL\_NONE;  
  } else {  
    sendNormalReadings();  
  }  
  smartSleep(SLEEP\_INTERVAL\_MS);  
}  
}

### 

### **7.1. Ajustes Importantes no Driver de Leitura**

A função readSensorArrayChannel() deve energizar toda a barra dos sensores uma única vez por leitura de canal, aguardar estabilização inicial da alimentação e depois estabilização do canal no multiplexador. Esse detalhe é importante porque há capacitância distribuída nos cabos e no próprio multiplexador, além do efeito de carga do sample-and-hold interno do ADC do ATmega.  
Descartar a primeira leitura analógica e usar média das seguintes é uma melhoria prática recomendável nesse tipo de topologia. Isso reduz erro transitório sem tornar o tempo de leitura excessivo, especialmente quando o intervalo entre ciclos é da ordem de minutos.

### **7.2. Estratégia de Envio MySensors**

O desenho mais simples é usar S\_MOISTURE com V\_LEVEL para a porcentagem calibrada e um child de controle com V\_TEXT para comandos de calibração. Durante calibração, os valores brutos podem ser enviados por V\_VAR1, preservando um canal semântico limpo entre operação normal e manutenção.  
Um pequeno atraso entre envios, da ordem de 80 a 100 ms, ajuda a evitar rajadas excessivas no enlace nRF24L01+ e facilita o trabalho do gateway baseado em ESP8266 ou Wemos D1 Mini. Em arranjos com 9 sensores sequenciais, essa folga costuma valer mais do que tentar espremer o tempo total de transmissão ao máximo.

## **8\. Adaptação para Gateway com JSON sobre MySensors**

Em uma arquitetura com Wemos D1 Mini atuando como gateway e serializando dados para JSON, o ideal é que o nó permaneça simples e envie mensagens MySensors semanticamente claras. A camada JSON do gateway deve anexar metadados do sensor, como posição longitudinal, profundidade, rótulo humano e tipo de leitura, em vez de empurrar JSON bruto para o Pro Mini.

### **Evento JSON Recomendado para Leituras Normais:**

JSON  
{  
  "nodeId": 1,  
  "childId": 31,  
  "kind": "soil\_moisture",  
  "label": "Moist\_3m\_10cm",  
  "position\_m": 3,  
  "depth\_cm": 10,  
  "value": 67,  
  "unit": "%",  
  "transport": "mysensors",  
  "valueType": "V\_LEVEL",  
  "ts": "2026-05-31T13:00:00-03:00"  
}

### 

### **Evento JSON Recomendado para Calibração Bruta:**

JSON  
{  
  "nodeId": 1,  
  "childId": 31,  
  "kind": "soil\_moisture\_raw",  
  "label": "Moist\_3m\_10cm",  
  "position\_m": 3,  
  "depth\_cm": 10,  
  "raw": 412,  
  "valueType": "V\_VAR1",  
  "calibrationMode": "dry",  
  "ts": "2026-05-31T13:05:00-03:00"  
}

O ganho dessa abordagem é separar responsabilidades: o nó mege e transmite; o gateway enriquece, normaliza, roteia e integra com banco de dados, dashboard ou motor de inferência. Isso também preserva memória e simplicidade no Pro Mini, que é o recurso mais restrito do sistema.

### 

### **Contrato Lógico Sugerido no Gateway:**

A camada do gateway deve manter uma tabela local que mapeia childId para posição, profundidade, label e tipo do dado. Ao receber V\_LEVEL, o gateway publica um evento de umidade calibrada; ao receber V\_VAR1, publica evento de leitura bruta de calibração; ao receber V\_TEXT com CAL\_DONE\_DRY ou CAL\_DONE\_WET, encerra a etapa operacional correspondente.

JavaScript  
const sensorMap \= {  
  11: { label: 'Moist\_1m\_10cm', position\_m: 1, depth\_cm: 10 },  
  12: { label: 'Moist\_1m\_20cm', position\_m: 1, depth\_cm: 20 },  
  13: { label: 'Moist\_1m\_30cm', position\_m: 1, depth\_cm: 30 },  
  31: { label: 'Moist\_3m\_10cm', position\_m: 3, depth\_cm: 10 },  
  32: { label: 'Moist\_3m\_20cm', position\_m: 3, depth\_cm: 20 },  
  33: { label: 'Moist\_3m\_30cm', position\_m: 3, depth\_cm: 30 },  
  51: { label: 'Moist\_5m\_10cm', position\_m: 5, depth\_cm: 10 },  
  52: { label: 'Moist\_5m\_20cm', position\_m: 5, depth\_cm: 20 },  
  53: { label: 'Moist\_5m\_30cm', position\_m: 5, depth\_cm: 30 }  
};

### 

### **Fluxo Operacional Recomendado:**

1. O nó acorda do smartSleep(), energiza o arranjo, lê os 9 sensores sequencialmente e envia 9 mensagens V\_LEVEL com porcentagem calibrada.  
2. O gateway recebe, enriquece cada evento com metadados fixos e publica JSON estruturado para armazenamento ou inferência.  
3. Em modo de calibração, o operador envia CAL\_START\_DRY ou CAL\_START\_WET para o child de controle.  
4. O nó mede, atualiza os parâmetros em RAM e envia os raw de cada child por V\_VAR1, finalizando com CAL\_DONE\_DRY ou CAL\_DONE\_WET.

## 

## **9\. Riscos e Mitigação**

| Risco | Causa Provável | Mitigação |
| :---- | :---- | :---- |
| **Leituras instáveis** | Ruído, cabo úmido, vedação insuficiente. | Cabos curtos, vedação da cabeça do sensor, média de leituras e descarte da primeira amostra. |
| **Corrosão prematura** | Polarização contínua e água na junção. | Chaveamento no D3, silicone neutro e termorretrátil adesivado. |
| **Drift entre sensores** | Geometria desigual ou solo diferente. | Calibração individual rawDry/rawWet por canal. |
| **Falha de rádio** | Queda de tensão no nRF24L01+. | Capacitor local de 10 µF a 47 µF e layout curto de alimentação. |
| **Interpretação errada** | Child IDs sem metadados no backend. | Tabela fixa no gateway para enriquecer JSON. |

## 

## **10\. Decisões Finais Recomendadas**

A arquitetura proposta é tecnicamente consistente para um MVP robusto de estufa e tem boa relação entre simplicidade, custo e capacidade analítica. O ponto mais importante para o sucesso do sistema não é adicionar mais eletrônica, mas sim manter geometria mecânica repetível, vedação correta, leitura analógica bem amortecida e calibração individual por canal.  
Para a integração com o gateway atual, a melhor estratégia é manter o Pro Mini focado em MySensors puro e deixar a serialização JSON no Wemos D1 Mini ou camada equivalente. Esse desacoplamento preserva clareza de protocolo, simplifica manutenção do firmware e reduz risco de esgotar memória no nó de campo.

11\. Lista de Materiais

[10 X Módulo Multiplexador Cd74hc4067 Cmos Analógico Digital](https://produto.mercadolivre.com.br/MLB-1146256833-10-x-modulo-multiplexador-cd74hc4067-cmos-analogico-digital-_JM)

[(10 Peças) Barra 12 Bornes Sindal H-05a 6mm Branca](https://produto.mercadolivre.com.br/MLB-2015061497-10-pecas-barra-12-bornes-sindal-h-05a-6mm-branca-_JM)

[Fita Isolante Autofusão Foxlux 5m X 19mm Alta Tensão 10kv Vedação Cabos Elétricos Impermeável Reparos Preto Lisa](https://produto.mercadolivre.com.br/MLB-4148993189-fita-isolante-autofuso-foxlux-5m-x-19mm-alta-tenso-10kv-vedaco-cabos-eletricos-impermeavel-reparos-preto-lisa-_JM)

[Vareta De Solda Tig Alumínio Er 4043 2,4mm 3/32 1 Kg Punta Branco](https://produto.mercadolivre.com.br/MLB-3950486475-vareta-de-solda-tig-aluminio-er-4043-24mm-332-1-kg-punta-branco-_JM)

