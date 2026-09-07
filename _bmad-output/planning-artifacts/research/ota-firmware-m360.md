# Estudo de Viabilidade — Atualização Remota de Firmware (OTA) — M360 Horta

**Data:** 31/08/2026
**Tipo:** Pesquisa técnica (fase 1-analysis) — **não altera código, hardware ou fluxos**
**Escopo de rádio:** nRF24L01+ (frota atual), RFM69 e RFM95W/LoRa (migração planejada)

---

## 1. Sumário executivo

Hoje toda atualização de firmware do M360 Horta exige cabo USB — no gateway (COM5) e em cada
nó. Não existe nenhuma infraestrutura de OTA no projeto: a única ocorrência de `MY_OTA_*` é
`MY_OTA_LOG_SENDER_FEATURE` nos dois gateways, que é relay de log por rádio e não tem relação
com atualização de firmware.

### Veredito por alvo

| Alvo | Veredito | Bloqueio determinante |
|---|---|---|
| **Gateway ESP8266** (Horta e Kit Hélio) | **Viável agora, sem pré-requisito** | Nenhum. ~346 KB de folga de flash, bibliotecas já no core |
| **Nós 01, 02, 11** (`M360_ALWAYS_ON`) | **Viável com custo de hardware** | Bootloader + memória de staging + passagem de `C_STREAM` no gateway |
| **Nó 99** (`M360_REPEATER`) | **Viável, porém apertado** | 8.888 B de flash e 547 B de RAM livres — o orçamento precisa ser medido, não estimado |
| **Nó 04** (`M360_LOW_POWER`, solar) | **Inviável sem mudança de firmware** | Acorda ~3 s por hora; precisa de comando de janela acordada |
| **Servidor de firmware** | **Ausente** | Ninguém serve o `.hex` em blocos — pré-requisito de tudo acima |

### Recomendação em uma frase

Fazer o **OTA do gateway já** (é barato, isolado e não depende de mais nada), e para os nós
adotar **DualOptiboot com memória de staging** — não `MYSBootloader` — porque só o DualOptiboot
sobrevive à migração para RFM95/RFM69 sem uma segunda visita com gravador ISP a cada nó.

---

## 2. Contexto e motivação

O custo real de uma atualização de firmware hoje não é o tempo de compilação: é o deslocamento
até o nó. Isso já pesa na estufa MVP e piora de forma não-linear com a migração planejada para
**RFM95W/LoRa**, cujo propósito declarado é justamente aumentar o alcance
(`hardware/adapter_rfm95w_nrf24socket/`, e `Manejo360 com Rede LoRa Neutra.md`). Alcance maior
significa nós mais distantes; nós mais distantes significam visitas mais caras. **A mesma
decisão que torna a rede maior é a que torna OTA um pré-requisito, não uma conveniência.**

Há também um custo silencioso: sem OTA, regras embarcadas — o failsafe de tempo máximo ligado
do Nó 99, os limiares de solo, o intervalo de reporte — tendem a migrar para o Node-RED só
porque lá dá para mudar sem visita. Isso desloca lógica de segurança para longe do atuador, o
que é o oposto do que se quer num failsafe.

### Decisões que balizam este estudo

| Decisão | Escolha registrada |
|---|---|
| Escopo desta entrega | Somente o estudo — nenhum código alterado |
| Rádios considerados | nRF24L01+, RFM69 e RFM95W |
| Estratégia de campo | **OTA agora na frota nRF24; migração de rádio depois** |
| Lado controlador | Levantar o requisito e estimar — não projetar o fluxo Node-RED |
| Frota analisada | Nós 01, 02, 04, 11 e 99, com veredito individual |

---

## 3. Linha de base medida

Todos os números desta seção foram **medidos**, não estimados.

### 3.1 Consumo de flash e RAM dos nós

Medido em 31/08/2026 com
`avr-size --mcu=atmega328p -C <workspace>/build/<env>/firmware.elf`
(workspace `%TEMP%/pio_builds/%USERNAME%/m360_horta`), após `pio run` de todos os envs.

Os percentuais são contra os **30.720 B utilizáveis** (32.768 B menos 2.048 B de bootloader),
que é o critério do PlatformIO. O `avr-size` reporta contra 32.768 B e por isso mostra um
percentual menor.

| env | Perfil | Flash | % de 30.720 | RAM | % de 2.048 | Folga flash |
|---|---|---|---|---|---|---|
| `nano_01nodeSolo3d` | `ALWAYS_ON` | 19.724 B | 64,2 % | 1.018 B | 49,7 % | 10.996 B |
| `ProMini_01nodeSolo3d` | `ALWAYS_ON` | 19.724 B | 64,2 % | 1.018 B | 49,7 % | 10.996 B |
| `nano_02nodeSolo3d` | `ALWAYS_ON` | 19.724 B | 64,2 % | 1.018 B | 49,7 % | 10.996 B |
| `pro16MHz_miniDHT` | `ALWAYS_ON` | 20.360 B | 66,3 % | 789 B | 38,5 % | 10.360 B |
| `nano_99reles` | `REPEATER` | 21.832 B | 71,1 % | **1.501 B** | **73,3 %** | **8.888 B** |
| `ProMini_04noodeSolarMini` | `LOW_POWER` | 22.492 B | 73,2 % | 894 B | 43,7 % | **8.228 B** |

Os dois nós com menos folga são justamente os dois casos especiais: o **Nó 99** (repetidor, 9
atuadores, maior pressão de RAM) e o **Nó 04** (o único a bateria).

### 3.2 Gateway ESP8266

| Item | Valor | Origem |
|---|---|---|
| Placa | `d1_mini`, `espressif8266@2.6.3` (core 2.7.4) | `src/DRY/horta/platformio.ini:54` |
| Flash total | 4 MB | `d1_mini.json`, `maximum_size: 4194304` |
| Layout | `eagle.flash.4m1m.ld` (padrão, sem override no projeto) | `d1_mini.json`, `ldscript` |
| Espaço de sketch | `irom0_0_seg len = 0xfeff0` = **1.044.464 B (~1.020 KB)** | `eagle.flash.4m1m.ld` |
| Binário atual | **345.376 B (~337 KB)** | `build/d1_mini_gateway/firmware.bin` |

**Orçamento de OTA em 2 passos.** O ESP8266 grava a imagem nova numa segunda área antes de
trocar, logo exige `tamanho_do_sketch <= espaço_livre / 2`:

```
337 KB x 2 = 674 KB  <  1.020 KB     →  folga de ~346 KB
```

Cabe com folga confortável. **Nenhuma alteração de `ldscript` é necessária.** O gateway poderia
mais que dobrar de tamanho antes de o OTA em 2 passos deixar de caber.

### 3.3 Os cinco bloqueios

