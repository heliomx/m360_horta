/*
 * lpDrivers.cpp — Orquestração da camada física da M360-LP
 *
 * Liga e desliga o trilho VSW e despacha cada leitura ao driver certo. As
 * regras que o hardware impõe (ARQUITETURA_FAMILIA.md §4.2) vivem aqui:
 *   - com o VSW desligado, nenhum pino ligado ao campo fica em HIGH
 *     (S0–S2 do 4051, A1, A2, A4/A5); um pino alto alimenta os sensores
 *     pelos diodos de proteção;
 *   - S0–S2 vão a LOW ANTES de cortar o VSW.
 */

#include "nodeConfig.h"   // flags LP_HAS_* e LP_POWER_* do nó (via -I do env)
#include "lpDrivers.h"
#include <M360.h>         // wait() da MySensors sem reincluir MySensors.h

static bool vswOn = false;

void initSensors()
{
	// VSW desligado no boot: o gate já tem 100 kΩ para o 3V3; o firmware confirma.
	digitalWrite(LP_PIN_VSW_EN, LP_VSW_OFF);
	pinMode(LP_PIN_VSW_EN, OUTPUT);
	vswOn = false;

	// Reed (SW1) do D3 ao GND, contato normalmente aberto: pull-up interno, zero
	// corrente com o contato aberto. Só drena (~100 µA) enquanto o ímã estiver perto.
	pinMode(LP_PIN_REED, INPUT_PULLUP);

#if LP_HAS_SOIL
	soilMuxInit();
#endif
#if LP_HAS_DHT
	dhtPowerDown();
#endif
#if LP_HAS_ONEWIRE
	ds18PowerDown();
#endif
}

void powerUpSensors()
{
	if (vswOn) {
		return;
	}
	digitalWrite(LP_PIN_VSW_EN, LP_VSW_ON);
	vswOn = true;

	// Partida suave do VSW (10 kΩ / 10 nF no gate ≈ 100 µs) mais a carga dos
	// cabos: 20 ms cobre as capacitâncias parasitas das sondas.
	wait(20);

#if LP_HAS_DHT
	dhtPowerUp();
#endif
#if LP_HAS_ONEWIRE
	ds18PowerUp();
#endif
#if LP_HAS_DHT || LP_HAS_ONEWIRE
	// DHT11 precisa de 1 a 1,5 s depois de alimentado; o DS18B20 converte em
	// 750 ms. Uma espera cobre os dois (mesmo valor do Nó 04).
	wait(1500);
#endif
}

void powerDownSensors()
{
	if (!vswOn) {
		return;
	}
#if LP_HAS_SOIL
	soilMuxIdle();
#endif
#if LP_HAS_DHT
	dhtPowerDown();
#endif
#if LP_HAS_ONEWIRE
	ds18PowerDown();
#endif
	digitalWrite(LP_PIN_VSW_EN, LP_VSW_OFF);
	vswOn = false;
}

float lpReadSource(int src)
{
#if LP_HAS_SOIL
	if (LP_IS_SOIL(src)) {
		return soilMuxRead(LP_SOIL_CH(src));
	}
#endif
#if LP_HAS_DHT
	if (src == LP_SRC_DHT_TEMP) {
		return dhtReadTemp();
	}
	if (src == LP_SRC_DHT_HUM) {
		return dhtReadHum();
	}
#endif
#if LP_HAS_ONEWIRE
	if (src == LP_SRC_DS18B20) {
		return ds18Read();
	}
#endif
	return NAN; // fonte sem driver compilado: item mal configurado no nodeConfig.h
}
