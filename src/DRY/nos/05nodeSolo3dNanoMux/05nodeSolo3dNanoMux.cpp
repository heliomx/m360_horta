/*
 * 05nodeSolo3dNanoMux.cpp — Nó 05: Monitoramento 3D de Solo Unificado (Canteiros A e B)
 *
 * Hardware: Arduino Nano (5V / 16MHz) + CD74HC4067 (MUX Analógico) + 18
 * Sensores Resistivos + nRF24L01+ (CE=D9, CSN=D10)
 * Alimentação: Fonte fixa ou bateria com carga contínua
 *
 * Perfil: M360_ALWAYS_ON — timer por millis(), sem sleep.
 * O pino D3 (PIN_POWER_SENSORS) é desligado entre leituras para
 * mitigar a eletrólise dos eletrodos no solo.
 */

#include <Arduino.h>
#include <MySensors.h>
#include <M360.h>

// ===== PINOS DO MULTIPLEXADOR CD74HC4067 E NATIVOS =====
#define MUX_PIN_SIG A0
#define MUX_PIN_S0  4
#define MUX_PIN_S1  5
#define MUX_PIN_S2  6
#define MUX_PIN_S3  7

// Pino de Energia dos Pull-ups (Mitigação de Eletrólise)
#define PIN_POWER_SENSORS 3 // D3

// Pinos Nativos Analógicos
#define PIN_NATIVE_A1 A1
#define PIN_NATIVE_A2 A2

