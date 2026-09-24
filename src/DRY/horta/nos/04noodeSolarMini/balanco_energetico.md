# Balanço Energético — Nó 4 (Clima Solar Mini)

**Pergunta:** o Pro Mini 5V/16MHz funciona em `M360_LOW_POWER` com uma 18650 (3,7 V)
recarregada pelo painel solar de 6 V?

**Resposta:** sim, com folga. Mesmo no pior caso considerado — painel de 100 mA,
LED de energia aceso e leitura a cada 1 minuto — o painel gera **~3×** o que o nó
consome. Com o LED removido, a folga passa de **6×**.

> Análise de 10/09/2026. Os **tempos** vêm do código; as **correntes** são
> estimativas de datasheet, não medições na placa — ver §6 para confirmá-las.
> Hardware e ligações em [`esquema_eletrico.md`](esquema_eletrico.md).

---

## 1. Ciclo do nó

O ciclo `M360_LOW_POWER` está em `M360Node::process()`
([`lib/M360-DRY/src/M360Node.cpp`](../../../../../lib/M360-DRY/src/M360Node.cpp)):

| Etapa | Onde | Duração |
| :--- | :--- | :--- |
| `powerUp()` → liga D3 e aguarda DHT11/DS18B20 | `sensorDrivers.cpp`, `wait(1500)` | 1,5 s |
| Leitura e envio dos 3 children + bateria | `_readAndSendAll()`, `_processBattery()` | dezenas de ms |
| Janela mínima acordado | `M360_MIN_AWAKE_MS` = 3000 (`M360Config.h`) | 3,0 s |
| Espera de mensagens do smartSleep | `MY_SMART_SLEEP_WAIT_DURATION_MS` = 500 (MySensors 2.3.2) | 0,5 s |
| Sono | `smartSleep(intervalo × 60 000)` | resto do intervalo |

**Intervalo padrão de fábrica: 30 minutos** — `-D M360_DEFAULT_INTERVAL=30` no env
(`platformio.ini`), aplicado só com EEPROM virgem. Um nó já gravado mantém o valor
da EEPROM (1 min, antigo padrão da lib e pior caso para energia) até receber um
`INTERVAL_<min>` pelo dashboard (§5). Os cálculos abaixo continuam em 1 min.

**Warm-up pós-boot:** 10 ciclos de 30 s a cada reset — ~10 × 0,028 mAh ≈ 0,3 mAh
por boot, desprezível frente ao consumo diário; só pesa se o nó entrar em loop de
reset por brown-out.

Durante as esperas o `wait()` do MySensors mantém o rádio escutando, então os ~5 s
acordado custam praticamente o mesmo em qualquer etapa.

---

## 2. Consumo

### Acordado — ~20 mA por ~5 s

| Componente | Corrente |
| :--- | :--- |
| nRF24L01+ em recepção | ~13 mA |
| ATmega328P ativo a 16 MHz, ~4 V | ~6–8 mA |
| DHT11 + DS18B20 (só nos 1,5 s com D3 ligado) | ~2 mA |
| Transmissões | ms a ~11 mA — desprezível |

Por ciclo: ~5 s × 20 mA ≈ 100 mA·s ≈ **0,028 mAh**.

### Dormindo — ~15 µA

| Componente | Corrente |
| :--- | :--- |
| ATmega328P em power-down + watchdog, BOD desligado | ~4–6 µA |
| nRF24L01+ em power-down | ~1 µA |
| HT7333 (quiescente) | ~4 µA |
| TP4056 + DW01 + 8205A (dreno na bateria) | ~3–6 µA |

O brown-out não pesa no sono porque o MySensors o desliga antes de dormir
(`sleep_bod_disable()` em `hal/architecture/AVR/MyHwAVR.cpp`). Sem isso seriam
~20 µA a mais. Em 24 h, o sono soma só ~0,4 mAh.

### Total por dia

| Intervalo | LED de energia | Consumo/dia |
| :--- | :--- | ---: |
| 1 min | removido | ~40 mAh |
| 1 min | aceso (~1,8 mA contínuos) | ~85 mAh |
| 5 min | removido | ~8 mAh |
| 15 min | removido | ~3 mAh |

