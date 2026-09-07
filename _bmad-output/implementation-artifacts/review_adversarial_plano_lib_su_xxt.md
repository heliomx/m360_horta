# Revisão Adversarial — Plano de Implementação `lib/SU-xxT`

**Data:** 2026-09-07
**Skill:** `bmad-review-adversarial-general`
**Artefato revisado:** `~/.gemini/antigravity-ide/brain/b6874523-0bf7-4170-aa8e-19d4f2dda547/implementation_plan.md`
**Branch:** `feat/timeout-dinamico-por-intervalo`

## Fontes de confronto

O plano não foi julgado isoladamente. Cada achado foi verificado contra:

| Fonte | Papel na revisão |
|---|---|
| `hardware/SU-xxT/README.md` | Especificação de hardware, geometria da câmara e fluxo de amostragem |
| `lib/M360-DRY/src/M360Node.h` / `.cpp` | Motor real que consumirá a biblioteca (ciclo de vida, filtros, callbacks) |
| `lib/M360-DRY/src/M360Config.h` | Mapa unificado de EEPROM |
| `lib/M360-DRY/src/M360Power.h` | Ganchos `powerUp()` / `powerDown()` |
| `src/DRY/horta/inventario.md` | Contrato de child IDs e tipos de payload |
| `CLAUDE.md` | Regras de paridade obrigatória (inventário, Node-RED, artefatos BMAD) |
| `.pio/libdeps/.../MySensors/core/MyMessage.h` | Tipos nativos `S_*` / `V_*` do MySensors |

**Estado atual:** `lib/SU-xxT/` existe e está **vazio** — nada do plano foi
implementado. `hardware/SU-xxT/README.md` está untracked.

---

## Achados

### 1. O contrato de sentinelas é falso contra o motor que vai consumi-lo

O plano afirma, na seção *User Review Required*, que `SU_ERR_LEVEL_LOW` (`-999.0f`)
"permite que o firmware do nó descarte o envio de dados corrompidos para o
MySensors / MQTT de forma padronizada".

`M360Node::_readAndSendAll()` (`lib/M360-DRY/src/M360Node.cpp:249`) descarta
exatamente dois casos:

```cpp
float val = _readCb(i);
if (isnan(val) || val <= -32767.0f) {
    continue;
}
```

Nenhuma das quatro sentinelas propostas (`-999`, `-998`, `-997`, `-127`) é
filtrada. Todas serão publicadas como leituras legítimas — o dashboard receberá
`pH = -999.0`. Pior: `_readAndSendAll` compara com `fabsf(val - last) > 0.05f`,
então a alternância entre sentinela e leitura real gera tráfego a cada ciclo.

**Correção:** as sentinelas precisam ser `NAN` ou valores abaixo de `-32767` — ou
o plano está prometendo um comportamento que o ecossistema não implementa.

### 2. `minRechargeTimeMs` não pode funcionar em nó `LOW_POWER`

O campo é declarado em milissegundos e sua função (§3.3 do hardware doc) é medir
tempo contínuo com `Nivel = 1` antes de revalidar pH e EC.

Em AVR, `millis()` **não avança durante `sleep()` / `smartSleep()`**. O nó dorme
15 a 30 min (`M360Node.cpp:137`) e acorda por algumas centenas de milissegundos.
Um limiar de 30 min em `millis()` exigiria dezenas de dias de tempo-acordado
acumulado.

**Efeito prático:** `isRechargeSettled()` nunca vira `true`; pH e EC **nunca
voltam a ser publicados** depois do primeiro esvaziamento da câmara.

**Correção:** contar em ciclos de acordar × intervalo, não em milissegundos.

### 3. O construtor torna o hardware impossível

```cpp
SU_Device(uint8_t pinMuxA, uint8_t pinMuxB, int8_t pinMosfetPwr, ...)
```

Duas linhas de endereço = **4 canais no máximo**. `SU_Types.h` declara **5 canais
MUX**: `MOISTURE_10CM`, `MOISTURE_30CM`, `LEVEL_CHAMBER`, `EC_SOLUTION`,
`PH_SOLUTION` — o mesmo que o §4.1 do hardware doc. Não fecha.

Adicionalmente, `74HC4052` (duplo 4:1, endereçado por A/B binário) e `74HC4053`
(triplo SPDT, três pinos de seleção independentes) são citados como
intercambiáveis. Têm modelos de endereçamento incompatíveis, e **nenhum dos dois**
é acionado pela assinatura proposta.

### 4. "Índice / Canal MUX N" funde dois espaços de índice que divergem

O plano escreve literalmente `SU_CH_LEVEL_CHAMBER (Índice / Canal MUX 2)`.

- O **canal MUX** é fixo pelo cobre da PCB.
- O **índice de enumeração** varia com `getDeviceCount()`, que o próprio plano diz
  depender do modelo instanciado.

Num SU-20T (sem pH) os dois deixam de coincidir. Some-se que a tabela §5 do
hardware doc usa uma **terceira** ordem — `childId 2 = temp_solo`, enquanto o
canal MUX 2 é o nível. Três numerações concorrentes, nenhuma reconciliada.

