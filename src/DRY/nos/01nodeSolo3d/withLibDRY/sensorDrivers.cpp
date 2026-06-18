#include "sensorDrivers.h"

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

void powerUpSensors() {
	// Energiza a barra de resistores de pull-up dos sensores
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	
	// Tempo de estabilização do MUX e das capacitâncias parasitas nos cabos longos
	delay(20);
}

void powerDownSensors() {
	// Desliga a alimentação para cessar corrente e evitar eletrólise nos eletrodos
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

static void selectMuxChannel(uint8_t channel) {
	digitalWrite(MUX_PIN_S0, (channel & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S1, (channel & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S2, (channel & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S3, (channel & 0x08) ? HIGH : LOW);
}

float readNodeItem(uint8_t itemIndex) {
	uint8_t pinToRead = MUX_PIN_SIG;
	float rOn = 0.0f; // Sem multiplexador por padrão (canais nativos)
	
	// Mapeia o índice para o canal correto
	if (itemIndex < 16) {
		selectMuxChannel(itemIndex);
		rOn = 70.0f; // Resistência interna típica (Ron) do CD74HC4067
	} else if (itemIndex == 16) {
		pinToRead = PIN_NATIVE_A1;
	} else if (itemIndex == 17) {
		pinToRead = PIN_NATIVE_A2;
	} else {
		return NAN; // Índice inválido
	}

	// Pequeno delay para estabilização elétrica do canal selecionado
	delay(5);

	// Primeira leitura para purgar a carga acumulada no Sample and Hold do ADC
	analogRead(pinToRead);
	delay(3);
	
	// Leitura real
	int rawAdc = analogRead(pinToRead);

	// Compensação de divisor de tensão (Opção B - pull-up de 10k único no SIG)
	float rPullup = 10000.0f; // Pull-up de 10kΩ
	
	if (rawAdc >= 1023) {
		rawAdc = 1022; // Evita divisão por zero
	}
	
	// Calcula a resistência real do solo descontando Ron do multiplexador
	float rSolo = rPullup * ((float)rawAdc / (1023.0f - (float)rawAdc)) - rOn;
	if (rSolo < 0.0f) {
		rSolo = 0.0f;
	}

	// Converte para porcentagem equivalente: R_pullup / (R_pullup + R_solo) * 100.0%
	float percentage = (rPullup / (rPullup + rSolo)) * 100.0f;
	
	// Limita o valor final entre 0 e 100
	if (percentage < 0.0f) {
		percentage = 0.0f;
	}
	if (percentage > 100.0f) {
		percentage = 100.0f;
	}

	return percentage;
}
