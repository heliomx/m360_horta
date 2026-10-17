---
title: 'Revisão do Architecture Spine — M360 Horta (2026-10-17)'
target: ARCHITECTURE-SPINE.md
reviewed: '2026-10-17'
verdict: 'revisar antes de liberar para épicos'
counts: { critical: 0, high: 5, medium: 11, low: 7 }
---

# Revisão do Architecture Spine — M360 Horta

## Veredito

**Revisar antes de liberar para épicos.** O spine acerta o paradigma (três camadas, plataforma hexagonal, nó declarativo), ratifica corretamente a maior parte do código existente e cobre F1–F16 no mapa de capacidades. Mas dois ADs contradizem o plano de migração e a spec em pontos com efeito físico (quem publica comandos e quem é dono do MUX do Nó 99). Um AD marcado `[ADOPTED]` é falso para o gateway do Kit Hélio. O envelope operacional não tem estratégia de testes, e a caixa postal de nós que dormem sumiu do spine.

| Severidade | Quantidade |
|---|---:|
| Crítica | 0 |
| Alta | 5 |
| Média | 11 |
| Baixa | 7 |

## O que foi conferido e confere

Conferido no código em 2026-10-17. Estes itens **não** geram achado:

| Afirmação do spine | Evidência |
|---|---|
| `dispatchCommand()`, `parseTopicByte()`, `reportCommandRejected()` no gateway Horta | `src/DRY/horta/gateway/libDryGatewayMqtt.cpp:448,459,479` |
| `buildTopicOut`/`buildTopicIn` | `lib/M360-DRY/src/M360Config.cpp:213-219` (e cópia em `M360MQTT.cpp:135-146`, ver M4) |
| Métricas em `{prefixo}/gateway/status`, heartbeat em `{prefixo}/out` | `M360Config.cpp:221`, `M360MQTT.cpp:148`, `libDryGatewayMqtt.cpp:367` |
| EEPROM 512 e 521+ | `M360Config.h:14-23` (`M360_EEPROM_INTERVAL_ADDRESS 512`, `M360_EEPROM_DEVICE_BASE 521`) |
| Children 253/254/255 e `V_TEXT`/`V_VAR1`/`V_VOLTAGE` | `M360Config.h:94-96`, `M360Node.cpp:220-222` |
| `FORCE_UPDATE`, `REPRESENT`, `DEBUG_NET` por `V_CUSTOM` | `M360Constants.h:49-54`, `M360Node.cpp:340` |
| Seções de rádio `[radio_rf24]`, `[radio_rfm95]`, `[common] radio_flags`, `[proto_rf24]`, `[kit_helio]` | `src/DRY/horta/platformio.ini`, `src/DRY/kit-helio/platformio.ini:10` |
| `NODE99_BOARD_ACT` (`act_99`, `act_99_rep`) e `M360_BOARD_GW` (`d1_mini_gw`) | `src/DRY/horta/platformio.ini` |
| Lib M360-DRY 2.2.0 | `lib/M360-DRY/library.json` |
| espressif8266 2.6.3 | fixado (`platform = espressif8266@2.6.3`) e instalado em `~/.platformio/platforms/espressif8266` |
| MySensors 2.3.2, PubSubClient 2.8 | resolvidos em `%TEMP%/pio_builds/.../libdeps/*` |
| KiCad 10.0.4 | `kicad-cli version` → `10.0.4` |
| Freerouting 1.9.0 | `hardware/ARQUITETURA_FAMILIA.md:8,420-435`; `~/.kicad-mcp/freerouting.jar` com build de 2023-10-30 (o manifesto não traz versão: evidência indireta) |
| Grade D-09, fábrica k = 8 | `MY_RFM95_FREQUENCY=916800000UL` = 915,2 + 0,2·8 MHz |
| Arquivos citados (`m360-node-factory.md`, `netdiff.py`, `nr_deploy.py`, skill KiCad, spec v1.3) | todos existem |
| F1–F16 | todos aparecem no Capability → Architecture Map |

