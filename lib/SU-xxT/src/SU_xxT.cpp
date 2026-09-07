/*
 * SU_xxT.cpp — Implementação de SU_Device
 */

#include "SU_xxT.h"
#include <EEPROM.h>
#include <string.h>

// A EEPROM emulada do ESP não tem update() — só read/write/commit. No AVR,
// update() é o que evita gastar um ciclo de escrita quando o byte não mudou.
static inline void su_eepromUpdate(int addr, uint8_t value)
{
#if defined(ESP8266) || defined(ESP32)
	if (EEPROM.read(addr) != value) {
		EEPROM.write(addr, value);
	}
#else
	EEPROM.update(addr, value);
#endif
}

// Warm-up dos front-ends analógicos após ligar o trilho chaveado.
// A DEFINIR POR MEDIÇÃO — o estágio de pH, de alta impedância, é o mais lento.
#define SU_WARMUP_MS            50

// Acomodação após trocar o canal do MUX, antes de converter.
#define SU_MUX_SETTLE_US        200

// Resolução do DS18B20. 9 bits = 94 ms de conversão contra 750 ms a 12 bits,
// e 0,5 °C é suficiente para compensar EC e pH. Ver README, orçamento de tempo.
#define SU_DS18B20_RESOLUTION   9

// Ganhos do ADS1115 (adsGain_t da Adafruit). Ver ARCHITECTURE.md §5.
#define SU_GAIN_WIDE            GAIN_ONE       // ±4,096 V
#define SU_GAIN_PH              GAIN_TWO       // ±2,048 V

// Tabela de configuração do ADC por canal físico do MUX.
// const: não é calibração de campo, é característica de projeto.
static const SU_ChannelAdcCfg SU_ADC_CFG[SU_MUX_CHANNEL_COUNT] = {
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 0 umidade 10 cm
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 1 umidade 30 cm
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 2 nível
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 3 EC (envelope retificado)
	{ SU_GAIN_PH,   RATE_ADS1115_128SPS },  // 4 pH — FS estreito dobra a resolução
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 5 livre
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 6 livre
	{ SU_GAIN_WIDE, RATE_ADS1115_128SPS },  // 7 livre
};

// Entrada single-ended do ADS1115 que recebe a saída comum do 4051.
#define SU_ADS_INPUT            0

// ===== CONSTRUCTOR =====

SU_Device::SU_Device(uint8_t pinMuxA, uint8_t pinMuxB, uint8_t pinMuxC,
                     uint8_t pinAcExcite,
                     int8_t  pinMosfetPwr,
                     int8_t  pinOneWire,
                     SU_Model model,
                     uint8_t  i2cAddress)
	: _pinMuxA(pinMuxA), _pinMuxB(pinMuxB), _pinMuxC(pinMuxC),
	  _pinAcExcite(pinAcExcite),
	  _pinMosfetPwr(pinMosfetPwr),
	  _pinOneWire(pinOneWire),
	  _model(model),
	  _i2cAddress(i2cAddress),
	  _count(SU_modelDeviceCount(model)),
	  _adsOk(false),
	  _ecRaw(SU_ERR_NOT_SAMPLED),
	  _level(SU_LEVEL_FAULT),
	  _cyclesSinceRecharge(0),
	  _sampled(false),
	  _savedCrc(0)
{
	for (uint8_t i = 0; i < SU_MAX_SENSORS; i++) {
		_cache[i] = SU_ERR_NOT_SAMPLED;
	}
	SU_calibrationDefaults(_cal);
}

// ===== CICLO DE VIDA =====

bool SU_Device::begin(TwoWire& wirePort)
{
	pinMode(_pinMuxA, OUTPUT);
	pinMode(_pinMuxB, OUTPUT);
	pinMode(_pinMuxC, OUTPUT);
	_selectChannel(0);

	// Alta impedância: sem polarização contínua sobre os eletrodos.
	pinMode(_pinAcExcite, INPUT);

	if (_pinMosfetPwr >= 0) {
		pinMode(_pinMosfetPwr, OUTPUT);
		powerDown();
	}

	// O ADS1115 fica no trilho PERMANENTE (ver ARCHITECTURE.md §4), então
	// begin() aqui basta — mas requestReadings() reaplica a configuração antes
	// de cada conversão, para que a leitura não dependa do histórico de
	// alimentação.
	_adsOk = _ads.begin(_i2cAddress, &wirePort);

	if (SU_modelHasSoilTemp(_model) && _pinOneWire >= 0) {
		_oneWire.begin((uint8_t)_pinOneWire);
		_dallas.setOneWire(&_oneWire);
		_dallas.begin();
		_dallas.setResolution(SU_DS18B20_RESOLUTION);
		// Bloqueante por conversão: o ciclo já é serializado por requestReadings().
		_dallas.setWaitForConversion(true);
	}

	const bool calOk = loadCalibration();
	return _adsOk && calOk;
}

