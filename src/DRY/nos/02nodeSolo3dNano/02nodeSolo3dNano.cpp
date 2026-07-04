/*
 * 02nodeSolo3dNano.cpp — Nó 02: Monitoramento 3D de Solo (Versão Arduino Nano)
 *
 * Hardware: Arduino Nano (5V / 16MHz) + 6 Sensores Resistivos diretamente +
 * nRF24L01+ (CE=D9, CSN=D10) Alimentação: Fonte fixa ou bateria com carga
 * contínua
 *
 * Perfil: M360_ALWAYS_ON — timer por millis(), sem sleep.
 * O pino D3 (PIN_POWER_SENSORS) ainda é desligado entre leituras para
 * mitigar a eletrólise dos eletrodos no solo.
 */

#include <Arduino.h>
#include <M360.h>
#include <MySensors.h>

// Pinos nativos do Arduino Nano para os 6 sensores resistivos
static const uint8_t sensorPins[] = {A0, A1, A2, A3, A4, A5};

// Pino de Energia dos Pull-ups (Mitigação de Eletrólise)
#define PIN_POWER_SENSORS 3 // D3

// Definição dos 6 sensores resistivos (Canteiro B)
static const M360::M360ItemDef nodeItems[] = {
    // Canteiro B - 6 sensores (Child IDs 0 a 5)
    {0, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A0, 0, 1, "B_1m_10cm", false,
     0},
    {1, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A1, 0, 1, "B_1m_30cm", false,
     0},
    {2, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A2, 0, 1, "B_2m_10cm", false,
     0},
    {3, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A3, 0, 1, "B_2m_30cm", false,
     0},
    {4, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A4, 0, 1, "B_3m_10cm", false,
     0},
    {5, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A5, 0, 1, "B_3m_30cm", false,
     0}};

static const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

static MyMessage messages[numItems + 3]; // +1 Intervalo (254) +1 Bateria (255)
                                         // +1 Debug (253)
static float lastValues[numItems];
static uint8_t nNoUpdates[numItems];

static M360::M360Node node(nodeItems, numItems, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

/**
 * Configura os pinos de controle (Power) como OUTPUT, e pinos analógicos como
 * INPUT.
 */
void initSensors() {
  // Configura o pino de energia (pull-ups) como saída e desliga por padrão
  pinMode(PIN_POWER_SENSORS, OUTPUT);
  digitalWrite(PIN_POWER_SENSORS, LOW);

  // Configura os pinos de leitura analógica como entrada
  for (uint8_t i = 0; i < numItems; i++) {
    pinMode(sensorPins[i], INPUT);
  }
}

/**
 * Liga a alimentação de pull-up dos sensores (mitigação de eletrólise) e
 * aguarda estabilização.
 */
void powerUpSensors() {
  // Energiza a barra de resistores de pull-up dos sensores se estiver desligada
  if (digitalRead(PIN_POWER_SENSORS) == LOW) {
    digitalWrite(PIN_POWER_SENSORS, HIGH);
    // Tempo de estabilização das capacitâncias parasitas nos cabos longos
    delay(20);
  }
}

/**
 * Desliga a alimentação de pull-up dos sensores.
 */
void powerDownSensors() {
  // Desliga a alimentação para cessar corrente e evitar eletrólise nos
  // eletrodos
  digitalWrite(PIN_POWER_SENSORS, LOW);
}

/**
 * Lê o sensor resistivo específico de forma direta (sem MUX).
 * Mapeia 1023..0 para 0..100%.
 * @param itemIndex O índice do sensor (0..5)
 * @return Umidade do solo estimada (0 a 100.0)
 */
float readNodeItem(uint8_t itemIndex) {
  if (itemIndex >= numItems) {
    return NAN; // Índice inválido
  }

  // Se for o início da varredura, garante que os pull-ups estejam energizados
  if (itemIndex == 0) {
    powerUpSensors();
  }

  uint8_t pinToRead = sensorPins[itemIndex];

  // Pequeno delay para estabilização elétrica do canal a ser lido
  delay(5);

  // Primeira leitura para purgar a carga acumulada no Sample and Hold do ADC
  analogRead(pinToRead);
  delay(3);

  // Leitura real
  int rawAdc = analogRead(pinToRead);

  Serial.print(F("CH"));
  Serial.print(itemIndex);
  Serial.print(F(" (Pin A"));
  Serial.print(itemIndex);
  Serial.print(F(") ADC:"));
  Serial.println(rawAdc);

  // Compensação de divisor de tensão (Pull-up de 10k conectado ao
  // PIN_POWER_SENSORS)
  float rPullup = 10000.0f; // Pull-up de 10kΩ

  if (rawAdc >= 1023) {
    rawAdc = 1022; // Evita divisão por zero
  }

  // Calcula a resistência real do solo (sem multiplexador, Ron é 0.0f)
  float rSolo = rPullup * ((float)rawAdc / (1023.0f - (float)rawAdc));
  if (rSolo < 0.0f) {
    rSolo = 0.0f;
  }

  // Converte para porcentagem equivalente: R_pullup / (R_pullup + R_solo) *
  // 100.0%
  float percentage = (rPullup / (rPullup + rSolo)) * 100.0f;

  // Limita o valor final entre 0 e 100
  if (percentage < 0.0f) {
    percentage = 0.0f;
  }
  if (percentage > 100.0f) {
    percentage = 100.0f;
  }

  // Ao final da varredura (último sensor, index 5), desliga os sensores
  if (itemIndex == numItems - 1) {
    powerDownSensors();
  }

  return percentage;
}

// Hooks do M360-DRY para acordar e dormir
namespace M360 {
void powerUp() { powerUpSensors(); }
void powerDown() { powerDownSensors(); }
} // namespace M360

void before() {
  Serial.begin(MY_BAUD_RATE);
  initSensors();
}

void presentation() { node.begin("02nodeSolo3dNano", "1.0"); }

void setup() {
  // Registra o callback de leitura analógica
  node.onRead(readNodeItem);
}

void loop() {
  // Processa temporizadores e ciclo de dormir/acordar/ler/enviar
  node.process();
}

void receive(const MyMessage &msg) {
  // Trata comandos de alteração de intervalo e force update
  node.handleMessage(msg);
}