| # | Bloqueio | Evidência |
|---|---|---|
| 1 | **O gateway rejeita `C_STREAM`** | `lib/M360-DRY/src/M360Translator.cpp:101-104` (`validate()`) e `:133` (`fromNative()`) aceitam apenas `C_SET`, `C_REQ` e `C_INTERNAL`. `C_STREAM` (4) é o comando que transporta FOTA. |
| 2 | **Payload trafega como `const char*`** | `fromNative()` recebe `const char* payload` e faz `outMsg.set(payload)` (`M360Translator.cpp:150`). Blocos de firmware são binários e contêm `0x00` — não sobrevivem a um caminho de string. |
| 3 | **Bootloader errado** | `nanoatmega328` e `pro16MHzatmega328` usam `ATmegaBOOT_168_atmega328.hex` (o legado de 2 KB), não Optiboot nem DualOptiboot. Nenhum env define `board_bootloader.*`. |
| 4 | **Sem memória de staging** | Nenhuma flash SPI ou EEPROM I2C em qualquer `esquema_eletrico.md`, BOM ou projeto KiCad. O único dispositivo no barramento SPI de todos os nós é o nRF24 (CE=D9, CSN=D10). |
| 5 | **Ninguém serve o firmware** | Nenhum `ST_FIRMWARE_*` em `src/DRY/horta/nodered/flows.json`. As 3 ocorrências da palavra "firmware" são comentários sobre o failsafe do Nó 99. |

> **Nota de método:** a verificação do item 5 foi feita contra o **backup versionado**
> (`flows.json`), não contra o servidor. A chamada MCP `list-tabs` retornou `404 page not
> found`; conforme CLAUDE.md, não houve retentativa em laço. Confirmar no servidor antes de
> tratar este ponto como definitivo.

### 3.4 Perfis de energia e sua consequência direta

`lib/M360-DRY/src/M360Node.cpp:108-143`:

- **`M360_ALWAYS_ON` e `M360_REPEATER`** (Nós 01, 02, 11, 99): o loop termina em `wait(50)`
  perpétuo. `wait()` roda `_process()` do MySensors, logo **o rádio está sempre sendo servido**.
  É a condição ideal para FOTA — o nó consegue pedir e receber blocos continuamente.
- **`M360_LOW_POWER`** (Nó 04): janela acordada de `M360_MIN_AWAKE_MS` = 3.000 ms
  (`M360Config.h:100`), seguida de `smartSleep(intervalo)`. Com `M360_DEFAULT_INTERVAL=60`,
  são **~3 s acordado por hora**.
- **`M360_PASSIVE`**: `process()` é `smartSleep()` puro, sem janela nenhuma. Nenhum nó usa
  hoje, mas o perfil é estruturalmente incompatível com FOTA como está escrito.

### 3.5 Pinos livres por nó — o que decide SPI flash vs EEPROM I2C

| Nó | Pinos ocupados | Livres | I2C (A4/A5) livre? | Staging viável |
|---|---|---|---|---|
| 01 / 02 | A0–A5 (6 canais de solo), D3 (power) | D2, D4–D8 | **Não** — A4/A5 são canais de solo | Flash SPI (CS em D4–D8) |
| 99 | D2, D3, D4–D8 (MUX), A0, A1 (relés) | A2–A5 | **Sim** | Ambos |
| 04 | D3, D4, D5, A0 | D2, D6–D8, A1–A5 | **Sim** | Ambos |
| 11 | D4 | quase tudo | **Sim** | Ambos |

Nos Nós 01 e 02, usar EEPROM I2C custaria **dois canais de umidade de solo** — o que
descaracteriza o nó. Para eles, flash SPI é o caminho.

### 3.6 Estado do adaptador RFM95W

`hardware/adapter_rfm95w_nrf24socket/README.md` — placa de 28x28 mm, RFM95W de um lado e header
macho 2x4 do outro, plugando no soquete fêmea da YL-105 (o mesmo do nRF24). Mapeamento:
CE→**NC**, CSN→NSS, SCK/MOSI/MISO diretos, IRQ→DIO0, RESET→**NC**.

Duas lacunas abertas **anteriores ao OTA**, registradas aqui porque afetam o planejamento:

1. **Não há SDA/SCL no conector 2x4.** Uma memória de staging I2C não pode morar na placa do
   adaptador — tem de ir na placa principal do nó. Troca de rádio e adição de staging são,
   portanto, duas alterações físicas distintas.
2. **RESET em NC e dependência de DIO0 chegar ao pino IRQ.** O driver RF24 atual do MySensors
   não usa a linha IRQ, então esse pino pode estar flutuante ou não roteado na placa principal
   dos nós. O driver RFM95 **depende** de DIO0 para sinalizar TxDone/RxDone. Verificar no
   esquema de cada nó antes de considerar a migração resolvida.

---

## 4. Parte A — OTA do Gateway ESP8266

### 4.1 Os três mecanismos disponíveis

Todos os três já vêm no core `framework-arduinoespressif8266` 3.20704.0 — **nenhuma entrada
nova em `lib_deps`**.

| Mecanismo | Como funciona | Serve para quê | Limitação |
|---|---|---|---|
| `ArduinoOTA` | Push via `espota.py`, gateway anuncia por mDNS | Bancada e rede local | Exige o operador na mesma LAN; mDNS costuma não atravessar VLAN/WiFi isolado |
| `ESP8266HTTPUpdateServer` | Rota `/update` no servidor web já existente, upload de `.bin` pelo navegador | Manutenção pontual, inclusive **em modo AP** | Manual, um gateway por vez |
| `ESP8266httpUpdate` | **Pull**: o gateway baixa o `.bin` de uma URL | **Campanha remota disparada por MQTT** | Exige servidor HTTP hospedando o binário |

**Recomendação:** implementar `ESP8266HTTPUpdateServer` **e** `ESP8266httpUpdate`. O primeiro é
a rede de segurança (funciona mesmo com o MQTT caído e mesmo em modo AP, que é exatamente o
estado em que um gateway problemático costuma estar); o segundo é o que de fato entrega
atualização remota sem ninguém no local. `ArduinoOTA` é dispensável — o
`ESP8266HTTPUpdateServer` cobre o mesmo caso de uso sem depender de mDNS.

### 4.2 Onde o código deve morar

**`lib/M360-DRY/src/M360OTA.{h,cpp}`, guardado por `#ifdef ESP8266`**, espelhando o padrão de
`M360Webserver.{h,cpp}`.

A razão é concreta: `src/DRY/horta/gateway/libDryGatewayMqtt.cpp` (662 linhas) e
`src/DRY/kit-helio/gateway/libDryGatewayMqtt.cpp` (~640 linhas) são **gêmeos copy-paste sem
fonte comum**. Qualquer código de OTA escrito direto no sketch teria de ser duplicado nos dois
e divergiria na primeira correção. A lib já é o lugar canônico para infraestrutura de gateway —
`M360Webserver`, `M360WiFi`, `M360MQTT` e `M360Leds` seguem essa regra, e CLAUDE.md a torna
explícita ("Lógica de infraestrutura em `libDryGatewayMqtt.cpp`" é padrão proibido).

### 4.3 Ponto de integração no loop

`M360Gateway::loop()` (`lib/M360-DRY/src/M360Gateway.cpp:72-110`) termina com:

