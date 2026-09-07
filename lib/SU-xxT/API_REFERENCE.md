# 📖 Referência da API — SU-xxT

> **Status:** implementada e compilando em AVR, ESP8266 e ESP32 — nunca executada
> em hardware. Ver [ARCHITECTURE.md](ARCHITECTURE.md) para o porquê de cada
> decisão.

---

## 1. Structs e Enums Core

### `SU_Model`

`SU_MODEL_10`, `SU_MODEL_10T`, `SU_MODEL_20`, `SU_MODEL_20T`, `SU_MODEL_30`,
`SU_MODEL_30T`.

Determina quais canais estão populados e, portanto, o valor de
`getDeviceCount()`.

### `SU_Channel`

O valor do enum é o **canal físico do 74HC4051**, fixo pelo cobre da PCB — nunca o
índice de enumeração.

| Constante | Canal MUX | Grandeza |
|---|:---:|---|
| `SU_CH_MOISTURE_10CM` | 0 | Umidade do solo a 10 cm |
| `SU_CH_MOISTURE_30CM` | 1 | Umidade do solo a 30 cm |
| `SU_CH_LEVEL_CHAMBER` | 2 | Nível da câmara |
| `SU_CH_EC_SOLUTION` | 3 | Condutividade da solução |
| `SU_CH_PH_SOLUTION` | 4 | Tensão do eletrodo de pH |
| — | 5–7 | Livres (expansão) |
| `SU_CH_TEMP_SOIL` | — | DS18B20, 1-Wire (fora do MUX) |

### `SU_LevelState`

| Constante | Valor | Significado |
|---|:---:|---|
| `SU_LEVEL_FAULT` | −1 | Falha de leitura — **não publicar nada** |
| `SU_LEVEL_DRY` | 0 | Câmara seca — alerta de secagem legítimo |
| `SU_LEVEL_FILLED` | 1 | Câmara preenchida |

### `SU_LevelProbeType`

`SU_LEVEL_OPTICAL` (FS-IR02 / XKC-001A, saída digital, sem excitação) ou
`SU_LEVEL_CONDUCTIVE` (hastes de inox 316, excitação AC obrigatória).

### Sentinelas de erro

Todas **abaixo de −32767**, para serem descartadas por
`M360Node::_readAndSendAll()` em vez de publicadas no MQTT.

| Constante | Valor | Significado |
|---|---:|---|
| `SU_ERR_LEVEL_LOW` | `-32767.0f` | Câmara seca |
| `SU_ERR_STALE_RECHARGE` | `-32768.0f` | Reserva ainda não trocada |
| `SU_ERR_ADC_FAULT` | `-32769.0f` | Falha do ADS1115, saturação de fundo de escala, grandeza fisicamente impossível, ou índice inválido |
| `SU_DEVICE_DISCONNECTED` | `-32770.0f` | DS18B20 ausente, mudo, ou devolvendo o 85,0 °C de reset do scratchpad |
| `SU_ERR_NOT_SAMPLED` | `-32771.0f` | Cache frio |

```cpp
bool SU_isError(float v);   // v <= -32767.0f || isnan(v)
```

Use sempre o predicado — nunca compare float por igualdade.

### `SU_SensorInfo`

| Campo | Tipo | Conteúdo |
|---|---|---|
| `channel` | `SU_Channel` | Identificador do canal |
| `label` | `const char*` | `"umidade_10cm"`, `"ph_solucao"`, … |
| `unit` | `const char*` | `"%"`, `"°C"`, `"µS/cm"`, `"pH"`, `"0/1"` |

> **Não há `childId` aqui, por decisão de arquitetura.** O contrato MySensors vive
> no `NODE_ITEMS[]` do nó, com SSoT em `inventario.md`.

### `SU_ChannelAdcCfg`

Tabela `const` indexada pelo canal físico do MUX, com `gain` (PGA) e `dataRate`.
Não é calibração de campo, é característica de projeto.

| Canal | PGA | Motivo |
|---|---|---|
| 0, 1 umidade | `GAIN_ONE` (±4,096 V) | Menor FS que comporte 3,3 V |
| 2 nível | `GAIN_ONE` (±4,096 V) | Decisão por limiar; resolução não é crítica |
| 3 EC | `GAIN_ONE` (±4,096 V) inicial | Faixa real só se conhece caracterizando |
| 4 pH | `GAIN_TWO` (±2,048 V) | Dobra a resolução onde ela decide o resultado |

> Ambos os campos são **`uint16_t`**, não `uint8_t`: os valores de `adsGain_t` são
> campos de bits do registrador de configuração (`GAIN_TWO` = `0x0200` = 512).
> Truncar para 8 bits selecionaria o ganho errado em silêncio — o compilador pegou
> isso como `-Wnarrowing` na primeira compilação.
>
> A tabela vive em RAM, não em PROGMEM. Movê-la para PROGMEM é degrau da escada de
> contenção de memória, não o estado atual.

---

## 2. Classe `SU_Device`

### Construtor

```cpp
SU_Device(uint8_t pinMuxA, uint8_t pinMuxB, uint8_t pinMuxC,
          uint8_t pinAcExcite,
          int8_t  pinMosfetPwr,
          int8_t  pinOneWire,
          SU_Model model = SU_MODEL_30T,
          uint8_t  i2cAddress = 0x48);
```

| Parâmetro | Papel |
|---|---|
| `pinMuxA/B/C` | Três linhas de endereço do 74HC4051 |
| `pinAcExcite` | Trem de pulsos AC da EC e do nível condutivo. Alta impedância fora da amostragem |
| `pinMosfetPwr` | Chaveia **apenas** os front-ends analógicos |
| `pinOneWire` | Barramento do DS18B20 |