---

## Achados

### Alta

#### H1 — O agendador publica direto no broker, contra a spec e contra a serialização do MUX

- **Local:** AD-5 (Rule: "só o despachante e o agendador publicam no broker"); Structural Seed (`AG -- publish --> BR`).
- **Problema:** a spec RF-CMD-01 diz "nenhum `publish` para `{prefix}/in/...` fora do despachante". RF-CMD-08 coloca no despachante a fila por nó e o grupo de exclusão 31–37 ("nunca dois comandos em voo no grupo"). Com um segundo publicador, o OFF agendado pula retry, confirmação pelo eco (AD-3), registro com `origin` (RF-CMD-10) e a serialização do MUX. É o divergence point que o próprio AD-5 diz prevenir.
- **Correção:** "Só o despachante publica em `{prefixo}/in`. O agendador e o reconciliador criam comandos e os entregam ao despachante." No seed, trocar `AG -- publish --> BR` por `AG --> DES`.

#### H2 — AD-6 deixa dois escritores no MUX e conflita com a fase 3 da migração

- **Local:** AD-6.
- **Problema:**
  1. "A posse é um flag explícito **por grupo**" não diz o que é o grupo. Se for por canteiro (31 para o backend, 32 para o Node-RED), os dois sistemas comandam o mesmo MUX. Ligar um canal preempta o outro (inventário §7.2) e corta a rega alheia. A spec §10 já avisa: "nunca duas fontes comandando os children 31–37 ao mesmo tempo".
  2. A spec §10 entrega na **fase 3** o despachante e os botões manuais do backend (31–37), mas só desliga a aba Irrigação do Node-RED na **fase 4**. Entre as fases 3 e 4 há dois escritores no grupo `mux99`. O AD-6 proíbe isso sem dizê-lo, e o plano de migração continua prevendo.
  3. 38 e 39 (bombas NFT, 220 V) são atuadores e ficam fora do AD.
- **Correção:**
  - Definir a unidade de posse como o **grupo de concorrência** da spec §9.1: `mux99` = 31–37 inteiro; 38 e 39, cada um por si.
  - Declarar que a fase 3 exige desligar também a aba Irrigação, ou que os botões manuais do backend ficam fora do `mux99` até a fase 4. Registrar o ajuste como pendência da spec §10.
  - Indicar onde o flag vive (banco do backend e flag correspondente no Node-RED) e quem o vira.

#### H3 — AD-5 `[ADOPTED]` é falso para o gateway do Kit Hélio

- **Local:** AD-5; Consistency Conventions ("Parsing de tópico", "Erros no gateway").
- **Problema:** `src/DRY/kit-helio/gateway/libDryGatewayMqtt.cpp` não tem `dispatchCommand()`, `parseTopicByte()` nem `reportCommandRejected()`:
  - `:451-454` usa `atoi()` nos segmentos do tópico;
  - `:446`, `:457`, `:503` descartam com `Serial.println`;
  - `:492` e `:515` chamam `send(outMsg, withAck)` com `withAck = true` para `V_STATUS`, contra a regra `send(outMsg, false)`.

  O spine afirma a regra "no gateway" e a marca como adotada. D-08 prevê o Kit Hélio como segunda instalação.
- **Correção:** escopar o AD ao gateway Horta (`d1_mini_gateway`/`d1_mini_gw`) e registrar o Kit Hélio como divergência conhecida, com prazo (portar antes de D-08/E3). A alternativa é exigir o porte já e tirar o `[ADOPTED]` até lá. Vale também extrair `dispatchCommand`/`parseTopicByte` para a lib, que é a regra do CLAUDE.md ("lógica de infraestrutura em módulo dedicado em `lib/M360-DRY/`"), e assim os dois sketches deixam de divergir.

#### H4 — Estratégia de testes ausente

