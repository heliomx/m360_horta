/*
 * 01nodeSolo3dNano.cpp — Nó 01: Monitoramento 3D de Solo (Versão Arduino Nano)
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
#include <MySensors.h>
#include <M360.h>

// Pinos nativos do Arduino Nano para os 6 sensores resistivos
static const uint8_t sensorPins[] = {A0, A1, A2, A3, A4, A5};

// Pino de Energia dos Pull-ups (Mitigação de Eletrólise)
#define PIN_POWER_SENSORS 3 // D3

// Definição dos 6 sensores resistivos (Canteiro A)
// Child IDs conforme R13 (1–10: Sensores de Solo)
static const M360::M360ItemDef nodeItems[] = {
    {1, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A0, 0, 1, "A_1m_10cm", false, 0},
    {2, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A1, 0, 1, "A_1m_30cm", false, 0},
    {3, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A2, 0, 1, "A_3m_10cm", false, 0},
    {4, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A3, 0, 1, "A_3m_30cm", false, 0},
    {5, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A4, 0, 1, "A_5m_10cm", false, 0},
    {6, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, A5, 0, 1, "A_5m_30cm", false, 0}
};

static const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

// +3 OBRIGATÓRIO: M360Node escreve [numItems], [numItems+1] e [numItems+2] —
// intervalo (254), bateria (255) e debug remoto (253). Dimensionar com +2 causa
// escrita fora do array em begin() e vazamento de RAM adjacente em sendDebug().
static MyMessage messages[numItems + 3];
static float lastValues[numItems];
static uint8_t nNoUpdates[numItems];

static M360::M360Node node(nodeItems, numItems, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

/**
 * Configura os pinos de controle (Power) como OUTPUT, e pinos analógicos como INPUT.
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
  // Desliga a alimentação para cessar corrente e evitar eletrólise nos eletrodos
  digitalWrite(PIN_POWER_SENSORS, LOW);
}

/**
 * Lê o sensor resistivo específico de forma direta (sem MUX).
 * @param itemIndex O índice do item em nodeItems[] (0..5)
 * @return Leitura do ADC
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
  Serial.print(F(" (Child "));
  Serial.print(nodeItems[itemIndex].childId);
  Serial.print(F(", Pin A"));
  Serial.print(itemIndex);
  Serial.print(F(") ADC:"));
  Serial.println(rawAdc);

  // Ao final da varredura (último sensor, index 5), desliga os sensores
  if (itemIndex == numItems - 1) {
    powerDownSensors();
  }

  return (float)rawAdc;
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

void presentation() { node.begin("01nodeSolo3dNano", "2.0.0"); }

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

