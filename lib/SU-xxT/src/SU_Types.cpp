/*
 * SU_Types.cpp — Tabelas de modelo da família SU-xxT
 */

#include "SU_Types.h"

// Catálogo canônico dos sensores. A ORDEM aqui é a ordem de enumeração
// apresentada por getReadingByIndex(), e segue a tabela §5 do
// hardware/SU-xxT/README.md — assim o IDX_MAP do nó SU-30T é a identidade.
//
// Nota de orçamento: estes literais vivem em RAM no AVR. Se o teto de 1400 B
// apertar, a primeira medida é movê-los para PROGMEM (~120 B), ao custo de
// getLabelByIndex() passar a copiar para um buffer.
static const SU_SensorInfo SU_CATALOG[] = {
	{ SU_CH_MOISTURE_10CM, "umidade_10cm",    "%"     },  // 0
	{ SU_CH_MOISTURE_30CM, "umidade_30cm",    "%"     },  // 1
	{ SU_CH_TEMP_SOIL,     "temp_solo",       "\xC2\xB0" "C" },  // 2 — °C em UTF-8
	{ SU_CH_EC_SOLUTION,   "ec_solucao",      "uS/cm" },  // 3
	{ SU_CH_PH_SOLUTION,   "ph_solucao",      "pH"    },  // 4
	{ SU_CH_LEVEL_CHAMBER, "nivel_lisimetro", "0/1"   },  // 5
};

// Quais entradas do catálogo cada modelo expõe, na ordem de enumeração.
static const uint8_t SU_MAP_10[]  = { 0, 1 };
static const uint8_t SU_MAP_10T[] = { 0, 1, 2 };
static const uint8_t SU_MAP_20[]  = { 0, 1, 3, 5 };
static const uint8_t SU_MAP_20T[] = { 0, 1, 2, 3, 5 };
static const uint8_t SU_MAP_30[]  = { 0, 1, 3, 4, 5 };
static const uint8_t SU_MAP_30T[] = { 0, 1, 2, 3, 4, 5 };

typedef struct {
	const uint8_t* map;
	uint8_t        count;
} SU_ModelMap;

static const SU_ModelMap SU_MODEL_MAPS[SU_MODEL_COUNT] = {
	{ SU_MAP_10,  sizeof(SU_MAP_10)  },
	{ SU_MAP_10T, sizeof(SU_MAP_10T) },
	{ SU_MAP_20,  sizeof(SU_MAP_20)  },
	{ SU_MAP_20T, sizeof(SU_MAP_20T) },
	{ SU_MAP_30,  sizeof(SU_MAP_30)  },
	{ SU_MAP_30T, sizeof(SU_MAP_30T) },
};

uint8_t SU_modelDeviceCount(SU_Model model)
{
	if (model >= SU_MODEL_COUNT) {
		return 0;
	}
	return SU_MODEL_MAPS[model].count;
}

const SU_SensorInfo* SU_modelSensorAt(SU_Model model, uint8_t index)
{
	if (model >= SU_MODEL_COUNT || index >= SU_MODEL_MAPS[model].count) {
		return NULL;
	}
	return &SU_CATALOG[SU_MODEL_MAPS[model].map[index]];
}

bool SU_modelHasSoilTemp(SU_Model model)
{
	return model == SU_MODEL_10T || model == SU_MODEL_20T || model == SU_MODEL_30T;
}

bool SU_modelHasChamber(SU_Model model)
{
	// SU-10 e SU-10T não têm lisímetro: sem câmara, sem EC, pH ou nível.
	return model >= SU_MODEL_20;
}