- **Local:** spine inteiro; o único rastro é a justificativa de D-05 em Deferred.
- **Problema:** a altitude de iniciativa é dona da estratégia de verificação, e o repositório não tem `test/` nem CI (`pio_test.ini` aponta para um sketch avulso). Os gates que impedem divergência entre unidades existem só espalhados:
  - simulador de nó obrigatório na fase 0 (spec §12.1, RNF-06);
  - teste de contrato que lê o `inventario.md` (spec §14);
  - teste de propriedade do payload `"0"/"1"` (spec §12.2-1);
  - teste de kill (RNF-01);
  - bancada das placas (SM-7);
  - `pio check`.

  Nada no spine diz quais são obrigatórios, em que camada nem quem os roda.
- **Correção:** um AD "Verificação por camada":
  - **firmware:** build de todos os `default_envs`, `pio check -e check_m360_dry`, orçamento de RAM (M10) e bancada por item do `ARQUITETURA_FAMILIA.md` §9;
  - **contrato:** teste que confronta `NODE_ITEMS[]`/`nodeConfig.h` × `inventario.md` × configuração da plataforma;
  - **backend:** simulador + broker local em CI, com os cenários da spec §12.2 como gate de fase;
  - **migração:** critérios de saída da spec §10.

  Registrar em Deferred onde a CI roda, se ainda não estiver decidido.

#### H5 — Caixa postal de nós que dormem e estados de comando incompletos

- **Local:** Consistency Conventions ("Estados de comando"); falta um AD para F9/FR-27.
- **Problema:** a convenção lista `PENDENTE, ENVIADO, CONFIRMADO, FALHOU, REJEITADO, EXPIRADO, DIVERGENTE` e omite `AGUARDANDO_DESPERTAR` e `CANCELADO`, que estão na spec §5.3. A caixa postal (RF-CMD-09: janela de 2 s, FIFO por nó, expiração em 24 h, reed abre a janela) não aparece no spine. É um ponto de divergência entre firmware (`smartSleep`, `setWakeInterrupt`), Node-RED (que hoje sobrescreve o comando) e backend. Quem ler só o spine monta uma máquina de estados sem caixa postal.
- **Correção:** completar a lista de estados com a da spec §5.3. Acrescentar uma regra: "comando a nó `LOW_POWER`/`PASSIVE` nunca é interativo; fica em fila FIFO por nó na plataforma e só é publicado na janela de despertar; o gateway não enfileira." O AD-8 já implica a última parte; deixar explícito.

### Média

#### M1 — Status invertido: AD-14 adotado sem implementação, AD-15 implementado sem status

- **Local:** AD-14, AD-15, AD-9.
- **Problema:** AD-14 está `[ADOPTED]`, mas o canal é hoje uma macro fixa (`[radio_rfm95] MY_RFM95_FREQUENCY=916800000UL`), e `M360DeviceConfig` (`M360Config.h:43-44`) não tem campo de canal. O mecanismo da D-09 (índice gravado na EEPROM e lido no `before()`) colide com AD-9 ("Macros `MY_*` só no `platformio.ini`") e com AD-15 ("escolhido na compilação"), e nada concilia os três. Já o AD-15 descreve o que o `platformio.ini` faz hoje e não está marcado como adotado.
- **Correção:**
  - marcar AD-15 `[ADOPTED]`;
  - marcar AD-14 como decidido e não implementado (ou `[PROPOSED]`);
  - escrever a exceção: o tipo de rádio é de compilação e a frequência vira variável lida da EEPROM, com faixa definida por AD-12 antes do código.

#### M2 — Isolamento de rádio ignora as redes nRF24

- **Local:** AD-14, D-08.
- **Problema:** os protótipos da Horta e o Kit Hélio usam nRF24 no canal 76 com o mesmo endereçamento-base. Se o Kit Hélio virar segunda instalação (D-08) e estiver no alcance de outra rede nRF24, nada isola as duas. AD-14 só fala da grade LoRa.
- **Correção:** acrescentar ao AD-14: "Duas instalações nRF24 no alcance mútuo são proibidas. Se forem inevitáveis, separar por `MY_RF24_CHANNEL`/`MY_RF24_BASE_RADIO_ID` por instalação, com medição (ver CLAUDE.md, canal 76)."