**Quem manda no consumo são os ~5 s acordado por ciclo**, não o sono. Dobrar o
intervalo praticamente divide o consumo por dois.

---

## 3. Geração

Premissas: ~5 horas de sol pleno por dia (DF) e **50 % de perda** — ângulo,
poeira, sombra da estufa ou do sombrite. Com o TP4056 (carregador linear), a
corrente que entra na bateria é a corrente do painel perto da tensão da bateria,
≈ corrente de curto-circuito (Isc ≈ 1,1 × Imp).

| Painel | Isc | Carga/dia |
| :--- | ---: | ---: |
| 6 V / 100 mA (especificação da estufa) | ~100 mA | ~250 mAh |
| 6 V / 1 W (BOM) | ~180 mA | ~450 mAh |
| 6 V / 2 W (BOM) | ~370 mA | ~900 mAh |

---

## 4. Margem e autonomia

### Geração ÷ consumo (intervalo de 1 min)

| Painel | LED removido | LED aceso |
| :--- | ---: | ---: |
| 100 mA | ~6× | ~3× |
| 1 W | ~11× | ~5× |
| 2 W | ~22× | ~10× |

### Autonomia sem sol nenhum

Base: 18650 real de ~2000 mAh, ~80 % aproveitável (≈ 1600 mAh). 18650 barata
costuma ter bem menos capacidade do que diz o rótulo — reduza na mesma proporção.

| Intervalo | LED | Autonomia |
| :--- | :--- | ---: |
| 1 min | aceso | ~19 dias |
| 1 min | removido | ~40 dias |
| 5 min | removido | ~6 meses |
| 15 min | removido | > 1 ano (a autodescarga passa a dominar) |

### Efeito no clock de 16 MHz

O ATmega328P só é garantido a 16 MHz acima de ~3,8 V. Com o painel repondo a
carga todo dia, a bateria passa quase o tempo todo entre 3,9 e 4,2 V — a região
fora da especificação só é alcançada depois de muitos dias sem sol.

---

## 5. Condições para a conclusão valer

| Condição | Obrigatória? | Motivo |
| :--- | :---: | :--- |
| nRF24L01+ alimentado pelo HT7333 | **Sim** | A bateria chega a 4,2 V; o máximo absoluto do rádio é 3,6 V |
| Bateria na sombra e ventilada | **Sim** | Os módulos TP4056 costumam vir com o pino TEMP aterrado — carregam em qualquer temperatura. Carregar Li-ion acima de ~45 °C degrada a célula rápido, e dentro da estufa ou de caixa fechada ao sol isso acontece fácil. Só o painel precisa de sol. **É o maior risco de longo prazo.** |
| LED de energia removido | Recomendada | Funciona com ele aceso, com metade da autonomia sem sol |
| Bateria no VCC, não no RAW | Recomendada | Evita o regulador de 5 V em dropout; e a leitura de bateria (child 255) depende disso |

**Alavanca de autonomia sem mexer no hardware:** aumentar o intervalo. Para
temperatura e umidade do ar da estufa, 5 min já sobram. O intervalo, em minutos,
é enviado pelo Node-RED no **child 254 (`V_VAR1`)** e fica gravado na EEPROM do
nó — não precisa regravar o firmware.

---

## 6. Como validar

As correntes da §2 são estimativas. Para confirmar:

1. **Corrente dormindo:** multímetro em série no positivo da bateria, escala de µA,
   com o nó dormindo. Esperado: ~15 µA. Se aparecer ~2 mA, o LED de energia ainda
   está na placa. Ligue o nó na escala de mA e só troque para µA com ele já
   dormindo — a queda no multímetro em µA pode resetar o nó quando ele acorda.
2. **Corrente acordado:** escala de mA — picos de ~20 mA durante ~5 s a cada ciclo.
3. **Em campo:** acompanhar o child 255 no Node-RED por uma semana. O esperado é a
   bateria voltar a ~4,1–4,2 V toda tarde de sol. Um dente de serra diário que
   **desce** de um dia para o outro indica déficit.

## Quando refazer esta análise

- Mudança em `M360_MIN_AWAKE_MS`, no `wait()` dos sensores ou no intervalo padrão
- Troca de painel, de bateria ou de posição do painel (sombra)
- Inclusão de sensor ou periférico que fique ligado durante o sono