### Ciclo de Vida

| Método | Contrato |
|---|---|
| `bool begin(TwoWire &wirePort = Wire)` | Inicializa I2C, ADS1115 e 1-Wire; carrega calibração |
| `void powerUp()` | Liga o trilho dos front-ends |
| `void powerDown()` | Corta o trilho dos front-ends |
| `void requestReadings()` | Warm-up, reaplicação da config do ADS1115, leitura multicanal, compensações e cache |

`requestReadings()` é **bloqueante** e é o custo dominante da janela acordada. Deve
ser chamado de `M360::powerUp()` — ver o guia rápido no [README](README.md).

### Enumeração / Indexação

| Método | Retorno |
|---|---|
| `uint8_t getDeviceCount() const` | Número de sensores ativos no modelo instanciado |
| `float getReadingByIndex(uint8_t index)` | Medição processada. `SU_ERR_NOT_SAMPLED` se o cache está frio; `SU_ERR_ADC_FAULT` se `index >= getDeviceCount()` |
| `const char* getLabelByIndex(uint8_t index) const` | Rótulo |
| `const char* getUnitByIndex(uint8_t index) const` | Unidade |
| `SU_Channel getChannelByIndex(uint8_t index) const` | Índice de enumeração → canal físico |

### Acesso Direto por Grandeza

| Método | Unidade | Sentinelas possíveis |
|---|---|---|
| `float getReading(SU_Channel channel)` | conforme o canal | todas |
| `float getMoisture10()` / `getMoisture30()` | % | `ADC_FAULT`, `NOT_SAMPLED` |
| `float getSoilTemperature()` | °C | `DEVICE_DISCONNECTED`, `NOT_SAMPLED` |
| `float getEC25()` | µS/cm | `LEVEL_LOW`, `STALE_RECHARGE`, `ADC_FAULT` |
| `float getECRaw()` | µS/cm | idem |
| `float getPH()` | pH | idem |
| `SU_LevelState getLevel()` | — | retorna `SU_LEVEL_FAULT` |
| `bool isChamberValid() const` | — | — |
| `bool isRechargeSettled() const` | — | — |
| `uint16_t getCyclesSinceRecharge() const` | ciclos | — |
| `void resetRechargeState()` | — | Zera a contagem e invalida a acomodação |

> ⚠️ **O nó deve chamar `resetRechargeState()` ao alterar o intervalo de reporte
> (`V_VAR1`).** O tempo físico de troca da reserva é `minRechargeCycles × intervalo`;
> baixar o intervalo de 30 para 2 min encolhe a acomodação 15 vezes e liberaria
> solução estagnada como se fosse nova. A biblioteca não conhece MySensors e não
> tem como detectar a mudança sozinha.

### Calibração e Configuração

| Método | Contrato |
|---|---|
| `SU_CalibrationData& getCalibration()` | Acesso mutável à cópia em RAM |
| `void setCalibration(const SU_CalibrationData&)` | Substitui a cópia em RAM |
| `bool saveCalibration()` | Grava com magic, versão e CRC. Só escreve se houve mudança |
| `bool loadCalibration()` | Valida magic/versão/CRC; em falha carrega defaults e retorna `false` |
| `void resetCalibration()` | Defaults de fábrica em RAM, sem gravar |

---

## 3. `SU_CalibrationData`

| Grupo | Campos |
|---|---|
| Umidade | `airAdc10`, `waterAdc10`, `airAdc30`, `waterAdc30` |
| pH | `voltagePH4`, `voltagePH7`, `voltagePH10`, **`calibTempC`** |
| EC | `kCell`, `alphaTemp` (padrão `0.0191 / °C`), `ecOffsetVoltage` |
| Excitação AC | `acExciteHz` (1000), `acExcitePulses` (16), `acSettleUs` |
| Nível | `levelThresholdAdc`, `invertedLevel`, `levelProbeType` |
| Sem DS18B20 | `assumedTempC` (padrão `25.0`) |
| Persistência | `magic`, `layoutVersion`, `crc` |

> ⚠️ **`calibTempC` não é opcional.** Sem ela a compensação de temperatura do pH
> não é definível, e aplicar o fator de Nernst sobre um slope já empírico é dupla
> compensação.

### Constantes de EEPROM

```cpp
#define SU_EEPROM_CALIB_ADDRESS 768   // dentro de M360_EEPROM_APP_BASE
#define SU_EEPROM_CALIB_SIZE     64
static_assert(sizeof(SU_CalibrationData) <= SU_EEPROM_CALIB_SIZE, "...");
```

A região 768–1023 é declarada como área de aplicação no mapa unificado de
[`M360Config.h`](../M360-DRY/src/M360Config.h). As constantes acima vivem **aqui**,
não lá — a lib core não conhece libs de aplicação.

---

## 4. Dependências

| Biblioteca | Observação |
|---|---|
| `adafruit/Adafruit ADS1X15` | Arrasta `adafruit/Adafruit BusIO` por transitividade — declarar explicitamente |
| `paulstoffregen/OneWire` | — |
| `milesburton/DallasTemperature` | Considerar 9 bits: 94 ms contra 750 ms a 12 bits |

Se o orçamento de flash estourar, a primeira troca é substituir
`Adafruit ADS1X15` + `Adafruit BusIO` por um driver mínimo de registrador sobre
`Wire` — o ADS1115 tem quatro registradores, e essas libs trazem abstrações (SPI,
objetos de device) que o projeto não usa.
