# Backend Definitivo M360 — Guia de Desenvolvimento

> Guia para substituir o Node-RED por um backend de produção no sistema M360 Horta.
> O Node-RED é o **backend temporário** e a implementação de referência: tudo que
> ele faz hoje está inventariado aqui, junto com o contrato que o firmware impõe
> e os modos de falha que já custaram sessões de debugging.

**Fontes de verdade — não duplicar aqui:**

| Assunto | Documento |
|---|---|
| Child IDs, labels, `S_*`/`V_*`, pinos, escalas por nó | [`src/DRY/horta/inventario.md`](../src/DRY/horta/inventario.md) |
| Regras de firmware e armadilhas de integração | [`CLAUDE.md`](../CLAUDE.md) |
| Implementação de referência (a ser substituída) | [`src/DRY/horta/nodered/flows.json`](../src/DRY/horta/nodered/flows.json) |
| Contrato de tradução MQTT ↔ MySensors | [`lib/M360-DRY/src/M360Translator.*`](../lib/M360-DRY/src) |

Quando este guia e o `inventario.md` divergirem sobre um child ID, **o
`inventario.md` vence** — e este documento está desatualizado.

---

## 1. Topologia

```
Nós MySensors (RF24 2.4 GHz, 250 kbps)
    │  Nó 1  Solo Canteiro A      (Nano,      LOW_POWER)
    │  Nó 2  Solo Canteiro B      (Nano,      LOW_POWER)
    │  Nó 4  SolarMini            (Pro Mini,  LOW_POWER)
    │  Nó 99 Central de Atuação    (Nano,      ALWAYS_ON / REPEATER)
    ▼
Gateway ESP8266 (Nó 0) ── WiFi ──► Broker MQTT ──► BACKEND (você está aqui)
                                    mqtt.viridiotech.com.br
```

O backend **nunca fala com os nós diretamente**. Todo o acesso é por MQTT, através
do gateway, que é a única ponte para o rádio. Não há caminho alternativo: se o
gateway estiver offline, o sistema inteiro está mudo — e o backend precisa dizer
isso explicitamente em vez de mostrar dados velhos.

### Prefixo de tópico

```
m360/{UF}/{CAR}
```

`UF` e `CAR` vêm da configuração do gateway em EEPROM (portal web, modo AP).
Instalação atual: `m360/DF/0000`. **O backend deve tratar o prefixo como
configuração**, nunca como literal — o Node-RED hardcodou `m360/DF/0000` em nós
da aba Irrigação, o que impede rodar duas instalações contra o mesmo código.

---

## 2. O contrato MQTT

O gateway compila em dois modos, selecionados por `-D M360_NATIVE_MQTT=1` no
`platformio.ini`. **A instalação atual usa o modo nativo.** O backend deve
suportar pelo menos o nativo; suportar os dois custa pouco e protege contra uma
troca de modo.

### 2.1 Modo nativo (ativo)

**Telemetria — gateway publica:**

```
m360/{UF}/{CAR}/out/{nodeId}/{childId}/{command}/{ack}/{type}
payload: valor bruto, sem aspas, sem JSON
```

**Comando — backend publica:**

```
m360/{UF}/{CAR}/in/{nodeId}/{childId}/{command}/{ack}/{type}
payload: valor bruto
```

O gateway assina `m360/{UF}/{CAR}/in/#`.

Exemplo — ligar a Solenóide do Canteiro A:

```
tópico:  m360/DF/0000/in/99/31/1/0/2
payload: 1
```

### 2.2 Os quatro tópicos que o backend precisa consumir

Assinar `m360/{UF}/{CAR}/out/#` captura os três primeiros — inclusive o tópico
`out` "pelado", porque no MQTT o curinga `#` casa também o nível pai. Por isso o
consumidor **precisa discriminar pelo número de segmentos**, não assumir formato:

| Tópico | Segmentos | Formato do payload | Conteúdo |
|---|---:|---|---|
| `{prefix}/out/{node}/{child}/{cmd}/{ack}/{type}` | 9 | valor bruto | Telemetria e ecos de atuação |
| `{prefix}/out` | 4 | JSON | **Heartbeat do gateway** |
| `{prefix}/out/events` | 5 | JSON | Eventos de transporte |
| `{prefix}/gateway/status` | 5 | JSON | Métricas da conexão MQTT |

