/*
 * SU_xxT.h — Classe principal da biblioteca SU-xxT
 *
 * Padrão de API da DallasTemperature/OneWire: disparo global de conversão
 * (requestReadings), acesso por índice e constantes de erro.
 *
 * USO (nó M360-DRY):
 *   1. Instancie SU_Device como estático no sensorDrivers.cpp.
 *   2. Sobrescreva M360::powerUp() DENTRO do namespace M360 chamando
 *      su.powerUp() e su.requestReadings().
 *   3. Ligue o onRead do M360Node a su.getReadingByIndex(IDX_MAP[i]).
 *
 * Perfis suportados: M360_LOW_POWER e M360_PASSIVE.
 * M360_ALWAYS_ON NÃO é suportado — nesse perfil a M360Node nunca chama
 * powerUp(), requestReadings() não rodaria e o nó publicaria cache frio.
 *
 * Não usa String nem std::vector — compatível com ATmega328P.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "SU_Types.h"
#include "SU_Calibration.h"

// Maior número de sensores em um modelo (SU-30T)
#define SU_MAX_SENSORS 6

class SU_Device {
public:
	/*
	 * Constructor.
	 *
	 * @param pinMuxA/B/C   Três linhas de endereço do 74HC4051. São CINCO canais
	 *                      analógicos: duas linhas só alcançariam quatro, e nem o
	 *                      4052 (duplo 4:1) nem o 4053 (triplo SPDT) os endereçam
	 *                      por A/B. Ver ARCHITECTURE.md §3.
	 * @param pinAcExcite   GPIO do trem de pulsos AC (EC e nível condutivo).
	 *                      Fica em alta impedância fora da amostragem — DC em
	 *                      inox dentro do lisímetro eletrolisa o eletrodo e
	 *                      contamina a solução medida. Ver ARCHITECTURE.md §6.
	 * @param pinMosfetPwr  MOSFET canal P. Chaveia APENAS os front-ends; o
	 *                      ADS1115 e o MUX ficam no trilho permanente.
	 *                      Ver ARCHITECTURE.md §4. -1 se não usado.
	 * @param pinOneWire    Barramento do DS18B20. -1 nos modelos sem 'T'.
	 */
	SU_Device(uint8_t pinMuxA, uint8_t pinMuxB, uint8_t pinMuxC,
	          uint8_t pinAcExcite,
	          int8_t  pinMosfetPwr,
	          int8_t  pinOneWire,
	          SU_Model model      = SU_MODEL_30T,
	          uint8_t  i2cAddress = 0x48);

	// ----- Ciclo de vida -----

	// Configura pinos, inicia I2C/ADS1115/1-Wire e carrega a calibração.
	// Retorna false se o ADS1115 não responder ou a calibração estiver inválida
	// (neste caso os defaults de fábrica já foram aplicados).
	bool begin(TwoWire& wirePort = Wire);

	void powerUp();    // liga o trilho dos front-ends
	void powerDown();  // corta o trilho dos front-ends

	// Aquisição em lote: warm-up, reaplicação da config do ADS, leitura
	// multicanal, compensações e cache. BLOQUEANTE — é o custo dominante da
	// janela acordada. Chamar de M360::powerUp().
	void requestReadings();

	// ----- API estilo One-Wire (enumeração / indexação) -----

	uint8_t     getDeviceCount() const { return _count; }
	float       getReadingByIndex(uint8_t index);
	const char* getLabelByIndex(uint8_t index) const;
	const char* getUnitByIndex(uint8_t index) const;
	SU_Channel  getChannelByIndex(uint8_t index) const;

	// ----- Acesso direto por grandeza -----

	float getReading(SU_Channel channel);
	float getMoisture10()      { return getReading(SU_CH_MOISTURE_10CM); }
	float getMoisture30()      { return getReading(SU_CH_MOISTURE_30CM); }
	float getSoilTemperature() { return getReading(SU_CH_TEMP_SOIL); }
	float getEC25()            { return getReading(SU_CH_EC_SOLUTION); }
	float getECRaw()           const { return _ecRaw; }
	float getPH()              { return getReading(SU_CH_PH_SOLUTION); }

	SU_LevelState getLevel() const { return _level; }

	// Câmara com solução E reserva já trocada — só então pH e EC valem.
	bool isChamberValid()   const { return _level == SU_LEVEL_FILLED && _cal.rechargeSettled; }
	bool isRechargeSettled() const { return _cal.rechargeSettled; }

	// Idade da amostra. Exportado para que o consumidor possa trendar a EC
	// contra ele: EC subindo em fase com o contador e caindo no reenchimento é
	// acúmulo de KCl, não salinização do solo. Ver ARCHITECTURE.md §9.
	uint16_t getCyclesSinceRecharge() const { return _cyclesSinceRecharge; }

	// Zera a contagem e invalida a acomodação, forçando pH e EC a esperarem
	// minRechargeCycles de novo.
	//
	// O NÓ deve chamar isto ao alterar o intervalo de reporte (V_VAR1): o tempo
	// físico de troca da reserva é `minRechargeCycles x intervalo`, então baixar
	// o intervalo de 30 para 2 min encolheria a acomodação 15 vezes e liberaria
	// solução estagnada como se fosse nova. Esta lib não conhece MySensors e não
	// tem como detectar a mudança sozinha — daí ser explícito.
	void resetRechargeState();

	// ----- Calibração -----

	SU_CalibrationData& getCalibration() { return _cal; }
	void setCalibration(const SU_CalibrationData& cal);

	bool saveCalibration();   // grava com magic, versão e CRC; só se mudou
	bool loadCalibration();   // valida; em falha carrega defaults e retorna false
	void resetCalibration();  // defaults em RAM, sem gravar

private:
	// --- pinos e identidade ---
	uint8_t  _pinMuxA, _pinMuxB, _pinMuxC;
	uint8_t  _pinAcExcite;
	int8_t   _pinMosfetPwr;
	int8_t   _pinOneWire;
	SU_Model _model;
	uint8_t  _i2cAddress;
	uint8_t  _count;

	// --- periféricos ---
	// Membros por valor, sem alocação dinâmica: OneWire e DallasTemperature têm
	// construtor default e recebem o pino em begin().
	Adafruit_ADS1115  _ads;
	OneWire           _oneWire;
	DallasTemperature _dallas;
	bool              _adsOk;

	// --- estado ---
	SU_CalibrationData _cal;
	float              _cache[SU_MAX_SENSORS];
	float              _ecRaw;
	SU_LevelState      _level;
	uint16_t           _cyclesSinceRecharge;
	bool               _sampled;   // false = cache frio
	uint16_t           _savedCrc;  // CRC do último bloco gravado

	// --- internos ---
	void    _selectChannel(uint8_t muxChannel);
	void    _applyAdcConfig(uint8_t muxChannel);
	int16_t _readRaw(uint8_t muxChannel, bool* ok);
	void    _exciteAC();

	float   _computeMoisture(int16_t raw, int16_t airAdc, int16_t waterAdc);
	float   _computeEC25(float ecRaw, float tempC);
	float   _computePH(float voltage, float tempC);
	float   _nernstSlope(float tempC) const;

	float   _measureTemperature();
	SU_LevelState _measureLevel();
	float   _effectiveTemperature(float measured) const;

	int8_t  _indexOfChannel(SU_Channel channel) const;
};