```cpp
// Sempre executar — inclusive em modo AP
if (_webHandler) _webHandler();
if (_ledUpdate)  _ledUpdate();
```

Há duas opções:

| Opção | Prós | Contras |
|---|---|---|
| **A** — pendurar na lambda `webHandler` existente | Zero alteração em `M360Gateway`; já roda em modo AP | Mistura responsabilidades; obriga a editar os dois sketches gêmeos |
| **B** — adicionar `onLoop(std::function<void()>)` a `M360Gateway` | Hook explícito e reutilizável; segue o padrão de `onHeartbeat()`/`onNodeCheck()` (`M360Gateway.h:144-152`) | Toca a lib (mudança pequena e retrocompatível) |

**Recomendação: opção B.** O padrão de callback já existe na classe, o destrutor já é virtual
(`M360Gateway.h:96`), e um hook nomeado deixa claro no sketch o que está sendo servido. Importa
que o novo hook seja invocado **fora** do `if (!isAPMode())`, como já são `_webHandler` e
`_ledUpdate` — senão o OTA de recuperação não funciona justamente no modo em que ele mais serve.

**Custo de latência:** o único bloqueio por iteração é `wait(1)`
(`M360Gateway.cpp:85`, ~1 ms, documentado como inevitável em `M360Gateway.h:167-169`), e ele é
pulado em modo AP. 1 ms por iteração é tolerável para `handleClient()`, mas alonga um POST
grande. Vale medir o tempo de upload na primeira implementação.

### 4.4 Reuso obrigatório

| Necessidade | O que já existe |
|---|---|
| Reportar progresso e falha | `publishTransportEvent()` (`libDryGatewayMqtt.cpp:634`) — publica em `.../out/events`. Usar `ota_start`, `ota_progress`, `ota_end`, `ota_failed` |
| Feedback visual durante a gravação | `setLedState()` / `ledFlicker()` (`M360Leds.h:33-37`) |
| Montar tópico | `buildTopicOut()` / `buildTopicIn()` — nunca literal, conforme CLAUDE.md |
| Disparo por MQTT | `processMQTTCommandNative()` (`libDryGatewayMqtt.cpp:540`) |

### 4.5 Envs PlatformIO

O OTA por `espota` exige env separado, **herdando** o atual para preservar a via serial de
recuperação:

```ini
[env:d1_mini_gateway_ota]
extends = env:d1_mini_gateway
upload_protocol = espota
upload_port = <ip-do-gateway>
upload_flags =
    --auth=<senha>
```

**Armadilha obrigatória de registrar:** o env atual traz

```ini
upload_flags =
    --before=default_reset
    --after=hard_reset
```

Esses são argumentos do **`esptool`** e são **inválidos para o `espota.py`** — se forem
herdados sem override, o upload falha. O `upload_flags` tem de ser **sobrescrito**, não
estendido. O mesmo vale para o env do Kit Hélio.

Diferença entre os dois sub-projetos que afeta a implementação: o env da Horta usa **glob**
(`build_src_filter = +<src/DRY/horta/gateway/*.cpp>`), então um arquivo novo na pasta compila
sozinho; o do Kit Hélio nomeia o arquivo explicitamente e exigiria edição do filtro.

### 4.6 Segurança

`setupWebServer()` (`lib/M360-DRY/src/M360Webserver.cpp:203-208`) registra **duas rotas** e
**nenhuma autenticação**, e o `server.begin()` roda incondicionalmente — inclusive em modo STA.
Uma rota `/update` acrescentada a esse mesmo objeto herdaria essa exposição na LAN inteira.

Requisitos mínimos:
- `ESP8266HTTPUpdateServer::setup(server, usuario, senha)` — nunca a variante sem credencial.
- Senha de OTA distinta da senha do AP e da do WiFi.
- Considerar autenticar também `/` e `/save`, hoje abertos — a rota `/save` já permite
  reconfigurar MQTT e WiFi e provocar `ESP.restart()` (`M360Webserver.cpp:199-200`).

### 4.7 EEPROM — sem colisão, com uma armadilha

Mapa atual (`lib/M360-DRY/src/M360Config.h:11-23`): 0–511 MySensors (reservado), 512–515
`M360NodeConfig`, 516–520 reservados para expansão, 521+ `M360DeviceConfig` com CRC.

Não há colisão estrutural: no ESP8266 a EEPROM emulada vive em setor próprio e o `Update.h`
nunca a toca. Mas duas cautelas:

1. **Acrescentar campo a `M360DeviceConfig` invalida o CRC.** Todo gateway já em campo cairia
   em `Config::reset()` e subiria em **modo AP** no boot seguinte — uma "atualização" que
   derruba a frota. Estado de OTA deve usar os bytes **516–520**, explicitamente reservados.
2. `Config::save()` re-chama `EEPROM.begin(...)` internamente (`M360Config.cpp:113`) porque o
   MySensors chama `EEPROM.begin(512)`. Qualquer caminho de OTA que toque EEPROM precisa
   respeitar a mesma dança.

---

## 5. Parte B — OTA dos nós AVR

### 5.1 Como o FOTA do MySensors funciona

O nó é quem puxa. O fluxo, sobre `C_STREAM`:

1. Nó envia `ST_FIRMWARE_CONFIG_REQUEST` com tipo/versão do firmware que tem.
2. Controlador responde `ST_FIRMWARE_CONFIG_RESPONSE` com tipo/versão/tamanho/CRC do firmware
   que **deveria** ter. Se diferirem, o nó entra em modo de atualização.
3. Nó pede bloco a bloco com `ST_FIRMWARE_REQUEST`; controlador devolve `ST_FIRMWARE_RESPONSE`.
4. Cada bloco é gravado na **memória de staging**.
5. Ao completar, o nó valida o CRC e reinicia. O **bootloader** copia o staging para a flash
   de programa.

Constantes verificadas na cópia vendorizada
(`.../libdeps/nano_99reles/MySensors/core/MyOTAFirmwareUpdate.h`):

| Constante | Valor | Observação |
|---|---|---|
| `FIRMWARE_BLOCK_SIZE` | **16 B** | Vale 16 quando `MAX_PAYLOAD_SIZE >= 22`; cai para 8 abaixo disso (`:68-71`) |
| `MAX_PAYLOAD_SIZE` | **25 B** | `MAX_MESSAGE_SIZE`(32) − `HEADER_SIZE`(7) — **igual para todos os rádios** |
| `MY_OTA_RETRY` | 5 | `:77` |
| `MY_OTA_RETRY_DELAY` | 500 ms | `:80` |
| `FIRMWARE_START_OFFSET` | 10 | "DualOptiboot wants to keep a signature first" (`:84`) |

**Consequência prática:** como `MAX_PAYLOAD_SIZE` é 25 para nRF24, RFM69 e RFM95 igualmente, o
bloco é sempre de **16 bytes**. Um firmware de 20 KB são **1.280 blocos**, cada um com uma ida
e uma volta. Isso é o que faz a análise de vazão da §5.5 importar.

### 5.2 Os dois bootloaders