> **Inconsistência conhecida:** no modo nativo o heartbeat continua indo para o
> tópico `out` pelado, em JSON, enquanto todo o resto vai para a forma
> estruturada. É assim porque `publishHeartbeat()` usa `buildTopicOut()`
> diretamente. Não corrija isso no firmware sem alinhar o backend — e vice-versa.

### 2.3 Modo envelope JSON (alternativo)

Com `M360_NATIVE_MQTT` desativado, tudo trafega como JSON no tópico plano
`{prefix}/out` e o gateway assina `{prefix}/in`. Campos produzidos por
`Translator::toJSON()`:

```json
{
  "nodeId": 99, "sensorId": 31, "destination": 0,
  "command": 1, "ack": 0, "type": 2,
  "payloadType": 1, "payload": "1",
  "timestamp": 51231, "description": "Status", "direction": "sensor"
}
```

`timestamp` é **uptime do gateway em segundos** (`millis()/1000`), não epoch.
Não use como relógio: o backend deve carimbar o horário de recebimento.

`direction` vale `"sensor"` ou `"transport_ack"` — ver §5.3.

### 2.4 Campos do heartbeat

Publicado periodicamente pelo gateway em `{prefix}/out`:

```json
{
  "nodeId": 0, "sensorId": 255, "command": 3, "ack": 0, "type": 22,
  "payload": "", "timestamp": 51231, "description": "heartbeat",
  "event": "heartbeat", "uptime": 51231, "rssi": -62, "wifiRssi": -62,
  "batteryLevel": 100, "ip": "192.168.0.42", "version": "2.0",
  "source": "gateway"
}
```

`uptime` reiniciando é o sinal mais confiável de que o gateway **rebootou** —
útil para invalidar estado que o backend tenha cacheado sobre a rede de rádio.

### 2.5 Eventos de transporte (`{prefix}/out/events`)

```json
{"event":"node_lost","nodeId":4,"timestamp":51231,"rssi":-62,
 "details":"Inactivity detected: timeout"}
```

Catálogo:

| `event` | Origem | Significado |
|---|---|---|
| `gateway_presented` | boot | Gateway subiu e apresentou-se |
| `node_discovered` | `I_DISCOVER_RESPONSE` | Nó respondeu à descoberta; `details` traz o parent |
| `node_reconnected` | primeira mensagem após timeout | Nó voltou |
| `node_lost` | registry timeout | Nó silencioso além do limite |
| `node_presentation` | `I_PRESENTATION` | Nó apresentou-se ao gateway |
| `child_presentation` | `C_PRESENTATION` | Child apresentado; `details` traz ID, tipo e label |
| `node_sketch_name` / `node_sketch_version` | `I_SKETCH_*` | Firmware do nó |
| `node_registration_request` | `I_REGISTRATION_REQUEST` | Nó pedindo registro |
| `node_config_request` | `I_CONFIG` | Nó pedindo configuração |
| `node_heartbeat` | `I_HEARTBEAT_RESPONSE` | Heartbeat de um nó |
| **`command_rejected`** | validação do gateway | Comando malformado; `details` traz o motivo |
| **`command_send_failed`** | `send()` retornou false | O rádio não conseguiu transmitir |

> **Lacuna a fechar:** o Node-RED **descarta todos esses eventos**. O
> `Decodificador Nativo` exige `nodeId` **e** `command` no payload, e eventos não
> têm `command`. Os dois eventos de comando existem justamente para dar
> diagnóstico imediato — o backend definitivo **deve** consumi-los.

---

## 3. Modelo de domínio

### 3.1 Faixas normativas de child ID

Definidas em `lib/M360-DRY/src/M360Constants.h` e válidas para todos os nós:

| Faixa | Domínio |
|---:|---|
| 1 – 10 | Solo |
| 11 – 20 | Clima |
| 21 – 30 | Hidrometria |
| 31 – 40 | Atuação |
| **253** | Debug remoto (`V_TEXT`) — nó → backend |
| **254** | Intervalo de reporte (`V_VAR1`) — bidirecional |
| **255** | Bateria (`V_VOLTAGE`) — nó → backend |

Todo nó apresenta 253, 254 e 255 automaticamente, além dos seus children.
O backend não deve tratá-los como sensores comuns na UI.

