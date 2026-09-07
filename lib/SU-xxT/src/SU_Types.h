/*
 * SU_Types.h — Tipos, canais e sentinelas da família SU-xxT
 *
 * Camada física: esta biblioteca NÃO conhece child IDs, MySensors nem Node-RED.
 * O contrato MySensors vive no NODE_ITEMS[] do nó, com SSoT em
 * src/DRY/horta/inventario.md.
 *
 * Ver ARCHITECTURE.md para o porquê de cada decisão registrada aqui.
 */

#pragma once
#include <Arduino.h>

// ===== MODELOS DA FAMÍLIA =====
//
// A diferenciação é por população seletiva de componentes na mesma PCB.
// O sufixo 'T' indica a presença do DS18B20.

typedef enum {
	SU_MODEL_10  = 0,  // Umidade 10/30 cm
	SU_MODEL_10T,      // + temperatura do solo
	SU_MODEL_20,       // + EC e nível (EC indicativa: sem termometria)
	SU_MODEL_20T,      // + temperatura → EC25 compensada
	SU_MODEL_30,       // + pH (EC ainda indicativa)
	SU_MODEL_30T,      // Topo de linha
	SU_MODEL_COUNT
} SU_Model;

// ===== CANAIS =====
//
// O valor do enum é o CANAL FÍSICO do 74HC4051, fixo pelo cobre da PCB.
// NÃO é o índice de enumeração — este varia com o modelo populado.
// Use getChannelByIndex() para traduzir entre os dois espaços.

typedef enum {
	SU_CH_MOISTURE_10CM = 0,
	SU_CH_MOISTURE_30CM = 1,
	SU_CH_LEVEL_CHAMBER = 2,
	SU_CH_EC_SOLUTION   = 3,
	SU_CH_PH_SOLUTION   = 4,
	// Canais MUX 5, 6 e 7 reservados para expansão (ORP, 2ª célula de EC)
	SU_CH_TEMP_SOIL     = 0xFF  // 1-Wire, fora do MUX
} SU_Channel;

#define SU_MUX_CHANNEL_COUNT 8

// ===== ESTADO DO NÍVEL =====
//
// Três estados, não dois. "Seco" é publicado como alerta de irrigação, então
// falha de instrumento NÃO pode ser indistinguível de câmara seca — um ADC mudo
// viraria recomendação de irrigar. Ver ARCHITECTURE.md §8.

typedef enum {
	SU_LEVEL_FAULT  = -1,  // Falha de leitura — não publicar nada
	SU_LEVEL_DRY    =  0,  // Câmara seca — alerta legítimo
	SU_LEVEL_FILLED =  1   // Câmara preenchida
} SU_LevelState;

// ===== TIPO DE SONDA DE NÍVEL =====

typedef enum {
	SU_LEVEL_OPTICAL    = 0,  // FS-IR02 / XKC-001A — saída digital, sem excitação
	SU_LEVEL_CONDUCTIVE = 1   // Hastes inox 316 — excitação AC OBRIGATÓRIA
} SU_LevelProbeType;

// ===== SENTINELAS DE ERRO =====
//
// TODAS abaixo de -32767. Este é o único piso que M360Node::_readAndSendAll()
// descarta (M360Node.cpp:249):
//
//     if (isnan(val) || val <= -32767.0f) { continue; }
//
// Uma sentinela "óbvia" como -999.0f ATRAVESSA esse filtro e é publicada no MQTT
// como leitura legítima — pH = -999.0 no dashboard. Pior: por alternar com o
// valor real, vence também o filtro de variação (> 0.05f) e transmite a cada
// ciclo. Ver ARCHITECTURE.md §1.

#define SU_ERR_LEVEL_LOW        (-32767.0f)  // Câmara seca
#define SU_ERR_STALE_RECHARGE   (-32768.0f)  // Reserva ainda não trocada
#define SU_ERR_ADC_FAULT        (-32769.0f)  // Falha do ADS1115 ou índice inválido
#define SU_DEVICE_DISCONNECTED  (-32770.0f)  // DS18B20 ausente ou mudo
#define SU_ERR_NOT_SAMPLED      (-32771.0f)  // Cache frio