| | `MYSBootloader` | `DualOptiboot` |
|---|---|---|
| Como recebe o firmware | **O próprio bootloader fala rádio** e pede os blocos | Não fala rádio; o **sketch** recebe e grava no staging, o bootloader só copia no boot |
| Memória de staging externa | **Não precisa** | **Obrigatória** (flash SPI ou EEPROM I2C) |
| Rádios suportados | **Só nRF24** | **Qualquer um** — é agnóstico por construção |
| Custo de hardware por nó | Zero | Uma flash SPI ou EEPROM I2C soldada |
| Caminho oficial MySensors 2.3.2 | Não | **Sim** (`MY_OTA_FIRMWARE_FEATURE`) |
| Manutenção upstream | Baixa, projeto praticamente parado | Ativa, é o caminho documentado |

Ambos exigem **gravador ISP** para instalar e ajustar o fuse `BOOTSZ` — os nós hoje têm o
`ATmegaBOOT` legado de 2 KB.

> **A verificar em bancada:** o tamanho exato do `MYSBootloader` e o valor de `BOOTSZ` que ele
> exige. Se precisar de mais de 2.048 B de seção de boot, a flash útil da aplicação cai abaixo
> dos 30.720 B atuais, o que aperta ainda mais o Nó 99 e o Nó 04.

### 5.3 Compatibilidade por rádio — o eixo decisivo

| Bootloader | nRF24L01+ | RFM69 | RFM95W / LoRa |
|---|---|---|---|
| `MYSBootloader` | **Sim** | **Não** | **Não** |
| `DualOptiboot` | **Sim** | **Sim** | **Sim** |

A razão é estrutural e não uma limitação temporária: o `MYSBootloader` embute um **driver
nRF24 dentro do bootloader**, porque é ele quem conversa com o gateway. Suportar outro rádio
significaria embutir outro driver em ~2 KB de seção de boot. O `DualOptiboot`, por outro lado,
**nunca toca no transceptor** — quem recebe os blocos é o sketch, através da camada MySensors
normal, com o rádio que estiver configurado. Por isso ele atravessa qualquer migração de rádio
sem ser reescrito nem regravado.

**Esta tabela é a conclusão central da Parte B.** Combinada com a decisão de campo registrada
("OTA agora, rádio depois"), ela é analisada na Parte C.

### 5.4 Memória de staging: flash SPI vs EEPROM I2C

| | Flash SPI (W25Q80 / AT25DF512C) | EEPROM I2C (24LC256) |
|---|---|---|
| Macro | padrão (`MY_OTA_FLASH_SS`, `MY_OTA_FLASH_JDECID`) | `MY_OTA_USE_I2C_EEPROM` |
| Padrões da lib | `MY_OTA_FLASH_SS` = **8** (D8), `MY_OTA_FLASH_JDECID` = **0x1F65** (`MyConfig.h:1244,1252`) | `MY_OTA_I2C_ADDR` = **0x50** (`MyConfig.h:192-196`) |
| Barramento | SPI — **compartilhado com o rádio** | I2C — independente do rádio |
| Pinos | 1 CS além de SCK/MOSI/MISO já usados | SDA/SCL (A4/A5) |
| Capacidade mínima | 1 Mbit sobra | **24(L)C256 é o mínimo** (`MyConfig.h:186`) |
| Aplicável a 01/02 | **Sim** (CS em D4–D8) | **Não** — A4/A5 são canais de solo |
| Driver incluído | `drivers/SPIFlash/SPIFlash.cpp` | `drivers/I2CEeprom/I2CEeprom.cpp` |

Dois pontos que a documentação da lib deixa explícitos e que mudam a decisão:

1. **`MY_OTA_FLASH_JDECID` tem de casar com o chip.** O padrão 0x1F65 é do AT25DF512C. Um
   W25Q80 (JEDEC 0xEF40) não é reconhecido sem redefinir a macro — falha silenciosa clássica.
2. **`MY_OTA_USE_I2C_EEPROM` exige um DualOptiboot especial.** O comentário em `MyConfig.h:187`
   é literal: *"Note that you also need an updated DualOptiboot supporting I2C EEPROM!"* O
   DualOptiboot padrão só lê flash SPI. Isso adiciona risco de fornecimento ao caminho I2C.

**Recomendação: flash SPI.** Ela funciona nos cinco nós (o caminho I2C está barrado nos Nós 01
e 02 por falta de A4/A5), usa o DualOptiboot padrão sem variante especial, e mantém uma única
configuração de hardware para toda a frota — o que importa mais do que a economia de um pino
quando o gargalo é a logística de visita, não o custo do componente.

### 5.5 Vazão e tempo de campanha por rádio

Premissas: bloco de 16 B (§5.1); pacote de pedido = 7 B de header + 6 B = **13 B**; pacote de
resposta = 7 B + 22 B = **29 B**; firmware de referência de **20 KB = 1.280 blocos**; preâmbulo
LoRa de 8 símbolos, CRC ligado, header explícito; **sem** retransmissão e **sem** latência de
controlador. Os tempos abaixo são portanto **pisos**, não previsões.

| Rádio / configuração | Bitrate efetivo | ms por bloco (ida+volta) | 20 KB |
|---|---|---|---|
| **nRF24** 250 kbps (config atual) | 250 kbps | ~3 ms | **~4 s** |
| **RFM69** FSK BR55.5 (padrão) | 55,5 kbps | ~12 ms | **~15 s** |
| **RFM95** `BW500CR45SF128` (SF7, BW500) | ~21,9 kbps | ~28 ms | **~36 s** |
| **RFM95** `BW125CR45SF128` (**padrão**, SF7, BW125) | ~5,5 kbps | ~113 ms | **~2,4 min** |
| **RFM95** `BW31_25CR48SF512` (SF9, BW31,25) | ~275 bps | ~2,37 s | **~51 min** |
| **RFM95** `BW125CR48SF4096` (SF12, BW125) | ~146 bps | ~3,69 s | **~79 min** |

Configurações do RFM95 conforme a tabela em `MyConfig.h:859-875`; padrão é
`RFM95_BW125CR45SF128`. Padrão do RFM69 é `RFM69_FSK_BR55_5_FD50` (`MyConfig.h:783`).

Na prática, para nRF24 e RFM69 o gargalo **não é o rádio** — é a latência do controlador
(MQTT → Node-RED → MQTT → gateway). Com 20–30 ms por ida e volta, uma campanha realista fica
entre 30 s e 2 min nesses dois. Para o RFM95 em SF alto, o rádio passa a dominar com folga.

#### O achado que decide a configuração LoRa

`MyOTAFirmwareUpdate.cpp:63-71`:

```cpp
if (_firmwareUpdateOngoing && (enterMS - _firmwareLastRequest > MY_OTA_RETRY_DELAY)) {
    if (!_firmwareRetry) {
        // Give up. We have requested MY_OTA_RETRY times without any packet in return.
        ...
    }
    _firmwareRetry--;
    ...
}
```

Com `_firmwareRetry` iniciado em `MY_OTA_RETRY + 1` (`:113`, `:281`), o nó **desiste após
(5+1) x 500 ms = 3 s sem resposta**.

