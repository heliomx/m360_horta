#include "sensorDrivers.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Instâncias globais OneWire e DallasTemperature
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

// Variáveis globais do driver de vazão
volatile uint32_t pulseCount = 0;

void pulseCounter()
{
	pulseCount++;
}

void initSensors()
{
	pinMode(PIN_POWER_SENSORS, OUTPUT);
	digitalWrite(PIN_POWER_SENSORS, LOW); // Inicia desativado (VCC cortado)
	
	// Ultrassônico
	pinMode(PIN_TRIG, OUTPUT);
	pinMode(PIN_ECHO, INPUT);
	digitalWrite(PIN_TRIG, LOW);
	
	// Analógicos
	pinMode(PIN_PH, INPUT);
	pinMode(PIN_EC, INPUT);
	
	// Vazão (Configura Pullup interno para sinal digital limpo do sensor de efeito Hall)
	pinMode(PIN_FLOW_SENSOR, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PIN_FLOW_SENSOR), pulseCounter, FALLING);
}

void powerUpSensors()
{
	// Energiza o barramento D3 dos sensores
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	// Aguarda estabilização elétrica dos circuitos integrados de condicionamento de sinal
	delay(100);
	// Inicializa barramento OneWire pós-energia
	sensors.begin();
}

void powerDownSensors()
{
	// Corta a alimentação do barramento D3
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

float readNodeItem(uint8_t itemIndex)
{
	switch (itemIndex) {
		case 0: { // Nível (Ultrassônico)
			// Dispara pulso de Trigger de 10us
			digitalWrite(PIN_TRIG, LOW);
			delayMicroseconds(2);
			digitalWrite(PIN_TRIG, HIGH);
			delayMicroseconds(10);
			digitalWrite(PIN_TRIG, LOW);
			
			// Mede o pulso de Echo (limite de 30ms = ~5 metros de alcance máximo)
			long duration = pulseIn(PIN_ECHO, HIGH, 30000);
			if (duration == 0) {
				return NAN;
			}
			
			// Distância em centímetros: velocidade do som = 343 m/s => 0.0343 cm/us
			float distance = (duration * 0.0343f) / 2.0f;
			
			// Constantes de calibração para nível da caixa d'água
			const float CAIXA_MAX_HEIGHT_CM = 100.0f; // Distância do sensor ao fundo (Vazia)
			const float CAIXA_MIN_HEIGHT_CM = 10.0f;  // Distância do sensor à água cheia (Cheia)
			
			// Converte para percentual de 0% a 100%
			float levelPercent = (CAIXA_MAX_HEIGHT_CM - distance) * 100.0f / (CAIXA_MAX_HEIGHT_CM - CAIXA_MIN_HEIGHT_CM);
			
			if (levelPercent < 0.0f) {
				levelPercent = 0.0f;
			}
			if (levelPercent > 100.0f) {
				levelPercent = 100.0f;
			}
			
			return levelPercent;
		}
		
		case 1: { // pH (Analógico A0)
			long sum = 0;
			for (int i = 0; i < 10; i++) {
				sum += analogRead(PIN_PH);
				delay(5);
			}
			float avgRaw = sum / 10.0f;
			float voltage = (avgRaw * 5.0f) / 1023.0f;
			
			// Equação de calibração do sensor pH-4502C:
			// Ajustar PH_SLOPE e PH_OFFSET conforme calibração com soluções tampão (pH 4 e 7)
			const float PH_SLOPE = -5.70f;
			const float PH_OFFSET = 21.34f;
			
			float phValue = PH_SLOPE * voltage + PH_OFFSET;
			
			if (phValue < 0.0f) {
				phValue = 0.0f;
			}
			if (phValue > 14.0f) {
				phValue = 14.0f;
			}
			
			return phValue;
		}
		
		case 2: { // EC (Analógico A1)
			long sum = 0;
			for (int i = 0; i < 10; i++) {
				sum += analogRead(PIN_EC);
				delay(5);
			}
			float avgRaw = sum / 10.0f;
			float voltage = (avgRaw * 5.0f) / 1023.0f;
			
			// Equação linear padrão para condutividade elétrica
			const float EC_SLOPE = 1.0f;
			const float EC_OFFSET = 0.0f;
			
			float ecValue = EC_SLOPE * voltage + EC_OFFSET;
			
			if (ecValue < 0.0f) {
				ecValue = 0.0f;
			}
			
			return ecValue;
		}
		
		case 3: { // Vazão (Litros por segundo - L/s)
			static unsigned long lastFlowTime = 0;
			unsigned long now = millis();
			
			if (lastFlowTime == 0) {
				lastFlowTime = now;
				return 0.0f;
			}
			
			unsigned long duration = now - lastFlowTime;
			if (duration < 200) {
				return 0.0f; // Evita medições instáveis em intervalos muito pequenos
			}
			
			// Seção crítica para ler e resetar a contagem de pulsos
			noInterrupts();
			uint32_t pulses = pulseCount;
			pulseCount = 0;
			interrupts();
			
			lastFlowTime = now;
			
			// Frequência de pulsos (Hz)
			float hz = (pulses * 1000.0f) / (float)duration;
			
			// YF-S201: F (Hz) = 7.5 * Q (L/min) => Q (L/min) = Hz / 7.5
			// Q (L/s) = Q (L/min) / 60 => Q (L/s) = Hz / (7.5 * 60) = Hz / 450.0f
			float flowRate = hz / 450.0f;
			
			return flowRate;
		}
		
		case 4: { // Temperatura da Água (DS18B20 - OneWire)
			sensors.requestTemperatures();
			float tempC = sensors.getTempCByIndex(0);
			
			// Validação do estado do sensor e leituras fora dos limites físicos da água
			if (tempC == DEVICE_DISCONNECTED_C || tempC < -10.0f || tempC > 100.0f) {
				return NAN;
			}
			return tempC;
		}
		
		default:
			return NAN;
	}
}

void writeNodeItem(uint8_t childId, bool state)
{
	// Este nó não possui atuadores nesta versão
	(void)childId;
	(void)state;
}