### 5. `childId 0` viola a faixa do projeto

`inventario.md` §2 define `childId` como identificador na faixa **1–252**.
A tabela adotada pelo plano atribui `0` a `umidade_10cm`.

### 6. A biblioteca declara os children 253/254/255, que a `M360Node` cria sozinha

`inventario.md` §3 é explícito:

> Presentes em **todos** os nós que usam `M360::M360Node`, criados automaticamente
> por `begin()`. Nunca declarar em `NODE_ITEMS[]`.

Carregar `debug_remoto`, `report_interval` e `bateria_v` dentro de `SU_SensorInfo`
induz exatamente essa duplicação em quem usar a enumeração por índice para montar
o array de itens.

### 7. `childId` dentro de um driver físico inverte a arquitetura do projeto

O `CLAUDE.md` separa `noX.cpp` (puramente declarativo, dono do `NODE_ITEMS[]`) de
`sensorDrivers` (camada física). Colocar `childId` em `SU_Types.h` cria uma
segunda fonte de verdade para um contrato cuja SSoT é o `inventario.md`.

Quando as duas divergirem, a divergência é **silenciosa** — sem erro de compilação
e sem log. É o modo de falha que o `CLAUDE.md` descreve: o nó ignora o comando e o
sintoma é timeout no Sincronizador ACK.

### 8. `requestReadings()` não tem ponto de chamada no ciclo de vida da `M360Node`

O motor expõe `onRead(float (*)(uint8_t nodeIndex))` — ponteiro de função simples,
chamado **um item por vez**. Os únicos ganchos pré/pós são `M360::powerUp()` e
`M360::powerDown()`.

Pior: no perfil `M360_ALWAYS_ON` esses ganchos **nunca são chamados**
(`M360Node.cpp:110-118` — só `LOW_POWER` e `PASSIVE` os invocam). Um nó SU em
`ALWAYS_ON` leria um cache nunca atualizado, indefinidamente.

O plano não nomeia o perfil de energia nem o ponto de integração.

### 9. Nada define o cache frio

Não está especificado o que `getReadingByIndex()` devolve antes do primeiro
`requestReadings()`. Sem um valor explícito, o modo de falha padrão é dado
obsoleto com aparência de válido — e as sentinelas reservadas já foram gastas em
outros significados.

### 10. `getLevel()` retorna `bool`, sem canal de erro — e é ele que gatilha alerta agronômico

Todo o resto da API tem `SU_ERR_ADC_FAULT`; o nível não tem.

Uma falha do ADS1115 se apresenta como `false` = "câmara seca". Pelo §3.3 Função 2
do hardware doc, isso **é publicado como alerta de solo seco**. Falha de
instrumento vira recomendação de irrigar.

### 11. O ADS1115 fica no trilho chaveado e nunca é reconfigurado

`begin()` está no ciclo de vida, `powerUp()` está separado, e `requestReadings()`
só menciona "warm-up". Um ADS1115 que perdeu alimentação volta com o registrador
de configuração no default.

Some-se o problema clássico de pull-ups I2C vivos contra um escravo
despotencializado.

### 12. Ganho (PGA) e taxa de amostragem do ADS1115 não aparecem na API

Os canais têm faixas incompatíveis:

- Umidade 0–3,3 V exige fundo de escala ±4,096 V.
- pH oscila em torno de um offset de ~1,65 V, e é onde a resolução importa.

Sem troca de ganho por canal — que a API proposta não expõe — ou se satura a
umidade, ou se joga fora a resolução do pH.

### 13. `SU_CalibrationData` é "persistível em EEPROM" sem endereço reservado

Mapa unificado (`lib/M360-DRY/src/M360Config.h:11-23`):

```
0   – 511  : MySensors (reservado)
512 – 515  : M360NodeConfig — magic + interval
516 – 520  : reservado para expansão futura
521 +      : M360DeviceConfig — WiFi / MQTT / UF / CAR + CRC
```

A struct proposta tem treze floats (52+ bytes). Não cabe nos 5 bytes livres, e
qualquer coisa a partir de 521 invade região mapeada. Não há CRC nem versionamento
de layout — ao contrário do padrão que o `CLAUDE.md` exige para o gateway
(`saveConfig()` campo-a-campo com CRC).

### 14. A calibração de pH não registra a temperatura em que foi feita

`voltagePH4/7/10` definem um slope empírico. O §6 do hardware doc manda multiplicar
pelo fator de Nernst `0,05916 · ((T + 273,15) / 298,15)`.

Aplicar o fator teórico sobre um slope já medido em campo é **dupla compensação**,
a menos que se conheça a temperatura de calibração — que a struct não guarda.

O plano também não diz como os três pontos se combinam: dois segmentos lineares?
regressão? qual é o ponto isopotencial adotado?

### 15. Nível condutivo por limiar DC contamina a química que a câmara existe para medir

`levelThresholdAdc` + `invertedLevel` descrevem uma comparação DC. A Opção B do
hardware doc são pinos de inox 316 com "amostragem AC ultrarrápida (10 ms)" —
precisamente para evitar eletrólise.