Nas duas configurações LoRa de longo alcance, a **resposta sozinha** leva 1,5 s (SF9/BW31,25) e
2,24 s (SF12/BW125) só de tempo de ar. Somando o pedido e qualquer latência de controlador, a
janela de 3 s é estourada **antes do primeiro bloco legítimo chegar** — a atualização aborta
sem que nada esteja errado com o enlace.

> **Conclusão:** os defaults de FOTA do MySensors são calibrados para nRF24. Adotar RFM95 em SF
> alto **exige** elevar `MY_OTA_RETRY_DELAY` (e provavelmente `MY_OTA_RETRY`) por env, ou o
> FOTA simplesmente nunca completa. Isto é uma incompatibilidade concreta, não teórica, e não
> aparece em nenhum log como erro de configuração — aparece como "OTA falhou".

### 5.6 Orçamento de flash e RAM por nó

O custo de `MY_OTA_FIRMWARE_FEATURE` é a soma de `core/MyOTAFirmwareUpdate.cpp` mais o driver
de staging (`drivers/SPIFlash/SPIFlash.cpp` ou `drivers/I2CEeprom/I2CEeprom.cpp`). **Este
número não foi medido** — medi-lo exige compilar com a feature ligada, o que está fora do
escopo desta entrega.

| Nó | Folga flash | Folga RAM | Veredito preliminar |
|---|---|---|---|
| 01 / 02 | 10.996 B | 1.030 B | Confortável |
| 11 | 10.360 B | 1.259 B | Confortável |
| **99** | **8.888 B** | **547 B** | **Medir antes de prometer.** É o nó com mais atuadores e mais estado; a RAM é o risco maior, não a flash |
| **04** | **8.228 B** | 1.154 B | Flash é o risco; e o perfil de energia é bloqueio à parte (§5.7) |

**Ação recomendada antes de qualquer compromisso de cronograma:** compilar os Nós 99 e 04 com
`-D MY_OTA_FIRMWARE_FEATURE` e o driver de staging, e medir. É uma medição de 10 minutos que
elimina a maior incerteza técnica do épico.

### 5.7 O nó que dorme (Nó 04)

O Nó 04 é onde OTA mais vale a pena — é solar, tende a ficar mais longe — e é o único
estruturalmente incapaz de recebê-lo hoje: acorda ~3 s por hora, e uma campanha de 1.280 blocos
não cabe nisso.

**Ponto de inserção natural.** O dispatch de `V_CUSTOM` em `M360Node::handleMessage()`
(`M360Node.cpp:200-233`) já trata `REPRESENT`, `FORCE_UPDATE` e `DEBUG_NET`, com as constantes
em `M360Constants.h:47-49`. Um comando novo — `OTA_WINDOW`, por exemplo — entraria ali, e
`process()` (`M360Node.cpp:108-143`) ganharia um ramo que substitui o `smartSleep()` por
`wait()` enquanto a janela estiver aberta.

**Riscos que o desenho tem de tratar desde o início:**

- **Dreno de bateria.** Um nó preso acordado por campanha que falhou drena a solar em horas. A
  janela precisa de **prazo máximo absoluto**, expirando sozinha mesmo sem nenhuma mensagem —
  a mesma lógica do failsafe de tempo máximo ligado do Nó 99.
- **Como o comando chega.** O nó só escuta durante os 3 s de janela; o comando de abrir a
  janela tem de ser enfileirado pelo gateway e entregue nesse instante. Isso interage com o
  `M360Registry` e com a cadência observada — vale conferir se `smartSleep()` já cobre o caso
  via mensagem pendente.
- **Bateria mínima.** Não iniciar campanha abaixo de um limiar de tensão. O nó já reporta
  bateria no child 255.

### 5.8 O bloqueio do gateway — `C_STREAM` e payload binário

Este é o bloqueio menos visível e o mais fácil de subestimar: **mesmo com bootloader e staging
resolvidos, o FOTA não passa pelo gateway do M360 como ele está escrito hoje.**

Dois pontos, ambos em `lib/M360-DRY/src/M360Translator.cpp`:

```cpp
// validate(), :101-104
const uint8_t cmd = msg.getCommand();
if (cmd != C_SET && cmd != C_REQ && cmd != C_INTERNAL) {
    return M360_CMD_ERR_COMMAND;
}
```

```cpp
// fromNative(), :133 e :150
if (command != C_SET && command != C_REQ && command != C_INTERNAL) {
    return M360_CMD_ERR_COMMAND;
}
...
outMsg.set(payload != NULL ? payload : "");
```

O que precisaria mudar:

1. **Aceitar `C_STREAM` (4)** nos dois pontos — de preferência num caminho separado, não
   afrouxando a validação do caminho de atuação, que existe por uma razão documentada
   (`M360Translator.cpp:106-117`: o payload de `V_STATUS` só pode ser `"0"` ou `"1"`, porque
   `getBool()` faz `atoi()` e qualquer lixo **desliga** o relé).
2. **Carregar payload binário.** `fromNative()` recebe `const char*` e chama `set(payload)`,
   que trata como string terminada em NUL. Blocos de firmware contêm `0x00`. O formato nativo
   do MySensors resolve isso com **codificação hexadecimal** no payload MQTT — e a própria lib
   já prevê o padrão: `MyMessage.h:316-317` documenta um buffer de `2 * MAX_PAYLOAD_SIZE + 1`
   "to be able to fit hex-conversion of a full binary payload".
3. **Sentido de subida também.** `Translator::toJSON()` e `buildNativeTopic()` estringam o
   payload da mesma forma; o `ST_FIRMWARE_CONFIG_REQUEST` que sobe do nó precisa chegar
   íntegro ao controlador.

> **Isto altera o contrato com o Node-RED.** Conforme CLAUDE.md, mudar um lado sem o outro
> quebra o sistema em silêncio. A §9 de `funcionalidades_nodered.md` teria de ser atualizada na
> mesma entrega, e o `Decodificador Nativo` teria de aprender a não tratar `command === 4`
> como telemetria.

**Um efeito colateral positivo a considerar:** o `MYSBootloader` conversa com o gateway
**direto pelo rádio**, sem passar pelo `Translator` — ele fala com o controlador via o
protocolo MySensors nativo. Isso significa que o bloqueio de `C_STREAM` pesa sobretudo no
caminho `DualOptiboot` + `MY_OTA_FIRMWARE_FEATURE`. **A verificar:** se o
`MY_OTA_LOG_SENDER_FEATURE` já ativo indica que parte do caminho de stream funciona.

### 5.9 Lado controlador — requisito e estimativa

Conforme decidido, esta seção **levanta o requisito e estima**; não projeta o fluxo.

**O que um servidor de firmware precisa fazer:**

1. Manter um repositório de firmwares indexado por `type` e `version`, com o CRC e o número de
   blocos pré-calculados.
2. Responder `ST_FIRMWARE_CONFIG_REQUEST` dizendo qual firmware aquele nó deveria ter.
3. Fazer o parse do **Intel HEX** produzido pelo PlatformIO
   (`build/<env>/firmware.hex`) para um vetor de bytes contíguo.