### 3.2 Escalas — o backend interpreta, o firmware não

| Grandeza | Nós | `V_*` | Unidade real |
|---|---|---|---|
| Umidade de solo | 1, 2 | `V_LEVEL` (37) | **ADC bruto 0–1023**, não percentual. Valor **alto = solo seco** (sensor resistivo) |
| Temperatura | 4, 99 | `V_TEMP` (0) | °C, 1 casa |
| Umidade do ar | 4, 99 | `V_HUM` (1) | %, 1 casa |
| Vazão | 99 (child 21) | `V_FLOW` (34) | L/min (YF-S201, K = 7,5 → 450 pulsos/litro) |
| Bateria | qualquer (255) | `V_VOLTAGE` (38) | volts, 1 casa |
| Atuação | 99 (31–39) | `V_STATUS` (2) | `"0"` / `"1"` |

> **`V_LEVEL` significa umidade de solo nos nós 1 e 2** por decisão histórica.
> O Node-RED remapeia `V_LEVEL → V_PERCENTAGE` quando o child foi apresentado
> como `S_MOISTURE`. O backend precisa da mesma regra, **baseada no tipo de
> apresentação do child**, não em lista fixa de nodeId.

Além do `V_VOLTAGE` bruto, os nós também emitem `I_BATTERY_LEVEL` (percentual,
`C_INTERNAL` type 0) para a UI nativa de bateria dos controladores.

### 3.3 Registro de nós

O registro é **dirigido pela apresentação do próprio nó**, nunca por lista
estática. Ao subir, cada nó envia:

1. `I_SKETCH_NAME` / `I_SKETCH_VERSION` — nome e versão do firmware. O nome tem
   sufixo do perfil de energia: `[LP]`, `[ON]`, `[PAS]`, `[REP]`.
2. Um `C_PRESENTATION` por child, com `type` = `S_*` e payload = label.

**O backend deve derivar tipo e rótulo daí**, e não hardcodar. Um nó regravado
com children diferentes precisa ser refletido sem alteração de código.

**Children fantasma:** ao renumerar children e regravar um nó, os IDs antigos
permanecem no registro. Limpar exige um comando **REPRESENT** (§5.5), que deve
zerar a tabela de children daquele nó antes de reconstruí-la.

**TTL:** o Node-RED usa 48 h (`172800000 ms`) para remover nós inativos. Nós
`LOW_POWER` podem ficar horas em silêncio — TTL curto demais os apaga
indevidamente.

---

## 4. Perfis de energia — por que a latência de comando varia

| Perfil | Nós | Comportamento | Latência de comando |
|---|---|---|---|
| `M360_ALWAYS_ON` | 99 | `wait(50)` no loop, nunca dorme | **< 50 ms** |
| `M360_REPEATER` | 99 (variante) | Igual ao acima + encaminha mensagens de terceiros | < 50 ms |
| `M360_LOW_POWER` | 1, 2, 4 | `smartSleep()` entre ciclos | **até 1 intervalo inteiro** |
| `M360_PASSIVE` | — | Acorda só para check-in; lê sob comando | até 1 intervalo |

**Consequência de projeto:** comandos para nós `LOW_POWER` **não são
interativos**. O `smartSleep()` entrega mensagens enfileiradas quando o nó
acorda — o backend precisa enfileirar e comunicar "pendente até o próximo
despertar", nunca esperar resposta em segundos. Só o Nó 99 responde na hora, e
por isso ele é o único que carrega atuadores.

O intervalo é ajustável em runtime pelo child **254** (`V_VAR1` ou `V_VAR5`),
aceito entre **1 e 1440 minutos**, e persistido na EEPROM do nó. O nó **sempre
responde com o valor vigente**, mesmo rejeitando a mudança — o backend descobre
se foi aceita comparando a resposta com o que pediu.

---

## 5. Comandos — o caminho crítico

Esta seção descreve o subsistema que mais falhou na implementação temporária.

### 5.1 Validação feita pelo gateway

O gateway **rejeita e publica `command_rejected`** em vez de repassar algo
degradado. O backend não precisa replicar, mas precisa **entender**:

