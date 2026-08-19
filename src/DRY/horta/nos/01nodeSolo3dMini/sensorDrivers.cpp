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
	// Energiza a barra de resistores de pull-up dos sensores se estiver desligada
	if (digitalRead(PIN_POWER_SENSORS) == LOW) {
		digitalWrite(PIN_POWER_SENSORS, HIGH);
		// Tempo de estabilização do MUX e das capacitâncias parasitas nos cabos longos
		delay(20);
	}
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
	// Se for o início da varredura, garante que os pull-ups estejam energizados
	if (itemIndex == 0) {
		powerUpSensors();
	}

	uint8_t pinToRead = MUX_PIN_SIG;
	
	// Mapeia o índice para o canal correto
	if (itemIndex < 16) {
		selectMuxChannel(itemIndex);
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

	// DEBUG TEMPORÁRIO: valor bruto do ADC por canal, para diagnosticar o MUX.
	// Se travado perto de 1023 (ou 0) em todos os canais 0-15 enquanto 16/17
	// variam normalmente, suspeitar do pino EN do CD74HC4067 (deve ir ao GND).
	Serial.print(F("CH"));
	Serial.print(itemIndex);
	Serial.print(F(" ADC:"));
	Serial.println(rawAdc);

	// Ao final da varredura (último sensor, index 17), desliga os sensores
	if (itemIndex == 17) {
		powerDownSensors();
	}

	return (float)rawAdc;
}
