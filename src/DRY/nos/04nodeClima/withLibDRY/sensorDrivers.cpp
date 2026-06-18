#include "sensorDrivers.h"

// Objeto Global do DHT
DHT dht(DHTPIN, DHTTYPE);

void initSensors() {
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW); // Repouso = Desligado
	
	pinMode(PIN_LDR, INPUT);
}

void powerUpSensors() {
	// Liga o barramento de alimentação (VCC) dos sensores
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	
	// Aguarda startup do DHT22 (Datasheet: necessita 2 segundos pós-boot)
	delay(2000);
	
	dht.begin();
}

void powerDownSensors() {
	// Desliga o barramento de alimentação (VCC)
	digitalWrite(PIN_POWER_SENSORS, LOW);
	
	// Aterra o pino de dados do DHT22 para evitar fuga de corrente reversa (parasitic powering)
	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, LOW);
}

float readNodeItem(uint8_t itemIndex) {
	if (itemIndex == 0) {
		float t = dht.readTemperature();
		return isnan(t) ? NAN : t;
	} else if (itemIndex == 1) {
		float h = dht.readHumidity();
		return isnan(h) ? NAN : h;
	} else if (itemIndex == 2) {
		// Leitura analógica do LDR (0 a 1023)
		int rawLdr = analogRead(PIN_LDR);
		
		// Divisor de tensão assumido: VCC -> LDR -> A0 -> Resistor(10k) -> GND
		// Mais luz = Resistência LDR cai = A0 se aproxima do VCC = 1023 (100%)
		// Menos luz = Resistência LDR sobe = A0 se aproxima do GND = 0 (0%)
		float percentage = ((float)rawLdr / 1023.0f) * 100.0f;
		
		if (percentage < 0.0f) percentage = 0.0f;
		if (percentage > 100.0f) percentage = 100.0f;
		
		return percentage;
	}
	
	return NAN;
}