| Rejeição | Motivo |
|---|---|
| Tópico sem exatamente 8 barras | Truncado, ou níveis extras |
| Segmento não numérico | Evita `atoi("xx") == 0`, que virava `childId 0` |
| `command` fora de {1, 2, 3} | Só `C_SET`, `C_REQ`, `C_INTERNAL` |
| `nodeId` fora de 1–255 | 0 é o próprio gateway |
| Payload > 25 bytes | `MAX_PAYLOAD_SIZE` do MySensors |
| **`C_SET`/`V_STATUS` com payload ≠ `"0"`/`"1"`** | Ver §5.2 |

### 5.2 A armadilha do `"1"` — leia antes de escrever qualquer comando

O nó lê valores booleanos com `MyMessage::getBool()`, que para payload string
faz **`atoi()`**. Portanto:

| Payload enviado | O nó entende | Efeito |
|---|---|---|
| `"1"` | 1 | liga |
| `"0"` | 0 | desliga |
| `"true"` | 0 | **desliga** |
| `"ON"` | 0 | **desliga** |
| `{"nodeId":99,...}` | 0 | **desliga** |

Toda entrada inválida degrada para **desligar**, indistinguível de um OFF
legítimo em qualquer log. Foi exatamente esse o defeito da irrigação automática
no Node-RED: os nós de função publicavam o envelope JSON como payload, o
gateway o repassava inteiro como string, e o solenoide **nunca ligava** — com
ACK confirmado e notificação de sucesso no caminho todo.

**Requisito para o backend:** a serialização do comando deve ser tipada, com o
payload validado antes de publicar. Um teste automatizado que publique cada
comando de atuação e afirme `payload ∈ {"0","1"}` paga por si.

### 5.3 Confirmação: eco de aplicação, não ACK de transporte

Existem **duas** confirmações possíveis, e só uma prova alguma coisa:

| Sinal | Quem emite | O que prova |
|---|---|---|
| ACK de transporte (`ack=1`, `direction: "transport_ack"`) | **O gateway**, ao `send()` retornar true | Apenas que o **próximo salto** de rádio respondeu com auto-ACK de hardware. Com o Nó 99 em modo repetidor, pode vir do repetidor |
| **Eco de aplicação** (`C_SET`, `V_STATUS`, `ack=0`) | **O nó**, depois de acionar o relé | Que o nó recebeu, aplicou e leu de volta o estado |

O gateway usa `send(outMsg, false)` deliberadamente — sem eco síncrono, para não
bloquear o loop. O ACK de transporte é publicado à mão logo em seguida.

**Critério correto de confirmação:**

```
mensagem.nodeId   == comando.nodeId
mensagem.childId  == comando.childId
mensagem.command  == 1        (C_SET)
mensagem.type     == 2        (V_STATUS)
mensagem.ack      == 0        (é eco do nó, não ACK de transporte)
mensagem.payload  == comando.payload
```

A comparação de payload não é preciosismo: o Nó 99 emite `V_STATUS=0` por conta
própria em dois casos legítimos (§6), e sem ela um desses ecos confirmaria um
comando de ligar.

**Consequência esperada e desejável:** comando para um child que não é atuador,
ou para um nó fora de alcance, agora **falha** em vez de ser falsamente
confirmado.

### 5.4 Política de retry

A referência usa: timeout inicial **5 s**, backoff exponencial (`5 s`, `10 s`,
`20 s`), **3 tentativas**, depois erro definitivo.

Comandos de atuação são **idempotentes** — reenviar "ligar" para um relé já
ligado é inofensivo. Isso torna o retry seguro.

**Corrigir na reimplementação:** o Node-RED usa **um único slot global** de
comando pendente para toda a horta — um comando para a Bomba NFT fica atrás de
um comando de solenoide. O certo é **uma fila por nó**, e para o Nó 99
especificamente uma fila por *grupo de concorrência* (§6.2).

### 5.5 Comandos especiais (`V_CUSTOM`, type 48)

Enviados como `C_SET` + `V_CUSTOM` para qualquer child (convencionalmente 0):

| Payload | Efeito no nó |
|---|---|
| `REPRESENT` | Reapresenta todos os children. Use após regravar firmware |
| `FORCE_UPDATE` | Lê e envia todos os sensores imediatamente, ignorando o filtro de variação |
| `DEBUG_NET` | Responde no child 253 com `P:<parent> D:<dist> R:<S/N>` |