// ===== DEFINIÇÃO DOS 18 SENSORES DE UMIDADE DE SOLO =====
static const M360::M360ItemDef nodeItems[] = {
    // Canteiro A - 9 sensores (Child IDs 0 a 8)
    {0, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_10cm", false, 0},
    {1, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_20cm", false, 0},
    {2, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_1m_30cm", false, 0},
    {3, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_10cm", false, 0},
    {4, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_20cm", false, 0},
    {5, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_3m_30cm", false, 0},
    {6, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_10cm", false, 0},
    {7, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_20cm", false, 0},
    {8, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "A_5m_30cm", false, 0},
    // Canteiro B - 9 sensores (Child IDs 9 a 17)
    {9, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_10cm", false, 0},
    {10, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_20cm", false, 0},
    {11, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_1m_30cm", false, 0},
    {12, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_10cm", false, 0},
    {13, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_20cm", false, 0},
    {14, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_3m_30cm", false, 0},
    {15, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_10cm", false, 0},
    {16, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_20cm", false, 0},
    {17, M360::M360_SENSOR, S_MOISTURE, V_LEVEL, -1, 0, 1, "B_5m_30cm", false, 0}
};

static const uint8_t numItems = sizeof(nodeItems) / sizeof(M360::M360ItemDef);

static MyMessage messages[numItems + 3]; // +1 Intervalo (254) +1 Bateria (255) +1 Debug (253)
static float     lastValues[numItems];
static uint8_t   nNoUpdates[numItems];

// Instanciação do nó no modo ALWAYS_ON
static M360::M360Node node(nodeItems, numItems, messages, lastValues,
                           nNoUpdates, M360::M360_ALWAYS_ON);

// ===== FUNÇÕES DE CONTROLE DE HARDWARE =====

/**
 * Configura os pinos de controle do MUX e barramento de energia.
 */
void initSensors() {
	// Configura o pino de energia (pull-ups) como saída e desliga por padrão
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW);

	// Configura os pinos de controle do MUX como saída
	pinMode(MUX_PIN_S0, OUTPUT);
	pinMode(MUX_PIN_S1, OUTPUT);
	pinMode(MUX_PIN_S2, OUTPUT);
	pinMode(MUX_PIN_S3, OUTPUT);

	// Configura os pinos de leitura analógica como entrada
	pinMode(MUX_PIN_SIG, INPUT);
	pinMode(PIN_NATIVE_A1, INPUT);
	pinMode(PIN_NATIVE_A2, INPUT);
}

/**
 * Liga a alimentação de pull-up dos sensores e aguarda a estabilização elétrica.
 */
void powerUpSensors() {
	if (digitalRead(PIN_POWER_SENSORS) == LOW) {
		digitalWrite(PIN_POWER_SENSORS, HIGH);
		// Tempo de estabilização do MUX e das capacitâncias parasitas nos cabos longos
		delay(20);
	}
}

/**
 * Desliga a alimentação de pull-up para mitigar a eletrólise.
 */
void powerDownSensors() {
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

/**
 * Seleciona o canal do CD74HC4067 usando os pinos S0-S3.
 */
static void selectMuxChannel(uint8_t channel) {
	digitalWrite(MUX_PIN_S0, (channel & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S1, (channel & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S2, (channel & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S3, (channel & 0x08) ? HIGH : LOW);
}

/**
 * Callback de leitura de sensores.
 * Mapeia e compensa o sinal analógico em porcentagem (0% seco -> 100% molhado).
 */
float readNodeItem(uint8_t itemIndex) {
	if (itemIndex >= numItems) {
		return NAN;
	}

	// Se for o início da varredura, garante que a barra de pull-up está energizada
	if (itemIndex == 0) {
		powerUpSensors();
	}

	uint8_t pinToRead = MUX_PIN_SIG;

	// Seleciona o canal físico correto
	if (itemIndex < 16) {
		selectMuxChannel(itemIndex);
	} else if (itemIndex == 16) {
		pinToRead = PIN_NATIVE_A1;
	} else if (itemIndex == 17) {
		pinToRead = PIN_NATIVE_A2;
	}

	// Pequeno delay para estabilização elétrica da impedância do canal
	delay(5);

	// Leitura de purga do capacitor Sample and Hold do ADC
	analogRead(pinToRead);
	delay(3);

	// Leitura real
	int rawAdc = analogRead(pinToRead);

	Serial.print(F("CH"));
	Serial.print(itemIndex);
	Serial.print(F(" ADC:"));
	Serial.print(rawAdc);

	// Cálculo do Divisor de Tensão e compensação do MUX
	float rPullup = 10000.0f; // Pull-up fixo de 10kΩ
	float rOn = 70.0f;        // Resistência de canal ativo do CD74HC4067 (~70Ω)

	if (rawAdc >= 1023) {
		rawAdc = 1022; // Evita divisão por zero
	}

	float rSolo;
	if (itemIndex < 16) {
		// Compensando a resistência Ron em série do multiplexador
		rSolo = rPullup * ((float)rawAdc / (1023.0f - (float)rawAdc)) - rOn;
	} else {
		// Canais nativos sem multiplexador
		rSolo = rPullup * ((float)rawAdc / (1023.0f - (float)rawAdc));
	}

	if (rSolo < 0.0f) {
		rSolo = 0.0f;
	}

	// Converte para porcentagem de umidade equivalente: R_pullup / (R_pullup + R_solo) * 100.0%
	float percentage = (rPullup / (rPullup + rSolo)) * 100.0f;

	if (percentage < 0.0f) {
		percentage = 0.0f;
	}
	if (percentage > 100.0f) {
		percentage = 100.0f;
	}

	Serial.print(F(" -> Solo: "));
	Serial.print(rSolo, 1);
	Serial.print(F(" R | Umid: "));
	Serial.print(percentage, 1);
	Serial.println(F("%"));

	// Ao terminar a leitura de todos os canais, desliga os sensores
	if (itemIndex == numItems - 1) {
		powerDownSensors();
	}

	return percentage;
}

// ===== GANCHOS DO M360-DRY =====

namespace M360 {
void powerUp() {
	powerUpSensors();
}
void powerDown() {
	powerDownSensors();
}
} // namespace M360

// ===== MYSENSORS HOOKS =====

void before() {
	Serial.begin(MY_BAUD_RATE);
	initSensors();
}

void presentation() {
	node.begin("05nodeSolo3dNanoMux", "1.0");
}

void setup() {
	node.onRead(readNodeItem);
}

void loop() {
	node.process();
}

void receive(const MyMessage &msg) {
	node.handleMessage(msg);
}