void SU_Device::powerUp()
{
	// MOSFET canal P em chave high-side: gate em LOW conduz.
	if (_pinMosfetPwr >= 0) {
		digitalWrite(_pinMosfetPwr, LOW);
	}
}

void SU_Device::powerDown()
{
	if (_pinMosfetPwr >= 0) {
		digitalWrite(_pinMosfetPwr, HIGH);
	}
	// Garante que a excitação não fica polarizando os eletrodos com o
	// front-end desligado.
	pinMode(_pinAcExcite, INPUT);
}

// ===== AQUISIÇÃO =====

void SU_Device::requestReadings()
{
	delay(SU_WARMUP_MS);

	// --- 1. Nível primeiro: ele governa a validade de pH e EC ---
	_level = _measureLevel();

	// --- 2. Contador de recarga ---
	//
	// Contagem em CICLOS. millis() não avança durante smartSleep() no AVR.
	if (_level == SU_LEVEL_FILLED) {
		if (_cyclesSinceRecharge < 0xFFFF) {
			_cyclesSinceRecharge++;
		}
		if (!_cal.rechargeSettled && _cyclesSinceRecharge >= _cal.minRechargeCycles) {
			_cal.rechargeSettled = true;
			saveCalibration();   // grava só na TRANSIÇÃO, não a cada ciclo
		}
	} else if (_level == SU_LEVEL_DRY) {
		_cyclesSinceRecharge = 0;
		if (_cal.rechargeSettled) {
			_cal.rechargeSettled = false;
			saveCalibration();
		}
	}
	// Em SU_LEVEL_FAULT o contador não é tocado: falha de instrumento não é
	// evidência de que a câmara esvaziou.

	// --- 3. Temperatura ---
	const float tempMeasured = _measureTemperature();
	const float tempEff      = _effectiveTemperature(tempMeasured);

	// --- 4. Umidade ---
	bool ok10 = false, ok30 = false;
	const int16_t raw10 = _readRaw(SU_CH_MOISTURE_10CM, &ok10);
	const int16_t raw30 = _readRaw(SU_CH_MOISTURE_30CM, &ok30);

	const float moist10 = ok10 ? _computeMoisture(raw10, _cal.airAdc10, _cal.waterAdc10)
	                           : SU_ERR_ADC_FAULT;
	const float moist30 = ok30 ? _computeMoisture(raw30, _cal.airAdc30, _cal.waterAdc30)
	                           : SU_ERR_ADC_FAULT;

	// --- 5. EC e pH: só valem com câmara cheia E reserva trocada ---
	float ec25 = SU_ERR_LEVEL_LOW;
	float ph   = SU_ERR_LEVEL_LOW;
	_ecRaw     = SU_ERR_LEVEL_LOW;

	if (_level == SU_LEVEL_FAULT) {
		ec25 = ph = _ecRaw = SU_ERR_ADC_FAULT;
	} else if (_level == SU_LEVEL_FILLED) {
		if (!_cal.rechargeSettled) {
			ec25 = ph = _ecRaw = SU_ERR_STALE_RECHARGE;
		} else if (SU_modelHasChamber(_model)) {
			bool okEc = false, okPh = false;

			_exciteAC();
			const int16_t rawEc = _readRaw(SU_CH_EC_SOLUTION, &okEc);
			if (okEc) {
				const float volts = _ads.computeVolts(rawEc) - _cal.ecOffsetVoltage;
				_ecRaw = volts * _cal.kCell * 1000.0f;   // uS/cm
				ec25   = _computeEC25(_ecRaw, tempEff);
			} else {
				_ecRaw = ec25 = SU_ERR_ADC_FAULT;
			}

			const int16_t rawPh = _readRaw(SU_CH_PH_SOLUTION, &okPh);
			ph = okPh ? _computePH(_ads.computeVolts(rawPh), tempEff)
			          : SU_ERR_ADC_FAULT;
		}
	}

	// --- 6. Publica no cache, na ordem de enumeração do modelo ---
	for (uint8_t i = 0; i < _count; i++) {
		const SU_SensorInfo* info = SU_modelSensorAt(_model, i);
		if (info == NULL) {
			_cache[i] = SU_ERR_ADC_FAULT;
			continue;
		}
		switch (info->channel) {
			case SU_CH_MOISTURE_10CM: _cache[i] = moist10;      break;
			case SU_CH_MOISTURE_30CM: _cache[i] = moist30;      break;
			case SU_CH_TEMP_SOIL:     _cache[i] = tempMeasured; break;
			case SU_CH_EC_SOLUTION:   _cache[i] = ec25;         break;
			case SU_CH_PH_SOLUTION:   _cache[i] = ph;           break;
			case SU_CH_LEVEL_CHAMBER:
				// Em falha devolve sentinela: o motor descarta e o nó CALA, em
				// vez de afirmar solo seco a partir de um instrumento mudo.
				_cache[i] = (_level == SU_LEVEL_FAULT) ? SU_ERR_ADC_FAULT
				                                       : (float)_level;
				break;
			default:                  _cache[i] = SU_ERR_ADC_FAULT; break;
		}
	}

	_sampled = true;
}