Requisição de leitura pontual: `C_REQ` (command 2) para o child desejado — o nó
responde com o **último valor em cache**, sem reler o hardware.

---

## 6. Segurança de atuação — o backend **não** é a última linha

Esta é a inversão de responsabilidade mais importante do documento.

### 6.1 O failsafe vive no firmware

O Nó 99 desliga cada atuador sozinho ao estourar um tempo máximo, e **reporta
`V_STATUS=0`**:

| Children | Carga | Tempo máximo |
|---|---|---:|
| 31, 32, 33 | Solenóides de gotejamento | **600 s** |
| 34 – 37 | Peristálticas (suplemento, pH) | **120 s** |
| 38, 39 | Bombas NFT | **sem limite** — circulação contínua por projeto |

**Por que existe:** nenhuma camada acima do nó garante o comando OFF. É a única
proteção que sobrevive à queda do WiFi, do MQTT, do backend e do próprio gateway.

**O que isso exige do backend:**

1. **Nunca agendar uma rega mais longa que o limite do child.** Ela seria cortada
   pela metade e o único sinal seria um `V_STATUS=0` inesperado.
2. **Tratar `V_STATUS=0` não solicitado como verdade**, não como ruído. É o nó
   dizendo que desligou sozinho — provavelmente porque o backend falhou em
   mandar o OFF.
3. **Não substituir o failsafe por lógica própria.** Reduzir os tempos no
   firmware é uma decisão de firmware, documentada no `inventario.md` §7.3.

### 6.2 Concorrência do MUX no Nó 99

Os children **31–37** compartilham um multiplexador CD74HC4067: **apenas um pode
estar ligado por vez**, por proteção da fonte de 12 V. Ao receber um comando para
ligar um canal, o firmware **desliga o anterior** e emite `V_STATUS=0` do child
preemptado antes do eco do child comandado.

Sequência observada ao ligar 32 com 31 ligado:

```
out/99/31/1/0/2  →  "0"     (preempção — o firmware desligou)
out/99/32/1/0/2  →  "1"     (eco do comando)
```

Os children **38 e 39** (bombas nativas) são livres e concorrem sem restrição.

**O que isso exige do backend:**

- Modelar **31–37 como um grupo de exclusão mútua** e serializar os comandos.
  Duas regras de irrigação disparando em paralelo cortam uma à outra.
- Aceitar o `V_STATUS=0` do child preemptado como estado real e cancelar
  qualquer temporizador de desligamento pendente para ele.

### 6.3 O estado de atuação não é reportado periodicamente

O motor do nó pula atuadores no ciclo de leitura: só sensores são reportados por
intervalo. O estado dos relés chega **apenas** em três momentos: eco de comando,
eco de preempção, e disparo do failsafe.

**Consequência:** após um restart do backend, ele **não sabe** o estado dos
relés e não há como perguntar de forma barata. Opções, em ordem de preferência:

1. Persistir o estado conhecido e reconciliar pelos ecos subsequentes.
2. Enviar `C_REQ` por child de atuação no startup — o nó responde do cache.
3. Assumir desligado e comandar OFF explicitamente em todos — seguro, mas
   interrompe uma rega em andamento.

Após reboot do Nó 99 todos os relés estão **desligados** (`initSensors()` roda
antes do init do MySensors, e a ligação é NA/COM). Um `uptime` reiniciando no
nó é sinal de que o estado cacheado é inválido.

---

## 7. Regras de negócio a portar

Lógica agronômica hoje embutida nos nós de função do Node-RED. Deve virar
**configuração**, não código.

### 7.1 Canteiro A — controle tradicional (parcela de referência)

Temporizador fixo, sem sensores. É o **grupo de controle** do experimento
Manejo360; sua simplicidade é intencional e não deve ser "melhorada".

- Cron `00 07 * * *` e `00 17 * * *`
- Duração 300 s
- Atuador: Nó 99, child 31

### 7.2 Canteiro B — Manejo360 (parcela experimental)

Avaliado a cada 5 min. Atuador: Nó 99, child 32.

**M.1 — Sanitização.** Descartar leituras com mais de 30 min, e valores fora de
0–1023. Agregar por **mediana** dos canais válidos (robusta a um sensor solto).

**M.2 — Modulação climatológica.** Se `temp > 25 °C` **e** `umidade < 18 %`,
multiplicar a duração por **1,33**.

