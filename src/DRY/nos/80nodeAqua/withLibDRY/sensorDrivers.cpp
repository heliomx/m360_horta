#include "sensorDrivers.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Instâncias globais OneWire e DallasTemperature
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

// Variáveis globais do driver de vazão
volatile uint32_t pulseCount = 0;
volatile uint32_t pulseCountA = 0;
volatile uint32_t pulseCountB = 0;
volatile uint32_t pulseCountC = 0;

// Leituras de vazão e flags compartilhadas
float currentFlowH = 0.0f;
float currentFlowA = 0.0f;
float currentFlowB = 0.0f;
float currentFlowC = 0.0f;
bool allowAquaRead = false;

void pulseCounter()
{
	pulseCount++;
}

void pulseCounterA()
{
	pulseCountA++;
}

// PCINT para o Sensor B (Pino D8 / PB0)
ISR(PCINT0_vect)
{
	static uint8_t lastPortState = PINB;
	uint8_t currentPortState = PINB;
	uint8_t changedPins = currentPortState ^ lastPortState;
	lastPortState = currentPortState;
	
	// Detecta borda de descida (FALLING) no pino D8 (PB0 / PCINT0)
	if ((changedPins & (1 << PB0)) && !(currentPortState & (1 << PB0))) {
		pulseCountB++;
	}
}

// PCINT para o Sensor C (Pino A2 / PC2 / PCINT10)
ISR(PCINT1_vect)
{
	static uint8_t lastPortState = PINC;
	uint8_t currentPortState = PINC;
	uint8_t changedPins = currentPortState ^ lastPortState;
	lastPortState = currentPortState;
	
	// Detecta borda de descida (FALLING) no pino A2 (PC2 / PCINT10)
	if ((changedPins & (1 << PC2)) && !(currentPortState & (1 << PC2))) {
		pulseCountC++;
	}
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
	
	// Vazão 1 (Configura Pullup interno para sinal digital limpo do sensor de efeito Hall)
	pinMode(PIN_FLOW_SENSOR, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PIN_FLOW_SENSOR), pulseCounter, FALLING);

	// Vazão A (Configura Pullup interno e utiliza interrupção externa INT1 no D3)
	pinMode(PIN_FLOW_SENSOR_A, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PIN_FLOW_SENSOR_A), pulseCounterA, FALLING);

	// Vazão B e C (Configura Pullup interno e habilita PCINT)
	pinMode(PIN_FLOW_SENSOR_B, INPUT_PULLUP);
	pinMode(PIN_FLOW_SENSOR_C, INPUT_PULLUP);

	// Habilita PCINT0 para o Port B (D8 / PB0)
	PCICR |= (1 << PCIE0);      // Habilita interrupções de Pin Change para Port B
	PCMSK0 |= (1 << PCINT0);    // Habilita PCINT0 específico do pino D8

	// Habilita PCINT1 para o Port C (A2 / PC2 / PCINT10)
	PCICR |= (1 << PCIE1);      // Habilita interrupções de Pin Change para Port C
	// D6-fix: PCINT10 = bit 2 de PCMSK1 (não bit 10 que truncaria para 0x00 em uint8_t)
	PCMSK1 |= _BV(PC2);         // Habilita PCINT10 (PC2/A2) corretamente
}

void powerUpSensors()
{
	digitalWrite(PIN_POWER_SENSORS, HIGH);
	delay(100);
	sensors.begin();
	// Conversão assíncrona: getTempCByIndex() lê sem bloquear 750 ms
	sensors.setWaitForConversion(false);
	sensors.requestTemperatures();
}

void powerDownSensors()
{
	// Corta a alimentação do barramento D7 / PIN_POWER_SENSORS
	digitalWrite(PIN_POWER_SENSORS, LOW);
}

