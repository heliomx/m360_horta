# 🏗️ Arquitetura Interna — SU-xxT

Este documento registra **por que** a biblioteca é como é. Cada decisão abaixo
custou uma armadilha identificada em revisão adversarial do plano de implementação
— o relatório original está em
[`_bmad-output/implementation-artifacts/review_adversarial_plano_lib_su_xxt.md`](../../_bmad-output/implementation-artifacts/review_adversarial_plano_lib_su_xxt.md).

Ler antes de mudar qualquer coisa aqui. Várias dessas escolhas parecem arbitrárias
até se saber o que acontece sem elas.

---

## 1. Sentinelas abaixo de −32767

**A escolha:** todo código de erro é um float menor ou igual a `-32767.0f`.

**Por quê:** o descarte não acontece na `SU-xxT` — acontece na `M360Node`, e o
único filtro que ela aplica é este (`M360Node.cpp:249`):

```cpp
float val = _readCb(i);
if (isnan(val) || val <= -32767.0f) {
    continue;
}
```

Uma sentinela "óbvia" como `-999.0f` **atravessa** esse filtro e é publicada no
MQTT como leitura legítima — `pH = -999.0` no dashboard. Pior: por alternar com o
valor real, vence também o filtro de variação (`> 0.05f`) e transmite a cada ciclo.

Os códigos permanecem distintos entre si para diagnóstico local e `sendDebug()`,
mas todos caem abaixo do piso. O `-127.0f` da `DallasTemperature`
(`DEVICE_DISCONNECTED_C`) é traduzido na fronteira da biblioteca — sozinho, ele
também vazaria.

---

## 2. Recarga contada em ciclos, não em milissegundos

**A escolha:** `minRechargeCycles` conta **ciclos de amostragem consecutivos** com
`Nivel == 1`, não tempo.

**Por quê:** em AVR, `millis()` **não avança durante `sleep()` / `smartSleep()`**.
O nó dorme 15 a 30 min e acorda por algumas centenas de milissegundos. Um limiar de
30 min medido em `millis()` exigiria dezenas de dias de tempo-acordado acumulado —
na prática, `isRechargeSettled()` nunca viraria `true` e **pH e EC nunca voltariam
a ser publicados** depois do primeiro esvaziamento.

O tempo efetivo é `minRechargeCycles × intervalo`, e o intervalo é ajustável em
campo por `V_VAR1`. A conversão é de quem configura; `M360Node::getInterval()` dá
o valor vigente.

**Persistência:** o contador vive em RAM e some em brownout — evento esperado num
nó solar. Em reset ele volta a zero, o que é conservador por construção: atrasa a
republicação, nunca publica reserva estagnada como válida. Só a **transição** para
"acomodado" é persistida — uma escrita por reenchimento, não uma por ciclo, que
consumiria os 100 mil ciclos da EEPROM em poucos anos.

---

## 3. Multiplexador 74HC4051, três linhas de endereço

**A escolha:** 8:1 single-ended, endereçado por `A`/`B`/`C`, saída comum em **uma**
entrada do ADS1115.

**Por quê:** são **cinco** canais analógicos e duas linhas de endereço só alcançam
quatro. O `74HC4052` é duplo 4:1 (4 canais por banco, endereço compartilhado) e o
`74HC4053` é triplo SPDT com três seletores independentes — nenhum dos dois
endereça cinco canais por A/B. O 4051 resolve com um pino de MCU a mais e deixa
três canais livres para expansão (ORP, segunda célula de EC).

**Consequência de nomenclatura:** o valor de `SU_Channel` é o **canal físico**,
fixo pelo cobre da PCB. O **índice de enumeração** é outra coisa — varia com o
modelo populado, porque `getDeviceCount()` depende dele. Confundir os dois faz um
SU-20T (sem pH) ler EC no lugar de pH. `getChannelByIndex()` existe justamente
para traduzir entre os dois espaços.

---

## 4. Partição de trilhos: ADS1115 e MUX ficam permanentes

**A escolha:** o MOSFET canal P chaveia **apenas** os front-ends analógicos. O
ADS1115, o 74HC4051 e os pull-ups I2C ficam no trilho permanente (~1,5 µA).

**Por quê:** em single-shot o ADS1115 se autodesliga entre conversões (~0,5 µA) e
o 4051 fica na casa de 1 µA — desprezível frente ao consumo do nó dormindo. Em
troca, evitam-se dois defeitos:

- **Perda de configuração.** No trilho chaveado, o ADS1115 voltaria a cada ciclo
  com o registrador no default, descartando em silêncio o ganho por canal.
- **Pull-up contra escravo morto.** Pull-ups I2C vivos sobre um ADS sem
  alimentação injetam corrente pelos diodos de proteção — consumo parasita e
  alimentação fantasma do CI.

`requestReadings()` reaplica a configuração mesmo assim: é uma escrita de
registrador, barata, e torna a leitura independente do histórico de alimentação.

> ⚠️ **Contrapartida:** com os front-ends desligados, as entradas do 4051 ficam
> flutuando, e entrada CMOS flutuando consome e oscila. A PCB prevê pull-down alto
> em cada canal de entrada.

---

## 5. Ganho (PGA) por canal, não global

**A escolha:** tabela `const` em PROGMEM com `gain` e `dataRate` de cada canal.

**Por quê:** as faixas são incompatíveis. Umidade e nível varrem 0 a 3,3 V e
exigem fundo de escala `±4,096 V`. O pH oscila em torno de um offset de ~1,65 V,
numa janela estreita onde a resolução é o que decide o resultado — `±2,048 V`
dobra a resolução ali. Com ganho único, ou se satura a umidade, ou se joga fora a
resolução do pH.