**M.3 — Limiares de solo** (ADC bruto, alto = seco):

| Mediana | Ação |
|---|---|
| < 350 | Solo úmido — não irrigar |
| 350 – 500 | Irrigar 15 s (base) |
| > 500 | Irrigar 45 s (base) |

**M.4 — Soak time.** Trava de absorção após cada rega: **20 min** se
`temp < 18 °C`, **15 min** caso contrário. Nenhuma rega nova antes disso.

> A mediana usa os children de solo do Nó 2. O clima vem do DHT11 do **Nó 99**
> (children 11 e 12) — não do Nó 4.

### 7.3 Acumulação de volume

Integração da vazão instantânea (`V_FLOW`, L/min) em volume (L):

```
dV = (vazao_L_min / 60) * dt_segundos
```

Só integra se `dt` estiver entre 0 e 300 s — janelas maiores indicam lacuna de
telemetria e extrapolá-las inventa consumo. Deve funcionar para **qualquer** nó
com child de vazão, não só o 99.

**Este é o contador que mais dói perder** (§8.2): hoje ele vive em RAM e zera a
cada restart do Node-RED.

### 7.4 Watchdogs

- **Gateway offline** — avaliado a cada 60 s contra o último heartbeat.
- **TTL de nós** — varrido a cada 300 s; nó sem contato há mais de 48 h sai do
  registro.

### 7.5 Notificações e relatório

- **Telegram** no início e fim de cada rega, com canteiro, duração, modo e
  diagnóstico.
- **Relatório diário 08:00** — contexto da rede enviado ao Gemini
  (`gemini-2.5-flash`), resposta em Markdown para o Telegram.

Ambos são **integrações externas** e devem ficar atrás de uma interface, com as
credenciais fora do código.

---

## 8. Requisitos não-funcionais — as lições do backend temporário

Cada item abaixo é um modo de falha **observado**, não uma boa prática genérica.

### 8.1 Agendamento durável

**O que quebrou:** o desligamento da irrigação é agendado com `setTimeout()`
dentro de um nó de função. Qualquer redeploy ou restart do Node-RED destrói o
temporizador. Uma rega iniciada às 07:00 com deploy às 07:02 **nunca recebe o
OFF** — e, sem o failsafe de firmware, a válvula fica aberta indefinidamente.

**Requisito:** toda ação futura é uma linha persistida com horário de execução,
recuperada no startup. Nada de temporizador em memória para ação de campo.
Reinício do processo não pode perder nem duplicar uma ação.

### 8.2 Estado persistente

**O que quebrou:** de todo o estado do Node-RED, **apenas `mys_nodes` é gravado
em disco**. Tudo o mais vive em RAM:

| Perdido a cada restart | Impacto |
|---|---|
| `hist_solo_A`, `hist_solo_B`, `hist_clima`, `hist_solar`, `hist_bateria`, `hist_vazao`, `hist_volume` | Histórico dos gráficos |
| `flow_vol_node_*` | **Volume de água acumulado** — contador de consumo zera |
| `solenoid_states`, `pump_states` | Estado dos relés na UI |
| `last_irrig_b_time` | Soak time zera — pode irrigar em seguida indevidamente |
| `gateway_online`, `gateway_last_seen` | Status do gateway |

**Requisito:** banco de dados. Série temporal para telemetria, tabelas
relacionais para registro de nós, comandos e agendamentos.

### 8.3 Confirmação real

**O que quebrou:** o Sincronizador aceitava o ACK de transporte fabricado pelo
gateway, então os 3 retries **nunca disparavam** na falha de última milha —
exatamente o caso para o qual existem. E `mys_nodes` registrava como estado do
nó um valor que só o gateway tinha afirmado.

**Requisito:** §5.3, e nunca gravar estado de dispositivo a partir de mensagem
com `ack=1`.

### 8.4 Configuração e segredos

**O que quebrou:** prefixo de tópico hardcodado em nós da aba Irrigação;
credenciais de Telegram e Gemini lidas de variáveis globais do runtime, com
fallback para string vazia — falha silenciosa.

**Requisito:** configuração externa validada no startup; falta de segredo
obrigatório é erro de inicialização, não degradação silenciosa.

### 8.5 Observabilidade