// Piso de descarte da M360Node. Manter em sincronia com M360Node.cpp.
#define SU_ERR_FLOOR            (-32767.0f)

// Teste de erro. Use SEMPRE este predicado — nunca compare float por igualdade.
static inline bool SU_isError(float v)
{
	return isnan(v) || v <= SU_ERR_FLOOR;
}

// ===== METADADOS DO SENSOR =====
//
// Sem childId, por decisão de arquitetura: o contrato MySensors pertence ao
// NODE_ITEMS[] do nó. Ver ARCHITECTURE.md §10.

typedef struct {
	SU_Channel  channel;
	const char* label;   // "umidade_10cm", "ph_solucao", ...
	const char* unit;    // "%", "°C", "uS/cm", "pH", "0/1"
} SU_SensorInfo;

// ===== CONFIGURAÇÃO DO ADC POR CANAL =====
//
// O ganho é POR CANAL, não global: as faixas são incompatíveis entre si.
// Umidade e nível varrem 0–3,3 V e exigem FS ±4,096 V; o pH oscila numa janela
// estreita em torno de ~1,65 V, onde a resolução decide o resultado.
// Com ganho único, ou se satura a umidade, ou se joga fora a resolução do pH.
//
// Os valores de `gain` correspondem ao enum adsGain_t da Adafruit_ADS1X15,
// replicados aqui para não arrastar o header da Adafruit para dentro deste.

// uint16_t, não uint8_t: os valores de adsGain_t e das taxas são campos de bits
// do registrador de configuração do ADS1115 (GAIN_TWO = 0x0200 = 512), não
// índices pequenos. Truncar para 8 bits selecionaria o ganho errado em silêncio.
typedef struct {
	uint16_t gain;      // adsGain_t
	uint16_t dataRate;  // RATE_ADS1115_*
} SU_ChannelAdcCfg;

// ===== FAIXAS DE PLAUSIBILIDADE FÍSICA =====
//
// Grandeza fora destes limites não é medição ruim: é impossível. Serve de rede
// final contra saturação de PGA, cabo partido, eletrodo ressecado e calibração
// corrompida — todos produzem números finitos e silenciosos, que sem esta
// checagem seriam publicados como leitura válida.
//
// O caso que motivou: com o módulo de pH montado em offset de 2,5 V em vez de
// Vcc/2, TODA a escala satura o canal 4 em ±2,048 V e o cálculo devolve pH 0,27
// — travado, sem sinal de erro algum.

#define SU_PH_MIN               0.0f
#define SU_PH_MAX              14.0f

#define SU_EC_MIN               0.0f       // condutividade negativa não existe
#define SU_EC_MAX          100000.0f       // 100 mS/cm — muito acima de solução
                                           // de fertirrigação saturada (~20)

// Umidade: excursão MODERADA além da calibração é legítima (solo mais seco que
// o ponto de ar, ou mais úmido que o de água) e é apenas grampeada em 0–100 %.
// Excursão grosseira indica sonda desconectada ou calibração degenerada.
#define SU_MOIST_GROSS_MIN    (-20.0f)
#define SU_MOIST_GROSS_MAX     120.0f

// Valor de reset do scratchpad do DS18B20. Aparece quando o chip reinicia com
// CRC ainda válido — típico de cabo longo enterrado com ruído. Na rizosfera a
// 30 cm, 85 °C é termodinamicamente impossível, então descartá-lo é seguro
// NESTE domínio (em outro, seria descartar dado legítimo).
#define SU_DS18B20_RESET_VALUE 85.0f

// ===== HELPERS DE MODELO =====

// Número de sensores ativos no modelo — igual ao getDeviceCount() do device.
uint8_t SU_modelDeviceCount(SU_Model model);

// Índice de enumeração (0..count-1) → metadados do sensor.
// Retorna nullptr se o índice não existir no modelo.
const SU_SensorInfo* SU_modelSensorAt(SU_Model model, uint8_t index);

// O modelo tem DS18B20? (sufixo 'T')
bool SU_modelHasSoilTemp(SU_Model model);

// O modelo tem câmara de lisímetro? (EC, nível — e pH nos modelos 30)
bool SU_modelHasChamber(SU_Model model);