4. Servir `ST_FIRMWARE_REQUEST` devolvendo os 16 B do bloco pedido — **com latência baixa o
   bastante para caber na janela de 3 s da §5.5**.
5. Manter estado por nó (bloco atual, tentativas, resultado) e expor progresso.

**Estado atual:** nada disso existe. Nenhum `ST_FIRMWARE_*` em `flows.json`.

**Estimativa comparativa:**

| Opção | Esforço | Prós | Contras |
|---|---|---|---|
| **Aba FOTA no Node-RED** | Alto — parser Intel HEX, servidor de blocos, máquina de estados por nó, UI de campanha | Integra ao dashboard e ao Telegram existentes; sem PC dedicado; SSoT única | É o maior item de software do épico. O `function` node servindo 1.280 blocos precisa de cuidado com latência |
| **MYSController (desktop)** | Baixo — ferramenta pronta | Caminho batido da comunidade; valida o resto da pilha rapidamente | Windows, PC conectado, fora do dashboard, fora do Git. Não serve como solução permanente |

**Recomendação:** usar o **MYSController para o PoC** — ele isola a validação de bootloader,
staging e rádio de todo o risco de software de controlador — e só depois decidir sobre a aba
Node-RED, já sabendo que o resto da pilha funciona. Provar cinco coisas ao mesmo tempo é o
jeito mais rápido de não descobrir qual delas quebrou.

---

### 5.10 Especificidades do Arduino Pro Mini

Metade da frota é Pro Mini: `ProMini_01nodeSolo3d`, `ProMini_04noodeSolarMini` e
`pro16MHz_miniDHT`. O microcontrolador é o mesmo ATmega328P dos Nano, então tudo de §5.1 a §5.9
se aplica sem alteração. O que muda é a **logística de gravação** e um conjunto de armadilhas de
fuse e de tensão que não existem no Nano.

#### A. Sem USB embarcado — o OTA rende mais aqui

O Pro Mini não tem conversor USB-serial na placa. Atualizar um hoje exige levar um adaptador
FTDI/CH340 e conectá-lo ao header de 6 pinos (DTR, TXO, RXI, VCC, GND, GND). Contra o Nano, que
só precisa de um cabo USB, o custo de status quo já é maior — **o retorno do OTA é maior nos nós
Pro Mini do que nos Nano**.

#### B. ISP sem header ICSP

O DualOptiboot exige gravador ISP, e clones de Pro Mini raramente trazem o header 2x3 ICSP
soldado. Isso não é bloqueio: SCK (D13), MISO (D12), MOSI (D11), RST, VCC e GND estão todos
disponíveis nos headers laterais, e é por ali que se grava.

Cuidado de procedimento: **D11/D12/D13 são o barramento SPI do rádio**. A gravação depende de o
CSN do nRF24 permanecer alto durante o ISP. O procedimento de campo deve prever **desplugar o
módulo de rádio do soquete antes de gravar** — o soquete YL-105 torna isso trivial e elimina a
dúvida de uma vez.

#### C. Os fuses são iguais; o que muda é o binário do bootloader

| | `pro16MHzatmega328` | `pro8MHzatmega328` |
|---|---|---|
| `f_cpu` | 16000000L | 8000000L |
| lfuse / hfuse / efuse | **0xFF / 0xDA / 0xFD** | **0xFF / 0xDA / 0xFD** |
| Binário do bootloader | `ATmegaBOOT_168_atmega328.hex` | `ATmegaBOOT_168_atmega328_pro_8MHz.hex` |
| `maximum_size` | 30.720 B | 30.720 B |
| `upload.speed` | 57600 | 57600 |

Os fuses são **idênticos** nas duas placas — a diferença física é o ressonador (8 ou 16 MHz), e a
diferença de software é o divisor de baud compilado dentro do bootloader.

Consequência: escolher o `board` errado **não** grava fuse de clock errado, mas grava um
**bootloader com o divisor de UART errado**. A via serial de recuperação passa a falar na metade
(ou no dobro) da velocidade combinada, e não há mensagem de erro que aponte a causa — o sintoma é
"o upload parou de funcionar".

#### D. O descompasso do Nó 04 — resolver antes da visita com ISP

| Fonte | Declara |
|---|---|
| `src/DRY/horta/nos/04noodeSolarMini/04noodeSolarMini.cpp` (cabeçalho) | Arduino Pro Mini **3.3V/8MHz** |
| `src/DRY/horta/nos/04noodeSolarMini/esquema_eletrico.md` | Arduino Pro Mini (ATmega328P, **3.3V / 8MHz**) |
| `src/DRY/horta/platformio.ini:201` | `board = pro16MHzatmega328` (**5V/16MHz**) |

Duas fontes documentais dizem 8 MHz; o build diz 16 MHz. Uma das duas está errada, e **este
estudo não determina qual** — isso exige olhar a placa física.

Por que importa para OTA em particular: a visita com gravador ISP é exatamente o momento em que
essa ambiguidade deixa de ser inofensiva. Gravar o bootloader de 16 MHz num Pro Mini de 8 MHz (ou
o inverso) quebra a via serial de recuperação — que é justamente a rede de segurança de que se
depende quando um OTA dá errado. **Confirmar a placa física do Nó 04 antes de qualquer gravação
de bootloader.**

Se a placa for mesmo de 8 MHz, o descompasso já afeta o firmware atual, independentemente de OTA:
`F_CPU` dobrado desloca todos os temporizadores, o `MY_BAUD_RATE=115200` sai a 57600, e as janelas
de `smartSleep()` e do DHT ficam com o dobro da duração pretendida.

#### E. `upload_speed` muda junto com o bootloader

`ProMini_01nodeSolo3d` e `ProMini_04noodeSolarMini` fixam `upload_speed = 57600`, que é a
velocidade do ATmegaBOOT. Um bootloader derivado de Optiboot a 16 MHz normalmente fala **115200**.
Após a troca, o `upload_speed` do env tem de acompanhar — senão a recuperação serial falha na
primeira vez em que for necessária, que é a pior hora possível para descobrir isso.

#### F. Uma oportunidade: o bootloader menor devolve flash

`hfuse = 0xDA` decodifica para `BOOTSZ[1:0] = 01` = **1.024 words = 2.048 B** de seção de boot.
É daí que saem os 30.720 B utilizáveis usados em todo o §3.1.

O DualOptiboot é derivado do Optiboot e é substancialmente menor que o ATmegaBOOT legado. Se
couber em 1.024 B (`BOOTSZ = 10`, `hfuse = 0xDC`), a flash de aplicação sobe para **31.744 B — um
ganho líquido de 1.024 B**. Isso incide justamente nos dois nós apertados:

| Nó | Folga hoje | Folga com boot de 1 KB |
|---|---|---|
| 04 (`ProMini_04noodeSolarMini`) | 8.228 B | **9.252 B** |
| 99 (`nano_99reles`) | 8.888 B | **9.912 B** |