// ===== ACESSO POR ÍNDICE =====

float SU_Device::getReadingByIndex(uint8_t index)
{
	if (index >= _count) {
		return SU_ERR_ADC_FAULT;
	}
	if (!_sampled) {
		return SU_ERR_NOT_SAMPLED;
	}
	return _cache[index];
}

const char* SU_Device::getLabelByIndex(uint8_t index) const
{
	const SU_SensorInfo* info = SU_modelSensorAt(_model, index);
	return info ? info->label : "";
}

const char* SU_Device::getUnitByIndex(uint8_t index) const
{
	const SU_SensorInfo* info = SU_modelSensorAt(_model, index);
	return info ? info->unit : "";
}

SU_Channel SU_Device::getChannelByIndex(uint8_t index) const
{
	const SU_SensorInfo* info = SU_modelSensorAt(_model, index);
	return info ? info->channel : SU_CH_TEMP_SOIL;
}

// ===== ACESSO POR GRANDEZA =====

int8_t SU_Device::_indexOfChannel(SU_Channel channel) const
{
	for (uint8_t i = 0; i < _count; i++) {
		const SU_SensorInfo* info = SU_modelSensorAt(_model, i);
		if (info && info->channel == channel) {
			return (int8_t)i;
		}
	}
	return -1;
}

float SU_Device::getReading(SU_Channel channel)
{
	const int8_t idx = _indexOfChannel(channel);
	if (idx < 0) {
		return SU_ERR_ADC_FAULT;   // canal não populado neste modelo
	}
	return getReadingByIndex((uint8_t)idx);
}

// ===== CALIBRAÇÃO =====

void SU_Device::setCalibration(const SU_CalibrationData& cal)
{
	_cal = cal;
	_cal.magic         = SU_CALIB_MAGIC;
	_cal.layoutVersion = SU_CALIB_LAYOUT_VERSION;
	_cal.crc           = SU_calibrationCRC(_cal);
}

void SU_Device::resetCalibration()
{
	SU_calibrationDefaults(_cal);
}

