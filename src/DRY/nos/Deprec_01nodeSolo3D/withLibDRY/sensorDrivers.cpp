#include "sensorDrivers.h"

void initSensors() {
	// Pino de Alimentação (Pull-ups)
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW); // Mantém desligado por padrão
	
	// Pinos de controle do MUX
	pinMode(MUX_PIN_S0, OUTPUT);
	pinMode(MUX_PIN_S1, OUTPUT);
	pinMode(MUX_PIN_S2, OUTPUT);
	pinMode(MUX_PIN_S3, OUTPUT);
	
	// Pinos de leitura
	pinMode(MUX_PIN_SIG, INPUT);
	pinMode(PIN_NATIVE_A1, INPUT);
	pinMode(PIN_NATIVE_A2, INPUT);
}

static void selectMuxChannel(uint8_t channel) {
	digitalWrite(MUX_PIN_S0, (channel & 0x01) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S1, (channel & 0x02) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S2, (channel & 0x04) ? HIGH : LOW);
	digitalWrite(MUX_PIN_S3, (channel & 0x08) ? HIGH : LOW);
}

float readNodeItem(uint8_t itemIndex) {
	// Seleciona o pino de leitura com base no índice
	// 0 a 15 = Canais do MUX Analógico (A0)
	// 16 = Pino Nativo A1
	// 17 = Pino Nativo A2
	uint8_t pinToRead = MUX_PIN_SIG;
	
	if (itemIndex < 16) {
		selectMuxChannel(itemIndex);
	} else if (itemIndex == 16) {
		pinToRead = PIN_NATIVE_A1;
	} else if (itemIndex == 17) {
		pinToRead = PIN_NATIVE_A2;
	} else {
		return NAN; // Índice inválido
	}

	// 1. Liga os pull-ups para fechar o circuito resistivo no solo
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	
	// 2. Tempo de estabilização do MUX e das capacitâncias parasitas do cabo (20ms)
	delay(20); 

	// 3. Primeira leitura para purgar a capacitância do S/H (Sample and Hold) do ADC
	analogRead(pinToRead);
	delay(5);
	
	// 4. Leitura real
	int rawAdc = analogRead(pinToRead);

	// 5. Desliga os pull-ups para cessar a corrente e evitar eletrólise
	digitalWrite(PIN_POWER_SENSORS, LOW);

	// Mapeamento do ADC para Porcentagem (0-100%)
	// Solo seco = Alta resistência = ADC alto (próximo a 1023) -> 0%
	// Solo úmido = Baixa resistência = ADC baixo (próximo a 0) -> 100%
	float percentage = 100.0f - ((float)rawAdc / 1023.0f * 100.0f);
	
	// Garantir que os valores fiquem entre 0 e 100
	if (percentage < 0.0f) percentage = 0.0f;
	if (percentage > 100.0f) percentage = 100.0f;

	return percentage;
}