Excitação DC em inox dentro de um lisímetro cuja premissa é "parede filtrante
inerte" (§3.1) corrói o eletrodo e enviesa pH e EC. E a amostragem de 10 ms não
sobrevive ao caminho 74HC4052 → ADS1115 a 128 SPS (~8 ms por conversão, mais
acomodação do MUX).

### 16. `getEC25()` é vendida como grandeza válida sobre um volume que a documentação declara enviesado

O §3.1.1 do hardware doc afirma que o acúmulo de KCl da junção de referência
"enviesa a EC para cima, de forma progressiva", e que hastes e bulbo leem águas
diferentes durante a troca.

A biblioteca oferece compensação térmica e **nenhum diagnóstico de deriva**. Além
disso, o `T_solo` usado na compensação vem de um DS18B20 enterrado **fora da
câmara**, com um atraso térmico que ninguém quantificou.

### 17. EC abandona o tipo nativo do MySensors sem justificativa

Existe `V_EC = 53` sob `S_WATER_QUALITY` (`MyMessage.h:182`), e o próprio projeto
já usa `S_WATER_QUALITY` / `V_PH` para o pH.

Escolher `S_CUSTOM` + `V_VAR2` para a EC descarta unidade e semântica em qualquer
controlador, e cria uma exceção que todo consumidor a jusante terá que conhecer de
cor.

### 18. O plano de verificação é só compilação — e nem isso está montado

O item 1 manda compilar os exemplos em `nanoatmega328` e ESP8266/ESP32. Mas:

- Os `.ino` ficam em `lib/SU-xxT/examples/`, que o PlatformIO **não** compila.
- O `platformio.ini` raiz usa `src_dir = .` com `build_src_filter` partindo de
  `src/DRY/...`. Não existe env que construa esses arquivos.
- Compilar um exemplo isolado não prova nada sobre o alvo real: o binário com
  MySensors + RF24 + `M360Node` juntos.

Pelo padrão do `CLAUDE.md`, a integração de referência pertence a
`src/DRY/horta/nos/<no>/` como `noX.cpp` + `sensorDrivers.h/cpp`, não a um exemplo
de biblioteca.

Nenhum teste cobre a lógica de maior risco: máquina de estados de acomodação da
recarga, histerese de nível, e as compensações EC25 / Nernst.

### 19. O orçamento de RAM/flash é caixa de seleção pós-fato, não restrição de projeto

"Verificar alocação estática adequada para 2 KB" não é número nenhum.

`Adafruit ADS1X15` arrasta `Adafruit BusIO` — dependência transitiva que o
`library.json` proposto **não lista** — somada a OneWire, DallasTemperature,
MySensors, RF24 e matemática de ponto flutuante, num ATmega328P.

Sem orçamento declarado e sem plano B (ADC direto? modelo reduzido? migrar para
ESP?), a descoberta do estouro vem no fim. O histórico do projeto já registra
exaustão de RAM como hipótese perseguida em campo.

### 20. `requestReadings()` bloqueante contradiz a premissa de baixo consumo

A conversão de 12 bits do DS18B20 são 750 ms, com o trilho de sensores ligado e o
front-end analógico consumindo. Somem-se warm-up, acomodação do MUX e cinco
conversões do ADS1115.

O plano trata "low-power" como adjetivo, não como número: não há tempo-acordado
alvo, nem menção a reduzir a resolução do DS18B20 (9 bits = 94 ms).

### 21. A entrega ignora as obrigações de mesma-entrega do `CLAUDE.md`

O plano introduz child IDs, tipos `V_*` inéditos na rede e um nó de referência, e
declara "respeito ao contrato do `inventario.md`" — sem alterar:

- `src/DRY/horta/inventario.md`
- `src/DRY/horta/nodered/flows.json`
- `src/DRY/horta/nodered/funcionalidades_nodered.md`
- `_bmad-output/planning-artifacts/` (PRD §3 Atores, épicos)

Pelas regras do repositório, um novo nó com novos children é entrega **incompleta**
enquanto o inventário e os consumidores Node-RED não forem atualizados no mesmo
commit.

### 22. `SU_ERR_STALE_RECHARGE` depende de estado só em RAM

Brownout de bateria ou reset zera o temporizador de acomodação. Num nó de campo
alimentado por solar, reset é evento **esperado**, não excepcional.

O resultado é publicar pH/EC de reserva estagnada como se fossem válidos, ou
reiniciar a espera indefinidamente.

---

## Bloqueadores antes de escrever a primeira linha

| # | Achado | Por quê bloqueia |
|---|---|---|
| 1 | Sentinelas não filtradas pela `M360Node` | Hoje elas vazam para o MQTT como leituras válidas |
| 2 | `minRechargeTimeMs` em `millis()` | Quebra permanente sob deep sleep — pH/EC nunca voltam |
| 3 | Duas linhas de MUX para cinco canais | Impede definir a assinatura do construtor |

Os demais são corrigíveis durante a implementação; estes três invalidam decisões
de API e de hardware que tudo o mais assume.