bool SU_Device::loadCalibration()
{
	SU_CalibrationData disk;

	// Zerar antes de preencher byte a byte: se a EEPROM devolver menos bytes do
	// que a struct tem, nenhum campo fica com lixo de pilha — e o padding entra
	// determinístico no CRC.
	// cppcheck-suppress memsetClassFloat
	memset(&disk, 0, sizeof(disk));

	uint8_t* dst = reinterpret_cast<uint8_t*>(&disk);

#if defined(ESP8266) || defined(ESP32)
	// A EEPROM é emulada e o MySensors a abre com EEPROM.begin(512): sem
	// reabrir, uma leitura em 768 não alcança o setor. Ver ARCHITECTURE.md.
	EEPROM.begin(SU_EEPROM_CALIB_ADDRESS + SU_EEPROM_CALIB_SIZE);
#endif

	for (size_t i = 0; i < sizeof(disk); i++) {
		dst[i] = EEPROM.read(SU_EEPROM_CALIB_ADDRESS + i);
	}

	const bool valid = (disk.magic == SU_CALIB_MAGIC)
	                && (disk.layoutVersion == SU_CALIB_LAYOUT_VERSION)
	                && (disk.crc == SU_calibrationCRC(disk));

	if (!valid) {
		// Nunca operar com calibração de procedência duvidosa em silêncio.
		// Bytes de um layout antigo lidos como válidos produziriam pH e EC
		// plausíveis e ERRADOS — o pior modo de falha possível aqui.
		SU_calibrationDefaults(_cal);
		_savedCrc = 0;
		return false;
	}

	_cal      = disk;
	_savedCrc = disk.crc;
	return true;
}

bool SU_Device::saveCalibration()
{
	_cal.magic         = SU_CALIB_MAGIC;
	_cal.layoutVersion = SU_CALIB_LAYOUT_VERSION;
	_cal.crc           = SU_calibrationCRC(_cal);

	if (_cal.crc == _savedCrc) {
		return true;   // nada mudou — poupa os ciclos da EEPROM
	}

	const uint8_t* src = reinterpret_cast<const uint8_t*>(&_cal);

#if defined(ESP8266) || defined(ESP32)
	EEPROM.begin(SU_EEPROM_CALIB_ADDRESS + SU_EEPROM_CALIB_SIZE);
#endif

	for (size_t i = 0; i < sizeof(_cal); i++) {
		su_eepromUpdate(SU_EEPROM_CALIB_ADDRESS + i, src[i]);
	}

#if defined(ESP8266) || defined(ESP32)
	if (!EEPROM.commit()) {
		return false;
	}
#endif

	_savedCrc = _cal.crc;
	return true;
}

// ===== INTERNOS — MUX E ADC =====

void SU_Device::_selectChannel(uint8_t muxChannel)
{
	digitalWrite(_pinMuxA, (muxChannel & 0x01) ? HIGH : LOW);
	digitalWrite(_pinMuxB, (muxChannel & 0x02) ? HIGH : LOW);
	digitalWrite(_pinMuxC, (muxChannel & 0x04) ? HIGH : LOW);
}

void SU_Device::_applyAdcConfig(uint8_t muxChannel)
{
	if (muxChannel >= SU_MUX_CHANNEL_COUNT) {
		return;
	}
	_ads.setGain((adsGain_t)SU_ADC_CFG[muxChannel].gain);
	_ads.setDataRate(SU_ADC_CFG[muxChannel].dataRate);
}

int16_t SU_Device::_readRaw(uint8_t muxChannel, bool* ok)
{
	if (!_adsOk || muxChannel >= SU_MUX_CHANNEL_COUNT) {
		if (ok) *ok = false;
		return 0;
	}

	_selectChannel(muxChannel);
	_applyAdcConfig(muxChannel);
	delayMicroseconds(SU_MUX_SETTLE_US);

	const int16_t raw = _ads.readADC_SingleEnded(SU_ADS_INPUT);
	if (ok) *ok = true;
	return raw;
}

void SU_Device::_exciteAC()
{
	// Trem de pulsos alternado. Fora desta janela o pino volta a alta
	// impedância: DC contínuo em inox 316 dentro do lisímetro eletrolisa o
	// eletrodo e contamina a solução que a câmara existe para medir.
	const uint16_t halfPeriodUs = (uint16_t)(500000UL / (_cal.acExciteHz ? _cal.acExciteHz : 1000));

	pinMode(_pinAcExcite, OUTPUT);
	for (uint8_t i = 0; i < _cal.acExcitePulses; i++) {
		digitalWrite(_pinAcExcite, HIGH);
		delayMicroseconds(halfPeriodUs);
		digitalWrite(_pinAcExcite, LOW);
		delayMicroseconds(halfPeriodUs);
	}
	pinMode(_pinAcExcite, INPUT);

	// Acomodação do envelope retificado antes da conversão. É este sinal
	// quase-DC que o ADS1115 lê — o teto de 128 SPS não limita nada aqui.
	delayMicroseconds(_cal.acSettleUs);
}

// ===== INTERNOS — GRANDEZAS =====