#### M3 — Identidade da instalação sem namespace definido

- **Local:** AD-13.
- **Problema:** a identidade é o par UF+CAR, mas o firmware guarda `uf[4]` e `carNumber[8]` (≤ 7 caracteres, `M360Config.h:43-44`). Um CAR real tem dezenas de caracteres, então o segmento `{CAR}` é um código curto que alguém escolhe. O spine não diz quem atribui, qual o formato nem como se garante unicidade. Duas instalações com `0001` na mesma UF colidem no broker. FR-54 (ACL por prefixo) depende disso.
- **Correção:** definir o formato (por exemplo `[A-Za-z0-9]{1,7}`, atribuído e garantido único pela plataforma no comissionamento) e o mapeamento para o CAR real como atributo da instalação. A alternativa é ampliar o campo com nova faixa de EEPROM (AD-12).

#### M4 — "Montado por função" já tem duas implementações e concatenações nos sketches

- **Local:** AD-4.
- **Problema:**
  - `buildTopicOut`/`buildTopicIn`/status existem como funções livres (`M360Config.cpp:213-223`) **e** como métodos de `MQTTManager` (`M360MQTT.cpp:135-151`), com corpos duplicados;
  - os sketches concatenam `"/events"` (`libDryGatewayMqtt.cpp:613`) e os segmentos do ACK de transporte (`:417-422`) à mão.

  É o tipo de deriva que o AD diz prevenir.
- **Correção:** a regra deve exigir **uma** implementação por tópico na lib (`buildTopicEvents`, `buildNativeTopic(prefix, node, child, cmd, ack, type)`) e remover a duplicata de `MQTTManager`. No backend, a "função equivalente" deve ser um módulo único, testado contra a mesma tabela da spec §2.2.

#### M5 — AD-4 omite o modo envelope JSON e os limites de validação

- **Local:** AD-4.
- **Problema:** a spec §2.2 obriga o backend a **aceitar** o envelope JSON (modo alternativo, `M360_NATIVE_MQTT` desligado). O AD-4 descreve só o nativo, e um consumidor pode ignorar o envelope. Também faltam o payload ≤ 25 bytes e as exatamente 8 barras, que são o contrato de `Translator::validate()`.
- **Correção:** "O produtor publica só o nativo; o consumidor aceita nativo e envelope." Incluir os limites de validação ou apontar para a spec §2.3.

#### M6 — O contrato do AD-1 não inclui o failsafe e não é verificável

- **Local:** AD-1, AD-2.
- **Problema:** AD-2 faz a plataforma limitar cada rega a `failsafe − 15 s`, mas o AD-1 não lista `failsafe_s` nem a pertença ao grupo `mux99` entre os campos do contrato. A spec §14 lista. Mudar o failsafe no firmware sem mudar a plataforma recria a corrida que a spec §7.3 aponta. Além disso, a regra é só de processo ("na mesma entrega"), sem verificação.
- **Correção:** acrescentar ao AD-1 failsafe, grupo de concorrência e lista de atuadores. Tornar a regra verificável pelo teste de contrato de H4.

#### M7 — AD-12 descreve o mapa de EEPROM com imprecisões

- **Local:** AD-12.
- **Problema:**
  - os nós escrevem 512–515 por `M360::saveInterval()` (`M360Power.h:50`), não por `saveConfig()`/`M360Config`;
  - 516–520 está explicitamente "reservado para expansão futura" (`M360Config.h:15`), e o AD manda criar "faixa nova", ignorando a reserva (5 bytes cabem um índice de canal);
  - a área do nó usa magic (`0x36D1`), não CRC.