static float calculateFlow(volatile uint32_t &pCount, unsigned long &lastTime)
{
	unsigned long now = millis();
	
	if (lastTime == 0) {
		lastTime = now;
		return 0.0f;
	}
	
	unsigned long duration = now - lastTime;
	if (duration < 200) {
		return 0.0f; // Evita medições instáveis em intervalos muito pequenos
	}
	
	// Seção crítica para ler e resetar a contagem de pulsos
	noInterrupts();
	uint32_t pulses = pCount;
	pCount = 0;
	interrupts();
	
	lastTime = now;
	
	// Frequência de pulsos (Hz)
	float hz = (pulses * 1000.0f) / (float)duration;
	
	// YF-S201: F (Hz) = 7.5 * Q (L/min) => Q (L/min) = Hz / 7.5
	// Q (L/s) = Q (L/min) / 60 => Q (L/s) = Hz / (7.5 * 60) = Hz / 450.0f
	float flowRate = hz / 450.0f;
	
	return flowRate;
}

void updateFlows()
{
	static unsigned long lastUpdate = 0;
	unsigned long now = millis();
	
	// Atualiza a cada 2 segundos
	if (lastUpdate == 0 || (now - lastUpdate) >= 2000) {
		lastUpdate = now;
		static unsigned long lastFlowTime = 0;
		static unsigned long lastFlowTimeA = 0;
		static unsigned long lastFlowTimeB = 0;
		static unsigned long lastFlowTimeC = 0;

		currentFlowH = calculateFlow(pulseCount, lastFlowTime);
		currentFlowA = calculateFlow(pulseCountA, lastFlowTimeA);
		currentFlowB = calculateFlow(pulseCountB, lastFlowTimeB);
		currentFlowC = calculateFlow(pulseCountC, lastFlowTimeC);

		// Inicia conversão DS18B20 assíncrona (resultado disponível em ~750 ms)
		sensors.requestTemperatures();
	}
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
			if (!allowAquaRead) {
				return NAN;
			}
			long sum = 0;
			for (int i = 0; i < 10; i++) {
				sum += analogRead(PIN_PH);
				// AVR ADC demora ~104 µs por conversão; delay adicional desnecessário
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
			if (!allowAquaRead) {
				return NAN;
			}
			long sum = 0;
			for (int i = 0; i < 10; i++) {
				sum += analogRead(PIN_EC);
				// AVR ADC demora ~104 µs por conversão; delay adicional desnecessário
			}
			float avgRaw = sum / 10.0f;
			float voltage = (avgRaw * 5.0f) / 1023.0f;
			
			// TODO: calibrar com solução padrão (ex.: KCl 1413 μS/cm).
			// Valores abaixo são placeholder — retornam tensão em V, não μS/cm.
			const float EC_SLOPE  = 1.0f;
			const float EC_OFFSET = 0.0f;

			float ecValue = EC_SLOPE * voltage + EC_OFFSET;
			
			if (ecValue < 0.0f) {
				ecValue = 0.0f;
			}
			
			return ecValue;
		}
		
		case 3: { // Vazão H (Litros por segundo - L/s)
			return currentFlowH;
		}
		
		case 4: { // Temperatura da Água (DS18B20 - OneWire)
			if (!allowAquaRead) {
				return NAN;
			}
			// Conversão já foi requisitada em updateFlows() (assíncrona)
			if (!sensors.isConversionComplete()) {
				return NAN;
			}
			float tempC = sensors.getTempCByIndex(0);
			
			// Validação do estado do sensor e leituras fora dos limites físicos da água
			if (tempC == DEVICE_DISCONNECTED_C || tempC < -10.0f || tempC > 100.0f) {
				return NAN;
			}
			return tempC;
		}
		
		case 5: { // Vazão A (Litros por segundo - L/s)
			return currentFlowA;
		}
		
		case 6: { // Vazão B (Litros por segundo - L/s)
			return currentFlowB;
		}
		
		case 7: { // Vazão C (Litros por segundo - L/s)
			return currentFlowC;
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