float SU_Device::_measureTemperature()
{
	if (!SU_modelHasSoilTemp(_model) || _pinOneWire < 0) {
		return SU_DEVICE_DISCONNECTED;
	}

	_dallas.requestTemperatures();
	const float t = _dallas.getTempCByIndex(0);

	// DEVICE_DISCONNECTED_C (-127) é traduzido na fronteira da biblioteca:
	// sozinho ele passaria pelo filtro do motor e seria publicado.
	if (t <= DEVICE_DISCONNECTED_C) {
		return SU_DEVICE_DISCONNECTED;
	}
	return t;
}

float SU_Device::_effectiveTemperature(float measured) const
{
	// Modelos sem DS18B20 caem na temperatura assumida. O custo é assimétrico:
	// ~0,10 pH contra ~19 % na EC a 10 °C de desvio. Ver README.md.
	return SU_isError(measured) ? _cal.assumedTempC : measured;
}

SU_LevelState SU_Device::_measureLevel()
{
	if (!SU_modelHasChamber(_model)) {
		return SU_LEVEL_DRY;   // modelo sem câmara: não há o que medir
	}

	if (_cal.levelProbeType == SU_LEVEL_CONDUCTIVE) {
		_exciteAC();
	}

	bool ok = false;
	const int16_t raw = _readRaw(SU_CH_LEVEL_CHAMBER, &ok);
	if (!ok) {
		return SU_LEVEL_FAULT;
	}

	const bool above = (raw >= _cal.levelThresholdAdc);
	const bool full  = _cal.invertedLevel ? !above : above;

	return full ? SU_LEVEL_FILLED : SU_LEVEL_DRY;
}

float SU_Device::_computeMoisture(int16_t raw, int16_t airAdc, int16_t waterAdc)
{
	// Ar = seco = 0 %, água = saturado = 100 %.
	const int32_t span = (int32_t)airAdc - (int32_t)waterAdc;
	if (span == 0) {
		return SU_ERR_ADC_FAULT;   // calibração degenerada
	}

	float pct = 100.0f * (float)((int32_t)airAdc - (int32_t)raw) / (float)span;

	if (pct < 0.0f)   pct = 0.0f;
	if (pct > 100.0f) pct = 100.0f;
	return pct;
}

float SU_Device::_computeEC25(float ecRaw, float tempC)
{
	const float denom = 1.0f + _cal.alphaTemp * (tempC - 25.0f);
	if (fabsf(denom) < 0.001f) {
		return SU_ERR_ADC_FAULT;
	}
	return ecRaw / denom;
}

float SU_Device::_nernstSlope(float tempC) const
{
	// S_nernst(T) = 0,05916 · (T + 273,15) / 298,15   [V/pH]
	return 0.05916f * (tempC + 273.15f) / 298.15f;
}

float SU_Device::_computePH(float voltage, float tempC)
{
	// Três passos. O fator de Nernst NÃO multiplica a leitura diretamente:
	// o slope medido em campo já embute a temperatura da calibração, e
	// multiplicá-lo pelo fator cru seria DUPLA COMPENSAÇÃO.
	// Ver ARCHITECTURE.md §7.

	// 1. Slope empírico do segmento, em V/pH.
	const bool  acid    = (voltage > _cal.voltagePH7);
	const float sSeg    = acid ? (_cal.voltagePH4  - _cal.voltagePH7) / 3.0f
	                           : (_cal.voltagePH7  - _cal.voltagePH10) / 3.0f;

	// 2. Eficiência do eletrodo — adimensional, e é a grandeza que NÃO depende
	//    da temperatura.
	const float sNernstCal = _nernstSlope(_cal.calibTempC);
	if (fabsf(sNernstCal) < 1e-6f) {
		return SU_ERR_ADC_FAULT;
	}
	const float eff = sSeg / sNernstCal;

	// 3. Leitura, com a temperatura do MOMENTO DA MEDIÇÃO.
	const float slope = eff * _nernstSlope(tempC);
	if (fabsf(slope) < 1e-6f) {
		return SU_ERR_ADC_FAULT;   // eletrodo morto ou calibração degenerada
	}

	// Premissa: voltagePH7 é o ponto isopotencial e independe da temperatura.
	return 7.0f + (_cal.voltagePH7 - voltage) / slope;
}