- **Correção:** "512–515 intervalo do nó (magic + valor, por `saveInterval()`); 516–520 reservado; 521+ `M360DeviceConfig` com CRC, por `saveConfig()`." Decidir se o canal e o perfil usam 516–520.

#### M8 — Envelope operacional incompleto: firmware, broker e observabilidade

- **Local:** Deferred; Stack.
- **Problema:** o backend tem D-05 adiado com prazo, o que é aceitável. Não há decisão nem adiamento para:
  1. **Versão e implantação de firmware.** Os sketches reportam `"2.0.0"`/`"2.1.0"` à mão e a lib está em 2.2.0. Não há regra de bump, nem registro de que binário está em cada nó, nem rollback (regravar o env do protótipo).
  2. **Broker.** Produto, versão, quem opera, ACL por prefixo (FR-54) e TLS (RNF-08) não aparecem no Stack nem nos ADs, embora o broker seja o eixo do hub-and-spoke.
  3. **Observabilidade.** Log JSON, `/metrics` e `/api/health` (RNF-05) ficam só na spec.
- **Correção:** acrescentar uma seção curta "Operação", ou linhas em Deferred com gatilho: versão de firmware (antes da E1 em campo), broker e ACL (antes da E3), observabilidade (fase 0).

#### M9 — Stack com versões não fixadas e itens faltando

- **Local:** Stack.
- **Problema:**
  - **atmelavr 5.2.0** é só a versão instalada. Todo env AVR declara `platform = atmelavr` sem versão (`src/DRY/horta/platformio.ini`, `check_m360_dry`, `lp_base`, `act_99`…), e um `pio pkg update` muda o compilador sem aviso.
  - **Faixas `^` sem lockfile:** ArduinoJson `^6.21.3` hoje resolve **6.21.6**.
  - **Libs de sensor ausentes:** DHT `^1.4.6`, Adafruit Unified Sensor `^1.1.14`, DallasTemperature `^3.9.1` (resolve 3.11.0) e OneWire `^2.3.7` (resolve 2.3.8).
  - **Ferramentas ausentes:** cppcheck (`tool-cppcheck`), Python (`nr_deploy.py`, D-01) e Java 17 (Freerouting).
  - **Versões desconhecidas:** Node-RED e broker.
  - Um `espressif32@7.0.1` também está instalado, sem uso no projeto (sem efeito, só registro).
- **Correção:** fixar `platform = atmelavr@5.2.0` e versões exatas das libs (ou registrar que as faixas são intencionais). Completar a tabela e marcar como "não verificado" o que não tiver evidência no repositório.

#### M10 — Falta um orçamento de RAM como invariante

- **Local:** AD-10; PRD §8.2.
- **Problema:** o PRD chama o Nó 99 de caso crítico de RAM, e a memória do projeto registra 103 B de pilha com assinatura. AD-10 exige só `pio check` para mudar a lib. Uma mudança na lib pode quebrar `act_99` ou `nano_99reles` sem nenhum gate.
- **Correção:** acrescentar a regra: "Mudança na lib compila todos os `default_envs` e confere RAM livre ≥ X B em `act_99`/`nano_99reles`, medida no build ou no `NODE99_HEALTH_LOG`."

#### M11 — F14 sem decisão que o governe

- **Local:** Capability Map (F14 → AD-16, AD-13).
- **Problema:** F14 é nova no PRD e não está na spec. Volume por canteiro depende de um único medidor (child 21) e da exclusão do MUX para atribuir litros à rega ativa, e as peristálticas estão no mesmo MUX. O modelo de ciclo e massa colhida (FR-49) não tem lugar normativo. AD-16 e AD-13 não governam nada disso.
- **Correção:** um AD ou uma linha em Deferred, a decidir antes da fase 5: a fonte de verdade do volume por canteiro é `irrigation_run` + contador do child 21, atribuído só com um canal do `mux99` ativo. Dizer também onde vivem o ciclo e a colheita.

### Baixa