> **A verificar em bancada, junto com a fase 0:** o tamanho real do DualOptiboot compilado para
> esta configuração. Se couber em 1 KB, parte do custo de flash do `MY_OTA_FIRMWARE_FEATURE` se
> paga sozinha na própria troca de bootloader.

#### G. O BOD já está habilitado — não desabilitar

`efuse = 0xFD` decodifica para `BODLEVEL = 101`: brown-out detection ativa em **2,7 V**.

Essa proteção é usada diretamente pelo OTA. O DualOptiboot copia o staging para a flash de
programa **durante o boot**, e uma queda de tensão no meio dessa cópia deixa o nó em brick. Num nó
solar com bateria descarregando, isso é cenário real, não hipotético.

Ao ajustar consumo do Nó 04, **não desabilitar o BOD por fuse**. A economia é de poucos µA e o
preço é o único mecanismo que impede uma gravação corrompida com bateria fraca.

#### H. Tensão de I/O e o adaptador RFM95W

Ponto que liga o Pro Mini à migração de rádio e que não está registrado em nenhum outro lugar do
projeto:

- O **nRF24L01+ tolera 5 V nas entradas digitais** — é por isso que os módulos YL-105 funcionam
  ligados direto num Nano de 5 V.
- O **RFM95W não é 5V-tolerante.** As entradas digitais do SX1276 são especificadas para
  VDD + 0,3 V, com VDD de 3,3 V.