**O que quebrou:** comando malformado morria num `println` no console serial do
gateway. Do lado do Node-RED, o sintoma era um timeout de 35 s sem causa.

**Requisito:** consumir `{prefix}/out/events` (§2.5); log estruturado e
correlacionável de cada comando, do pedido ao desfecho; métrica de comandos
rejeitados, falhos e não confirmados.

### 8.6 Teste sem hardware

**O que faltou:** não há como exercitar a lógica sem a estufa ligada.

**Requisito:** o backend deve falar apenas MQTT, e o conjunto de testes deve
poder rodar contra um broker local com um **simulador de nó** — que reproduza o
eco de aplicação, a preempção do MUX e o failsafe. Isso torna testável tudo em
§5 e §6, que é onde os defeitos caros aparecem.

---

## 9. Arquitetura recomendada

Recomendação, não imposição. O que **não** é negociável é §8.

```
         ┌──────────────┐
MQTT ───►│   Ingestor   │──► normaliza → série temporal
         └──────┬───────┘                registro de nós
                │
         ┌──────▼───────┐
         │  Barramento  │  (eventos de domínio)
         └──┬────────┬──┘
            │        │
   ┌────────▼──┐  ┌──▼─────────────┐
   │ Motor de  │  │  Despachante   │──► MQTT out
   │  Regras   │  │  de Comandos   │◄── ecos (confirmação)
   └───────────┘  └──┬─────────────┘
                     │
              ┌──────▼──────┐
              │ Agendador   │  (durável, §8.1)
              └─────────────┘
```

### 9.1 Máquina de estados do comando

```
PENDENTE ──publica──► ENVIADO ──eco do nó──► CONFIRMADO
              │            │
              │            └──timeout──► (retry < 3) ──► ENVIADO
              │                             │
              │                             └──► FALHOU
              └──command_rejected──► REJEITADO
```

Persistir cada transição. `CONFIRMADO` só pelo critério de §5.3.

### 9.2 Esboço de esquema

```sql
-- Registro dirigido pela apresentação do nó (§3.3)
node        (node_id PK, sketch_name, sketch_version, power_profile,
             first_seen, last_seen, last_heartbeat, status)
child       (node_id FK, child_id, presentation_type, label,
             PRIMARY KEY (node_id, child_id))

-- Telemetria: série temporal, particionada por tempo
reading     (ts, node_id, child_id, v_type, value_raw, value_si, unit)

-- Comandos: ciclo de vida completo, para auditoria e retry
command     (id PK, ts_created, node_id, child_id, command, v_type, payload,
             state, attempts, ts_sent, ts_confirmed, reject_reason)

-- Ações futuras sobrevivendo a restart (§8.1)
scheduled_action (id PK, run_at, kind, payload_json, state, ts_executed)

-- Contadores que não podem zerar (§7.3)
counter     (node_id, child_id, kind, value, ts_updated)
```

`value_raw` guarda o que veio do rádio; `value_si` guarda o valor convertido
(§3.2). Manter os dois permite reinterpretar histórico se uma escala for
corrigida — impossível hoje.

### 9.3 Ordem de implementação sugerida

1. **Ingestor + persistência.** Só isso já resolve §8.2, o prejuízo diário.
   Pode rodar **em paralelo com o Node-RED**, apenas consumindo.
2. **Registro de nós e API de leitura.** Substitui `mys_nodes`.
3. **Despachante de comandos** com a confirmação de §5.3, ainda sem regras.
   Migrar os botões manuais primeiro — são de baixo risco e validam o caminho.
4. **Agendador durável.**
5. **Motor de regras** (§7.1, §7.2), com o Node-RED desligado.
6. **Notificações, relatório e UI.**

Os passos 1–3 convivem com o Node-RED. **Desligue as regras de irrigação de um
lado antes de ligar do outro** — duas fontes comandando os children 31–37
disparam a preempção do MUX (§6.2) e cortam a rega uma da outra.

---

## 10. Checklist de paridade

O backend definitivo só substitui o Node-RED quando cobre:

**Ingestão**
- [ ] Assinar `{prefix}/out/#` e discriminar por número de segmentos (§2.2)
- [ ] Decodificar formato nativo e envelope JSON
- [ ] Consumir `{prefix}/out/events` — hoje descartado (§2.5)
- [ ] Consumir `{prefix}/gateway/status`
- [ ] Carimbar horário de recebimento (o `timestamp` do gateway é uptime)