#### L1 — AD-3: broadcast e sujeito ambíguo

- **Local:** AD-3.
- **Problema:** "ou qualquer child, se o alvo for 255" não tem correspondente na spec RF-CMD-02/04, que não aceita `atuar` em 255. A última frase ("Não grava estado de atuador...") não deixa claro o sujeito.
- **Correção:** "O ACK de transporte não grava estado..." e remover ou justificar o caso 255.

#### L2 — AD-11 não fixa o child dos comandos `V_CUSTOM`

- **Local:** AD-11.
- **Problema:** a spec usa child 0. O firmware aceita qualquer child (`M360Node.cpp:340`).
- **Correção:** fixar "child 0".

#### L3 — AD-2 não diz que 38/39 não têm failsafe

- **Local:** AD-2.
- **Problema:** as bombas NFT (220 V) são "sem limite" por projeto (inventário §7.3). A plataforma não pode tratar o failsafe como última barreira para elas.
- **Correção:** declarar a exceção.

#### L4 — Convenções de nó: idioma misto e faixas de nodeId desatualizadas

- **Local:** Consistency Conventions.
- **Problema:**
  - `EXPIRED` destoa da convenção em português (vem da spec RF-REG-11);
  - `M360Constants.h:18-26` define faixas de nodeId (1–50 clima, 51–150 solo, 151–200 atuador) que os nós reais (1 e 2 solo, 99 atuador) violam;
  - o spine não diz como se alocam IDs por instalação até F16.
- **Correção:** padronizar `EXPIRADO`, ou registrar a exceção. Declarar as faixas de `M360Constants.h` como legado e dizer que o ID vem do env por instalação.

#### L5 — MySensors em faixa `^2.3.2` com comportamento dependente da versão

- **Local:** Stack.
- **Problema:** o comportamento citado como normativo depende da 2.3.2: o broadcast não reapresenta (spec §2.3) e `getBool()` faz `atoi()`.
- **Correção:** fixar `@2.3.2` exato.

#### L6 — `docs/architecture.md` sem aviso de superado

- **Local:** frontmatter `supersedes`.
- **Problema:** o documento antigo ainda descreve nRF24 apenas, RS485/Modbus e gateway ESP32, e não traz aviso de superado.
- **Correção:** acrescentar um banner de superado no arquivo, como pendência fora do spine.

#### L7 — Stack: "RFM95W 915 MHz" impreciso

- **Local:** Stack.
- **Problema:** a configuração real é 916,8 MHz (k = 8), BW125 CR4/5 SF7 e 17 dBm (`[radio_rfm95]`). O enquadramento ANATEL (Q-7) pode mudar BW e grade.
- **Correção:** registrar a configuração e a dependência de Q-7.

---

## Checklist do bom spine

| Critério | Situação |
|---|---|
| Fixa os pontos de divergência reais e não perde nenhum | Parcial: faltam caixa postal (H5), dono único da publicação (H1), posse do MUX (H2), orçamento de RAM (M10), namespace do CAR (M3), nRF24 (M2) |
| Cada Rule é aplicável e previne a divergência declarada | Parcial: AD-5 se contradiz (H1), AD-6 é ambíguo (H2), AD-1 sem verificação (M6), AD-4 já violado (M4) |
| Nada em Deferred permite que duas unidades divirjam | OK para os itens listados; os problemas são o que falta (M8, M11) |
| Ratifica o brownfield | Na maior parte, sim; exceções: H3, M1, M7 |
| Cobre F1–F16 | Sim no mapa; F14 sem decisão real (M11) |
| Envelope operacional decidido, adiado ou aberto | Implantação do backend adiada com prazo; testes ausentes (H4); firmware, broker e observabilidade sem registro (M8) |
| Stack com evidência | KiCad, Freerouting, espressif8266, MySensors, PubSubClient, lib 2.2.0 conferem; atmelavr não fixado, faixas sem lock e itens faltando (M9, L5, L7) |