O README do adaptador trata da **alimentação** ("a YL-105 já regula para 3.3V via AMS1117
on-board — seguro para o RFM95W"), mas alimentação e **níveis lógicos** são coisas diferentes:
SCK, MOSI e NSS vêm do microcontrolador, não do regulador.

| Nó | Placa | Lógica em | Adaptador RFM95W direto? |
|---|---|---|---|
| 01, 02, 99 | Nano | 5 V | **Não** — exige translação de nível |
| 11 | `pro16MHzatmega328` | 5 V | **Não** — exige translação de nível |
| 04 | Pro Mini 3,3 V (se confirmado — ver D) | 3,3 V | **Sim** |

Ou seja: **o Pro Mini de 3,3 V é a única plataforma da frota que aceita o adaptador RFM95W sem
hardware adicional.** Esse é um argumento forte para que o "nó OTA-ready de referência" das
próximas placas seja Pro Mini 3,3V/8MHz — a mesma escolha resolve OTA e migração de rádio de uma
vez.

> **A verificar:** o esquemático `adapter_rfm95w_nrf24.kicad_sch`, para confirmar que não há
> translação de nível embarcada. O README não menciona nenhuma e descreve a placa como contendo
> apenas U1 (RFM95W) e J1 (header), o que indica que não há.

## 6. Parte C — Impacto da migração RFM69/RFM95

### 6.1 O ponto central

A decisão de campo registrada é **"OTA agora, rádio depois"**. Isso torna a escolha do
bootloader mais consequente do que ela pareceria, porque **ela determina quantas visitas com
gravador ISP a frota vai receber no total**.

| | `MYSBootloader` agora | `DualOptiboot` agora |
|---|---|---|
| Visita 1 (habilitar OTA, hoje) | ISP apenas — **sem solda** | ISP **+ soldar a flash SPI** |
| Funciona na frota nRF24 atual? | Sim | Sim |
| Visita 2 (migração RFM95) | **ISP de novo em cada nó** — trocar por DualOptiboot, soldar staging, e só então plugar o adaptador | **Só plugar o adaptador no soquete** |
| Total de visitas com ISP | **2** | **1** |
| Total de operações de solda | 1 (na visita 2) | 1 (na visita 1) |

O `MYSBootloader` parece mais barato porque adia a solda — mas adia para uma visita em que
também será preciso reabrir a caixa, regravar o bootloader e reconfigurar tudo. Como o
adaptador RFM95W foi **projetado justamente para ser plugável no soquete do nRF24 sem alterar a
placa principal**, o `DualOptiboot` transforma a migração de rádio numa operação de campo de
minutos. O `MYSBootloader` desperdiça essa propriedade do adaptador.

Some-se a isso que o `DualOptiboot` é o caminho oficial e mantido do MySensors 2.3.2, e o
`MYSBootloader` é um projeto essencialmente parado.

**Recomendação: `DualOptiboot` + flash SPI, mesmo custando mais na primeira visita.**

### 6.2 O que ainda não está resolvido na migração

Independentemente do OTA, dois itens do §3.6 precisam de resposta antes de a migração RFM95 ser
planejável: **DIO0 chega ao pino IRQ do soquete na placa principal de cada nó?** e **a ausência
de RESET é aceitável para o driver RFM95 do MySensors?** Se a resposta ao primeiro for não em
algum nó, aquele nó precisa de um fio-jumper — o que muda o custo da "visita 2" para ele.

### 6.3 RFM69 como alternativa intermediária

O RFM69 raramente é considerado porque a conversa costuma ser "nRF24 ou LoRa". Vale registrar
que, para OTA, ele é o melhor dos dois mundos: **alcance muito superior ao nRF24 e vazão 100x
superior à do LoRa em SF alto** (§5.5: ~15 s contra ~79 min para 20 KB). Se o objetivo do
RFM95 for alcance e não interoperabilidade com redes LoRaWAN neutras, o RFM69 merece uma
medição de alcance antes de a decisão ser fechada — e o `DualOptiboot` funciona com ele
igualmente, sem nenhuma mudança no plano de OTA.

---

## 7. Riscos

| Risco | Impacto | Mitigação |
|---|---|---|
| **Queda de energia durante a gravação do bootloader** | Nó em brick, exige ISP | Fazer a gravação com fonte estável; nunca em campo com bateria fraca |
| **Imagem corrompida chegando íntegra ao CRC** | Nó sobe com firmware quebrado | O DualOptiboot valida CRC antes de copiar; garantir que o `.hex` publicado é o mesmo que foi testado |
| **Sem rollback automático** | Firmware ruim exige visita — o oposto do objetivo | Manter a versão anterior no repositório; considerar watchdog que reverte. **Sem rollback, um OTA ruim é pior que nenhum OTA** |
| **Nó fica sem escutar após atualização parcial** | Perde-se o canal de correção remota | Nunca atualizar toda a frota de uma vez; começar por um nó de menor criticidade (o 11, do Kit Hélio) |
| **FOTA aborta em silêncio no LoRa SF alto** | Campanha nunca completa, sem causa aparente | §5.5 — ajustar `MY_OTA_RETRY_DELAY` por env |
| **Nó 04 preso acordado** | Bateria drenada | Prazo máximo absoluto da janela (§5.7) |
| **Nó 99 estoura RAM com a feature** | Não compila, ou pior: compila e trava em campo | Medir antes de prometer (§5.6) |
| **Mudança em `M360DeviceConfig` derruba gateways** | Frota inteira em modo AP | Usar bytes 516–520 (§4.7) |
| **Bootloader de clock errado no Pro Mini** | Via serial de recuperação quebrada, sem erro que aponte a causa | Confirmar a placa física do Nó 04 antes de gravar (§5.10-D); os fuses são iguais, mas o binário do bootloader não |
| **BOD desabilitado por engano ao tunar consumo** | Brick por queda de tensão durante a cópia do staging | `efuse = 0xFD` (BOD 2,7 V) é proteção do OTA — não mexer (§5.10-G) |
| **RFM95W ligado a lógica de 5 V** | Entradas do SX1276 fora de especificação | Só o Pro Mini 3,3 V aceita o adaptador direto; Nano e `pro16MHz` exigem translação de nível (§5.10-H) |
| **Versionamento de firmware inexistente** | Não se sabe o que roda em cada nó | O FOTA exige `type`/`version`; adotar isso já resolve — hoje só há a string de `sendSketchInfo` |

---

## 8. Recomendação faseada

| Fase | Entrega | Depende de | Esforço | Valor |
|---|---|---|---|---|
| **0** | **Medir** o custo de `MY_OTA_FIRMWARE_FEATURE` + driver de staging nos Nós 99 e 04; medir o tamanho do DualOptiboot compilado; **confirmar a placa física do Nó 04** (8 ou 16 MHz) | nada | ~30 min | Elimina a maior incerteza técnica do épico e a única ambiguidade que pode brickar um nó na visita |
| **1** | **OTA do gateway** — `M360OTA` na lib, rota `/update` autenticada, `ESP8266httpUpdate` por MQTT, env `espota`, eventos em `/out/events` | nada | Baixo | Alto e imediato. Os dois gateways deixam de exigir cabo |
| **2** | **Passagem de `C_STREAM`** no `Translator` + payload hex, e atualização da §9 de `funcionalidades_nodered.md` | 1 | Médio | Nenhum sozinho — é infraestrutura para a fase 3 |
| **3** | **PoC de FOTA num nó** — DualOptiboot + flash SPI no **Nó 11** (Kit Hélio, o menos crítico), servido por MYSController | 0, 2 | Alto | Prova a pilha inteira com risco contido |
| **4** | **Rollout** nos Nós 01, 02 e 99 | 3 | Médio | O ganho operacional de fato |
| **5** | **Servidor de firmware no Node-RED** (se a fase 3 justificar) | 3 | Alto | Campanha sem PC dedicado |
| **6** | **Janela acordada do Nó 04** | 3 | Médio | Fecha a frota |

A ordem não é arbitrária: a fase 1 entrega valor sem depender de nada; a fase 0 custa minutos e
pode **matar ou redimensionar** as fases 3–6 antes de qualquer investimento; e a fase 3 escolhe
deliberadamente o nó cuja falha não para a irrigação.

---

## 9. Referências

**Código do projeto:**
- `lib/M360-DRY/src/M360Translator.cpp:95-153` — `validate()` e `fromNative()`, o bloqueio de `C_STREAM`
- `lib/M360-DRY/src/M360Node.cpp:108-143` — `process()` e os perfis de energia
- `lib/M360-DRY/src/M360Node.cpp:200-233` — dispatch de `V_CUSTOM`, ponto de inserção da janela
- `lib/M360-DRY/src/M360Gateway.cpp:72-110` — o loop e seus hooks
- `lib/M360-DRY/src/M360Gateway.h:144-152` — padrão de callback (`onHeartbeat`, `onNodeCheck`)
- `lib/M360-DRY/src/M360Webserver.cpp:203-208` — `setupWebServer()`, onde as rotas nascem
- `lib/M360-DRY/src/M360Config.h:11-23` — mapa de EEPROM
- `src/DRY/horta/gateway/libDryGatewayMqtt.cpp:296-345` — `setup()` e `loop()` do gateway
- `src/DRY/horta/platformio.ini:54-96` — env `d1_mini_gateway`
- `hardware/adapter_rfm95w_nrf24socket/README.md` — o adaptador RFM95W

**Biblioteca MySensors 2.3.2** (cópia vendorizada em `libdeps/nano_99reles/MySensors/`):
- `core/MyOTAFirmwareUpdate.h:68-84` — tamanho de bloco, retries, offset do DualOptiboot
- `core/MyOTAFirmwareUpdate.cpp:63-71` — a lógica de desistência de 3 s
- `MySensors.h:155-164` — a escolha entre `SPIFlash.cpp` e `I2CEeprom.cpp`
- `MyConfig.h:184-196` — `MY_OTA_USE_I2C_EEPROM` e a exigência de DualOptiboot especial
- `MyConfig.h:1240-1252` — `MY_OTA_FLASH_SS` e `MY_OTA_FLASH_JDECID`
- `MyConfig.h:782-801` — tabela de configurações do RFM69
- `MyConfig.h:841-875` — tabela de configurações do RFM95
- `core/MyMessage.h:41-64, 316-317` — tamanhos de mensagem e o padrão de conversão hex

**Documentos:**
- `docs/RFM95_96_97_98W.pdf` — datasheet HopeRF
- `docs/mysensors.pdf`
- `docs/architecture.md` §7 — resumo arquitetural derivado deste estudo
- `src/DRY/horta/nodered/funcionalidades_nodered.md` §9 — contratos com o firmware

---

## 10. O que este estudo não resolveu

Registrado explicitamente para não virar premissa falsa numa próxima sessão:

1. **O custo real de flash e RAM de `MY_OTA_FIRMWARE_FEATURE`** — não medido. É a fase 0.
2. **O tamanho e o `BOOTSZ` do `MYSBootloader` e do `DualOptiboot`** — não verificados em
   bancada. Se a seção de boot precisar passar de 2.048 B, a flash útil cai abaixo dos 30.720 B
   usados como base em todo o §3.1.
3. **Se DIO0 chega ao pino IRQ do soquete** em cada placa de nó — bloqueio da migração RFM95,
   anterior ao OTA.
4. **Os números de vazão LoRa são calculados, não medidos.** A fórmula de tempo de ar do
   SX1276 é determinística, mas a latência real de controlador não foi observada.
5. **Se o `MY_OTA_LOG_SENDER_FEATURE` já ativo implica algum caminho de stream funcionando** no
   gateway (§5.8).
6. **A ausência de `ST_FIRMWARE_*` no Node-RED** foi confirmada no backup versionado, não no
   servidor — o MCP retornou 404.
7. **A placa física do Nó 04 é 3,3V/8MHz ou 5V/16MHz?** Duas fontes documentais dizem 8 MHz e o
   `platformio.ini` diz 16 MHz (§5.10-D). Resolver antes de qualquer gravação de bootloader.
8. **O DualOptiboot cabe em 1 KB?** Se couber, devolve 1.024 B de flash de aplicação a cada nó
   (§5.10-F) — o que muda o veredito de aperto dos Nós 99 e 04.
9. **O adaptador RFM95W tem translação de nível?** O README indica que não. Se confirmado, só os
   nós de 3,3 V podem usá-lo direto (§5.10-H).
10. **Regulamentação ANATEL** para o uso de 915 MHz com RFM69/RFM95 nesta aplicação — limites de
   potência e de ocupação de canal não foram levantados. O padrão da lib é 868 MHz
   (`MyConfig.h:643,855`), que **não** é a faixa brasileira.