**Registro**
- [ ] Descoberta pela apresentação do nó, sem lista fixa (§3.3)
- [ ] `REPRESENT` limpa children fantasma
- [ ] TTL de 48 h compatível com nós `LOW_POWER`

**Comandos**
- [ ] Payload de `V_STATUS` tipado e validado como `"0"`/`"1"` (§5.2)
- [ ] Confirmação pelo eco de aplicação (§5.3)
- [ ] Retry 5 s / 10 s / 20 s, 3 tentativas
- [ ] Fila por nó, e grupo de exclusão mútua para os children 31–37 (§6.2)
- [ ] `REPRESENT`, `FORCE_UPDATE`, `DEBUG_NET`, ajuste de intervalo (child 254)

**Segurança**
- [ ] Nenhuma rega agendada acima do limite do failsafe (§6.1)
- [ ] `V_STATUS=0` não solicitado tratado como verdade
- [ ] Reconciliação de estado dos relés no startup (§6.3)

**Regras**
- [ ] Canteiro A: cron 07:00 / 17:00, 300 s (§7.1)
- [ ] Canteiro B: M.1 a M.4 (§7.2)
- [ ] Acumulação de volume com janela máxima de 300 s (§7.3)
- [ ] Watchdog de gateway e TTL de nós (§7.4)

**Operação**
- [ ] Agendamento durável (§8.1)
- [ ] Persistência de histórico, volume, estados e soak time (§8.2)
- [ ] Prefixo de tópico e segredos como configuração (§8.4)
- [ ] Log correlacionável por comando (§8.5)
- [ ] Testes contra broker local com simulador de nó (§8.6)

---

## 11. Anexo — o que já sangrou

Condensado, para revisão rápida antes de tocar no caminho de comando.

| Armadilha | Sintoma | Onde |
|---|---|---|
| Envelope JSON como payload no tópico nativo | Relé nunca liga, tudo reporta sucesso | §5.2 |
| `getBool()` = `atoi()` | Qualquer payload inválido **desliga** | §5.2 |
| ACK de transporte confundido com confirmação | "Confirmado" com o nó desligado; retries nunca disparam | §5.3 |
| `getSender()` vale 0 em mensagem enviada | ACK publicado com nodeId errado | §2.3 |
| Preempção do MUX não observada | Rega cortada pela metade, sem erro em lugar nenhum | §6.2 |
| `setTimeout` em nó de função | OFF perdido no redeploy; válvula aberta | §8.1 |
| Estado em RAM | Volume de água zera; soak time zera | §8.2 |
| `atoi()` em segmento de tópico | `childId 0`, comando descartado em silêncio pelo nó | §5.1 |
| Payload acima de 25 bytes | Truncado em silêncio pelo `MyMessage::set()` | §5.1 |
| Child renumerado sem atualizar o consumidor | Comando ignorado em silêncio; timeout sem causa | §3.3 |

---

## 12. Glossário

| Termo | Significado |
|---|---|
| **Nó** | Dispositivo MySensors com `MY_NODE_ID` próprio. Gateway é o nó 0 |
| **Child** | Sensor ou atuador dentro de um nó, endereçado por `childId` |
| **`C_*`** | Classe do comando MySensors: 0 apresentação, 1 SET, 2 REQ, 3 interno, 4 stream |
| **`V_*`** | Tipo do valor (`V_TEMP`, `V_STATUS`, …) — usado com `C_SET`/`C_REQ` |
| **`S_*`** | Tipo do sensor na apresentação (`S_BINARY`, `S_MOISTURE`, …) |
| **`I_*`** | Subtipo interno — usado com `C_INTERNAL`. Compartilha o espaço numérico de `V_*`, então só o `command` desambigua |
| **Eco de aplicação** | Mensagem que o nó envia após aplicar um comando, com o estado lido de volta |
| **ACK de transporte** | Confirmação de salto de rádio, publicada pelo gateway. Não prova entrega ao nó |
| **Preempção** | Desligamento automático de um canal MUX ao ligar outro |
| **Failsafe** | Desligamento autônomo do atuador pelo firmware ao estourar o tempo máximo |
| **Soak time** | Intervalo mínimo entre regas, para absorção da água pelo solo |