Não é calibração de campo, é característica de projeto — daí ser `const`.

---

## 6. Excitação AC gerada pelo MCU

**A escolha:** o firmware dispara o trem de pulsos por `pinAcExcite`; o front-end
só faz buffer, retificação e hold. Fora da janela de amostragem o pino fica em
**alta impedância**.

**Por quê:** polarização DC em hastes de inox 316 **dentro** do lisímetro
eletrolisa o eletrodo e contamina justamente a solução que a câmara existe para
medir — contra a premissa de parede filtrante inerte, com deriva progressiva e
silenciosa em pH e EC.

Gerar no MCU também torna frequência e número de pulsos (`acExciteHz`,
`acExcitePulses`) parâmetros de calibração, ajustáveis sem trocar componente.

> **Os 10 ms citados na especificação de hardware são da excitação, não da
> conversão.** O ADS1115 converte o envelope já retificado, um sinal quase-DC — o
> teto de 128 SPS do ADC nunca foi obstáculo aqui.

---

## 7. Compensação de pH em três passos

**A escolha:** a calibração de 3 pontos guarda também `calibTempC`, e o fator de
Nernst **não** multiplica a leitura diretamente.

**Por quê:** o slope medido em campo já embute a temperatura em que a calibração
foi feita. Multiplicá-lo pelo fator de Nernst cru é **dupla compensação**, com erro
que cresce junto com o desvio térmico. Sem registrar `calibTempC`, a compensação
sequer é definível.

```
1. Slope empírico por segmento (dois segmentos, três pontos):
   S_acid = (V_pH4 - V_pH7) / 3        S_alk = (V_pH7 - V_pH10) / 3

2. Eficiência do eletrodo — adimensional, ~0,90 a 1,00,
   e é a grandeza que NÃO depende da temperatura:
   eff = S_segmento / S_nernst(calibTempC)
   onde S_nernst(T) = 0,05916 · (T + 273,15) / 298,15

3. Leitura, escolhendo o segmento pela comparação de V com V_pH7:
   pH = 7 + (V_pH7 - V) / (eff · S_nernst(T_medida))
```

**Premissa registrada:** `V_pH7` é o ponto isopotencial e é tratado como
independente da temperatura. Simplificação usual, boa perto de pH 7, pior nos
extremos.

**Efeito colateral bem-vindo:** o eletrodo entrega tensão linear em pH, então a
conta é só multiplicação e divisão. Nada de `log10()` nem `pow()` — economia de
flash que conta no ATmega328P.

---

## 8. Nível tem três estados

**A escolha:** `SU_LevelState` — `FILLED` (1), `DRY` (0) e `FAULT` (−1).

**Por quê:** `Nivel = 0` é publicado como **alerta de irrigação**. Com um retorno
`bool`, falha de instrumento seria indistinguível de câmara seca, e um ADS1115
mudo viraria recomendação de irrigar.

| Estado | pH / EC | Child `nivel_lisimetro` |
|---|---|---|
| `FILLED` | publicados, se `isRechargeSettled()` | `1` |
| `DRY` | `SU_ERR_LEVEL_LOW` | `0` — alerta legítimo |
| `FAULT` | `SU_ERR_ADC_FAULT` | **nada publicado** |

Na falha o nó **cala**. A ausência do dado é diagnosticável pelo timeout do
`M360Registry`; um `0` falso, não.

---

## 9. A EC deriva, e a biblioteca só torna isso visível

**A limitação, assumida:** o KCl que vaza da junção de referência do eletrodo de pH
enviesa a EC **para cima, progressivamente**, e durante a troca as hastes e o bulbo
leem águas diferentes. É consequência aceita da reserva de hidratação, não defeito
a consertar em firmware.

**O que a biblioteca faz:** exporta `getCyclesSinceRecharge()`, para que a EC possa
ser trendada contra a idade da amostra. EC subindo em fase com o contador, e caindo
a cada reenchimento, é assinatura de acúmulo — não de salinização do solo. Sem esse
eixo as duas curvas são indistinguíveis, e a leitura errada leva a decisão de
fertirrigação errada.

**Consequência operacional:** EC absoluta exige lavagem periódica da câmara.
Cadência **a definir por medição**.

---

## 10. Onde a biblioteca deliberadamente não vai

| Não faz | Quem faz | Por quê |
|---|---|---|
| Conhecer child IDs | `NODE_ITEMS[]` do nó | SSoT é o `inventario.md`; duas fontes divergem em silêncio |
| Falar MySensors | `noSU30T.cpp` | Camada física não conhece transporte |
| Definir endereço no mapa de EEPROM | `M360Config.h` declara a **região**; a lib reivindica a fatia | A lib core não conhece libs de aplicação |
| Corrigir a deriva da EC | Manutenção de campo | É química, não numérica |

---

## Armadilha de portabilidade — ESP

A biblioteca declara `espressif8266` e `espressif32`. Nessas plataformas a EEPROM é
**emulada**, e o MySensors a abre com `EEPROM.begin(512)` — uma escrita em 768
simplesmente não chega ao flash.

Rodando em ESP, `saveCalibration()` precisa reabrir com o tamanho necessário e
chamar `commit()`, a mesma dança que `M360::Config::save()` faz em
`M360Config.cpp:110-113`. No AVR o acesso é direto e nada disso se aplica.

O mapa de EEPROM pressupõe **1024 B** (ATmega328P); não vale para AVR de 512 B.
