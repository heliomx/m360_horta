# Extração de fontes — firmware e hardware de campo (M360 Horta)

**Data:** 2026-10-17 · **Branch lida:** `refactor/m360-dry-ram` (+ spine em `feat/firmware-generico-autoconfig`)
**Uso:** insumo para o PRD. Não é requisito aprovado; é o que as fontes dizem hoje.

Abreviações de fonte:
- **AF** = `hardware/ARQUITETURA_FAMILIA.md`
- **INV** = `src/DRY/horta/inventario.md`
- **CL** = `CLAUDE.md` (raiz)
- **DRY** = `src/DRY/README.md`
- **LP** = `src/DRY/horta/nos/lp/README.md`
- **N99** = `src/DRY/horta/nos/99nodeReles/README.md`
- **GWR** = `src/DRY/horta/gateway/README.md`
- **API** = `lib/M360-DRY/API_REFERENCE.md`
- **SPINE** = `feat/firmware-generico-autoconfig:_bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-09-08/ARCHITECTURE-SPINE.md`
- **BK** = `docs/especificacao-backend-m360.md` (consultado só para D-09/§15.6 e §2.4)
- **NF** = `.agent/workflows/m360-node-factory.md` (Passo 0)
- **MEM** = memória do projeto (notas `decisao-gateway-por-propriedade`, `assinatura-freq-variavel-medido`, `radio-lora-rfm95w-915`)

Legenda de estado:
- **PROD**: em produção no protótipo (Nano / Pro Mini 16 MHz / D1 mini, nRF24)
- **NOVA-ST**: implementado no firmware das placas M360, compila, **sem teste de bancada**
- **PEND**: não implementado

---

## 1. Capacidades do sistema de campo (FEATURES)

### F-01 Monitoramento de umidade de solo multiponto
- O nó lê 6 sondas resistivas por canteiro (3 distâncias × 2 profundidades). Publica o ADC bruto 0–1023, em que valor alto significa seco. O Node-RED remapeia `V_LEVEL` para percentual.
- A M360-LP tem 8 canais pelo 74HC4051 (6 em uso, 2 de reserva). Os sensores só recebem energia durante a varredura.
- Também aceita sonda capacitiva: basta não montar (DNI) o pull-up.
- **Estado:** PROD (Nós 1 e 2 no Nano). NOVA-ST na LP (`lp_01`/`lp_02`).
- **Fonte:** INV §4–§5, §9; LP "Pinagem"; AF §4.2.

### F-02 Monitoramento de clima (ar e solo)
- Mede temperatura e umidade do ar (DHT11) e temperatura do solo (DS18B20) e publica em °C/% com 1 casa decimal.
- O Nó 4 é a **fonte de clima** do motor de regras, do Radar M360 e do `/status` do Telegram.
- A LP tem drivers DHT e 1-Wire ativados por flag. I2C está previsto na placa, mas ainda não há sensor escolhido.
- **Estado:** PROD (Nó 4; Nó 99 no Nano, que publica só para exibição). Na LP, os drivers DHT e 1-Wire existem, mas nenhum nó LP os usa.
- **Fonte:** INV §6, §8; LP "Arquivos" e "I2C"; AF §4.2.

### F-03 Vazão e volume de irrigação
- O sensor YF-S201 é lido por interrupção e publicado em L/min (450 pulsos/L).
- O volume acumulado é calculado no Node-RED.
- A M360-ACT condiciona o sinal para aceitar as duas variantes do sensor (push-pull 5 V e coletor aberto).
- **Estado:** PROD (Nano). NOVA-ST na ACT.
- **Fonte:** INV §7.1, §9; AF §5.2 "Vazão".

### F-04 Atuação de relés com failsafe e exclusão mútua do MUX
- São 9 atuadores: 7 pelo MUX (3 solenóides e 4 peristálticas), com **um ativo por vez**, e 2 nativos (bombas NFT), que são livres.
- Na preempção, o nó ecoa `V_STATUS=0` do canal desligado antes do eco do canal comandado.
- O failsafe por tempo máximo ligado fica **no nó**: 600 s para solenóides, 120 s para peristálticas e sem limite para as bombas NFT. Ele sobrevive à queda de WiFi, MQTT, Node-RED e gateway.
- Os relés são ativos em LOW e ligados em NA: em reset ou falha, a carga fica desligada. Na ACT, o /OE do buffer garante que nenhum relé bata no boot.
- **Estado:** PROD (Nano). NOVA-ST na ACT.
- **Fonte:** INV §7.2–§7.3; N99 §3.5, §4, §5.2–§5.2.1; AF §5.2 "/OE".

### F-05 Gestão de energia: fonte fixa × bateria/solar
- Há 4 perfis: `ALWAYS_ON` (fonte fixa, nunca dorme), `LOW_POWER` (`smartSleep`), `PASSIVE` (lê só sob comando) e `REPEATER`.
- Os nós a bateria fazem warm-up pós-boot de 10 ciclos × 30 s. A bateria é reportada no child 255, com 3,0 V = 0 % e 4,2 V = 100 %.
- Na LP, a bateria é lida pelo A6 com correção pelo bandgap, e o TX é cortado abaixo de `LP_VBAT_CUTOFF`. A LP aceita as duas alimentações: 5 V fixo ou módulo solar TP4056+DW01.
- **Estado:** PROD (Nó 4 em `LOW_POWER`; Nós 1, 2 e 99 em `ALWAYS_ON`). Na LP o modo bateria está NOVA-ST e **nenhum nó LP o usa hoje**.
- **Fonte:** DRY "Perfis de energia"; API §3–§4; AF §4.1; LP "Reed switch".

### F-06 Intervalo remoto e comandos administrativos
- O child 254 recebe e persiste o intervalo em minutos (eco de confirmação).
- Comandos `V_CUSTOM`: `FORCE_UPDATE`, `REPRESENT` e `DEBUG_NET`, este com resposta no child 253 (debug remoto, até 24 caracteres).
- O gateway valida cada comando (tópico, faixas, payload de `V_STATUS` só `"0"`/`"1"`) e publica `command_rejected` ou `command_send_failed`.
- **Estado:** PROD.
- **Fonte:** API §2 "Métodos" e "Comandos V_CUSTOM"; INV §3, §8.1; CL "Armadilhas".

### F-07 Reed switch (ímã, sem abrir a caixa)
- **LP:** o ímã pela parede força a leitura e o envio de todos os sensores e da bateria. Em `LOW_POWER`, acorda o nó por INT1. Abaixo do corte de bateria, não tem efeito.
- **ACT:** o ímã mantido 3 s faz o repareamento sem reboot (busca de pai, reapresentação, reenvio de vazão e bateria). O estado dos relés **não** é reenviado.
- **GW:** com o ímã no boot, sobe em modo AP. Afastar e manter 3 s nos primeiros 30 s do `loop()` faz o factory reset: publica o evento, apaga a configuração e sobe em AP. Ímã mantido desde o boot não apaga nada.
- **Portal:** o botão "Restaurar padrões de fábrica" (`POST /reset`, com confirmação) tem o mesmo efeito.
- **Estado:** NOVA-ST nas três (firmware de 2026-10-17, M360-DRY 2.2.0). O factory reset vale também para os gateways protótipos (botão no A0).
- **Fonte:** AF §2 dec. 13, §5.2 "Reed", §6.1 "Reed", §9 "Factory reset"; LP "Reed switch"; GWR "Modo AP e factory reset"; API `forceUpdate`/`setWakeInterrupt`/`requestRejoin`.

### F-08 Portal de configuração do gateway
- O modo AP `M360-Config` (192.168.4.1) aparece com configuração inválida ou com o A0 em GND no boot.
- O portal tem as rotas `/`, `/save` e `/reset`. Grava SSID, senha, broker, usuário, UF e CAR na EEPROM 521+ com CRC.
- **Não tem autenticação**: em modo STA, qualquer um na LAN pode reconfigurar ou apagar o gateway.
- **Estado:** PROD (protótipo). Na M360-GW, NOVA-ST.
- **Fonte:** GWR "Módulos", "EEPROM", "Modo AP e factory reset"; CL "Arquitetura do Gateway".

### F-09 Telemetria do gateway e registro de nós
- Ponte MySensors ↔ MQTT em modo nativo (campos no tópico, valor bruto no payload).
- Heartbeat a cada 60 s com uptime, RSSI, IP e versão.
- Eventos `node_discovered`, `node_lost` (15 min sem mensagem) e `node_reconnected`.
- O ACK de transporte prova só o próximo salto; a confirmação real é o eco do nó.
- **Estado:** PROD.
- **Fonte:** GWR "MQTT"; DRY "Gateway"; CL "Armadilhas".

### F-10 LEDs de status do gateway
- Verde = WiFi + MQTT ok; amarelo piscando = sem MQTT; vermelho piscando = sem WiFi.
- Na M360-GW os LEDs ficam no painel da caixa (chicote JST de 6 vias), com o verde via 2N7002 no D4.
- O verde aceso durante reset ou gravação é esperado.
- **Estado:** PROD (protótipo). NOVA-ST na GW.
- **Fonte:** CL "LEDs"; AF §6, §6.1; GWR "Hardware e envs".

### F-11 Rádio selecionável RFM95W 915 MHz / nRF24L01+
- As placas M360 usam o mezanino RFM95W (SMA de borda) **ou** o nRF24 pelo header 1×8 legado. É um rádio por placa.
- A escolha é feita em compilação (`[common] radio_flags`). Hoje o RFM95W está ativo: 916,8 MHz, BW125, SF7, 17 dBm.
- Protótipos e Kit Hélio ficam fixos em nRF24, canal 76 validado.
- O Nó 99 pode atuar como repetidor (envs `_rep`).
- **Estado:** nRF24 em PROD. RFM95W NOVA-ST: banda ainda não validada em bancada.
- **Fonte:** AF §1, §2 dec. 2 e 15, §3; DRY "Rádio"; INV §1 nota.

### F-12 Firmware por placa, configuração por nó
- A LP tem um fonte único (`lpNode.cpp` + drivers por flag). Cada nó é só um `nodeConfig.h` + um env.
- Na ACT e na GW, o mesmo fonte do protótipo atende a placa nova por flag (`NODE99_BOARD_ACT`, `M360_BOARD_GW`).
- Criar um nó LP novo não exige código C++.
- **Estado:** NOVA-ST (LP, ACT e GW verificadas contra a netlist em 2026-10-14).
- **Fonte:** LP intro e "Como criar"; NF Passo 0; DRY "Placas e envs".

### F-13 Família de placas M360 (hardware)
- São quatro placas: RF (mezanino), LP (sensor), ACT (atuador) e GW (gateway). A montagem é PCBA JLCPCB, e só módulos e conectores são soldados à mão.
- Portas RJ45 com pinagem T568B e TVS em todas as linhas de campo.
- Os arquivos de fabricação estão gerados; **nenhuma placa foi montada ou testada**.
- **Estado:** projeto concluído (ERC/DRC 0), PEND fabricação e bancada.
- **Fonte:** AF status, §1, §7, §8, §10.

**Total: 13 features.**

---

## 2. Inventário atual de nós

| ID | Função | Placa protótipo (em campo) | Placa M360 | Perfil | Children |
|---:|---|---|---|---|---|
| 0 | Gateway MySensors ↔ MQTT | D1 mini (`d1_mini_gateway`) | M360-GW (`d1_mini_gw`) | — | — |
| 1 | Solo, Canteiro A | Nano / Pro Mini 16 MHz | M360-LP (`lp_01`) | ALWAYS_ON (5 V) | 1–6 |
| 2 | Solo, Canteiro B (motor de regras) | Nano | M360-LP (`lp_02`) | ALWAYS_ON (5 V) | 1–6 |
| 4 | Clima + temperatura de solo, solar; **fonte de clima** | Pro Mini 5 V/16 MHz | **nenhuma** (fica no protótipo) | LOW_POWER | 1, 11, 12 |
| 99 | Atuação da estufa (9 relés) + vazão | Nano (`nano_99reles[_rep]`) | M360-ACT (`act_99[_rep]`) | ALWAYS_ON / REPEATER | Nano: 11, 12, 21, 31–39 · ACT: 21, 31–39 |
| 11 (Kit Hélio) | DHT11, rede separada nRF24 | Pro Mini 16 MHz | nenhuma | ALWAYS_ON | 11, 12 |

- Reservados em todo nó: 253 (debug), 254 (intervalo) e 255 (bateria).
- Os nós 5, 13 e 80 foram removidos.
- **Fonte:** INV §1, §3, Apêndices A e B; AF §1.

---

## 3. Restrições de hardware e ambiente (candidatas a NFR)

**RAM e Flash (ATmega328P: 2 KB SRAM, 32 KB Flash)**
- Medido: `lp_01` ocupa 20 502 B de Flash e 657 B de RAM (Nano: 20 556 / 662). Fonte: LP "Arquivos".
- Medido com a M360-DRY 2.0.0, RF24, sem assinatura: Nó 99 caiu de 1501 para 791 B; Nós 01/02 de 1018 para 590 B; Nó 04 de 894 para 589 B. Fonte: MEM `assinatura-freq-variavel-medido`.
- A assinatura soft MySensors custa +372 B de RAM e +4,6 KB de Flash. No Nó 99 antigo restavam só 103 B de pilha, então **não cabia**. Com os cortes da 2.0.0, a prova fora do repo deu 813 B livres, mas nada rodou em hardware. Fonte: MEM; BK D-09.
- Frequência LoRa em variável custa +4 B de RAM. Fonte: MEM; BK D-09.

**Energia**
- **Célula 18650 e módulo de carga:** a LP aceita 3,0–6 V em `J_VIN`. Exige módulo TP4056 **com** DW01+8205 (6 terminais); o módulo de 4 terminais e o shield com boost não servem. Fonte: AF §4.1.
- **Bateria no VCC:** proibida nas placas novas, porque o SX1276 tem máximo absoluto de 3,9 V. O Nó 4 protótipo roda direto na célula. Fonte: AF §4.1; INV §6.
- **BOD e limiar de corte:** o BOD é de 2,7 V (efuse 0xFD, a confirmar por ISP). O limiar de corte de TX deve ser **≥ 3,0 V** com a correção pelo bandgap (≥ 3,4 V sem ela). Abaixo de ~2,85 V, o pico de TX provoca laço de reset. Fonte: AF §9 "Firmware das placas novas", "Fuses".
- **Pico de TX:** o RFM95W puxa 87–120 mA. O mezanino tem um reservatório de 22 µF. Fonte: AF §3.
- **Trilho de 3,3 V da GW:** ESP + LoRa somam ~290 mA nos datasheets e encostam nos 500 mA do regulador com os picos de WiFi. Fonte: AF §9.
- **Corrente de sono da LP:** não medida. O regulador do Pro Mini pode ser realimentado. Fonte: AF §9.

**Rádio**
- RFM95W 915 MHz. O padrão da lib (868,1 MHz) é ilegal no Brasil. Fonte: MEM `radio-lora-rfm95w-915`.
- **ANATEL:** o enquadramento de BW125 em canal fixo × BW500 ainda precisa ser **confirmado com laboratório**. A fonte primária não foi acessada (HTTP 403). Fonte: spine-branch `analise_radio_rfm95w_915.md` §4 R-2.
- **Grade D-09:** 64 canais de 200 kHz, `f = 915,2 + 0,2·k` MHz, com canal de fábrica k = 8 (916,8 MHz). A banda B (902–907,5 MHz) fica de reserva, e o reuso de canal só é permitido a mais de 10 km. Fonte: BK §15.6, D-09.
- **Alcance:** **não medido**. Na ACT, a antena fica dentro de um quadro com metal ao redor, e o alcance deve ser medido na instalação. O repetidor (Nó 99 `_rep`) cobre propriedades maiores que o alcance. Fonte: AF §5.2 "Antena"; spine-branch análise §5.
- **Lição do nRF24:** um MAX_RT no gateway acusa o transmissor do próprio gateway. Fonte: CL "Armadilhas".

**Robustez elétrica**
- **TVS:** SRV05-4 em todas as linhas de campo, com 100 Ω ou 1 kΩ em série até o MCU. Fonte: AF §4.2, §5.2, §7.
- **Inversão de polaridade:** P-MOSFET AO3401A na LP e na ACT; Schottky SS14 na GW. Fonte: AF §4.1, §5.1, §6.1.
- **Sobretensão no 5 V:** PTC 500 mA + SMAJ5.0A na ACT e na GW. A fonte do quadro deve ser ajustada e **travada em 5,0 V** (máximo 5,1 V). Numa sobretensão lenta entre 6,5 e 8 V, o D1 mini fica alguns segundos acima do limite. Fonte: AF §5.1, §6.1.
- **Relés e cargas indutivas:** relés em NA, de modo que falha deixe a carga desligada. Snubber ou MOV obrigatório nos contatos de carga indutiva. Fonte: N99 §4, §6.
- **Isolamento de tensões:** nem 12 V nem 220 V tocam a PCB da ACT. Fonte: AF §5.1.

**Caixa, IP e temperatura**
- **Caixa da LP:** 100 × 100 × 50 mm, placa de 86 × 62 mm. O FTDI não cabe com a placa montada, então a gravação é feita na bancada. Fonte: AF §9 "Placa da LP", "Gravação".
- **Grau IP e posição do SMA: PEND.** Fonte: AF §9 "Caixa".
- **Distância do ímã:** o reed atua a ~6,5 mm, o que deixa a montagem justa na LP. Fonte: AF §9.
- **Temperatura de operação:** **não especificada** em nenhuma fonte. Só há menção à deriva do divisor de bateria e da fuga superficial com umidade. Fonte: AF §4.1, §9 achado 7.

---

## 4. Decisões de projeto vigentes (uma linha cada)

1. MCU Pro Mini 3,3 V / 8 MHz soldado; LED de power removido — AF §2 #1
2. Rádio em mezanino comum (header 2×5, chaveado por pino cortado) — AF §2 #2, §3
3. Atuador com MCU a 3,3 V e 74HCT244 para os sinais de 5 V — AF §2 #3
4. Carga solar por módulo TP4056+DW01 externo; a LP não tem carregador — AF §2 #4
5. 74HC4051 na LP: 8 canais de solo num pino — AF §2 #5
6. RJ45 dentro da caixa, par marrom = VCC/GND, TVS em todas as linhas — AF §2 #6
7. Nós 01/02 em fonte fixa na mesma LP — AF §2 #7
8. Placas novas a 8 MHz, com envs próprios; os envs antigos seguem servindo o hardware em campo — AF §2 #8
9. SMA de borda no mezanino; placa de 1,6 mm obrigatória — AF §2 #9, §3
10. RS485 fora do escopo — AF §2 #10
11. KiCad 10 do início ao fim; roteamento pelo Freerouting — AF §2 #11
12. PCBA nas quatro placas; só módulos e conectores soldados à mão — AF §2 #12
13. Reed switch nas três placas carrier, acionado por ímã — AF §2 #13
14. GW em SMD montada pela JLCPCB — AF §2 #14
15. nRF24 legado por header 1×8; um rádio por placa — AF §2 #15
16. Frequência LoRa provisionada por software na instalação, sem chave DIP — AF §2 (nota)
17. **Um gateway por propriedade (UF+CAR)**; Multi-CAR descartado em 2026-09-10 — MEM `decisao-gateway-por-propriedade`; AF §2 (nota)
18. **Firmware por placa, configuração por nó** (2026-10-14); nem `.cpp` por nó, nem binário genérico — NF Passo 0; LP intro
19. **Fonte de clima = Nó 4** (2026-10-17); a ACT não tem DHT — INV §6, §7, §8; AF §9
20. A ACT não tem DHT, pH nem O2; o único sensor é a vazão — AF §5.2 "Fora do escopo"; INV §7
21. O Nó 4 continua no Pro Mini 5 V/16 MHz e não migra para a LP — AF §1
22. Canal nRF24 padrão 76 validado; não trocar sem medição — CL "Canal de rádio"
23. Separação de rádio entre propriedades por frequência (grade D-09), não por sync word nem assinatura — BK D-09

---

## 5. O firmware genérico do spine (AD-01..AD-07)

**O que o spine propunha (SPINE, 2026-09-08):**

| AD | Proposta | Situação hoje | Por quê |
|---|---|---|---|
| AD-01 | Binário único `genericSensorNode`; sensores escolhidos por máscara de perfil na EEPROM | **Superado** por "firmware por placa, config por nó" (NF Passo 0, 2026-10-14) | Todos os drivers linkados sempre, rótulos fora da Flash e dependência do pareamento. O monolito com assinatura não cabia na RAM (MEM: ≈1922 B estimados) |
| AD-02 | Backend define o `nodeId` e o perfil de cada slot por UF+CAR | **Futuro**. Hoje o ID vem de `MY_NODE_ID` por env (BK §15.2) | Depende do pareamento e do backend, que ainda não existe (o Node-RED está em uso) |
| AD-03 | Janela de pareamento de 60 s no gateway para nós virgens | **Futuro, não implementado.** O `requestRejoin()` da ACT é repareamento de ID fixo, "não atribui ID nem troca frequência" (API; AF §5.2) | — |
| AD-04 | Sincronia gateway ↔ backend por `m360/<UF>/<CAR>/config` (QoS 1, retained) | **Futuro**. Esse tópico não existe no gateway (GWR "MQTT") | — |
| AD-05 | Reset remoto da EEPROM do nó via child 254 `CMD_RESET_EEPROM` | **Futuro, e conflita com o contrato atual**: o child 254 é `V_VAR1` (intervalo, INV §3) | Teria de ir para `V_CUSTOM` em outro child |
| AD-06 | Isolamento por `tenantId` de 16 bits (hash UF+CAR) em toda mensagem | **Superado em parte** pela decisão de um gateway por propriedade + frequência D-09 | A MEM mostra que o driver dá ACK antes da aplicação: filtro por aplicação **não** isola. `tenantId` ≥ 32 bits fica só como guarda residual |
| AD-07 | Canal RF24 76 único para todos | **Superado** pelo RFM95W 915 MHz + grade de 64 canais por propriedade (BK §15.6) | O LoRa alcança quilômetros: vizinhos se ouvem |

Outras divergências do spine:
- Convenção de tópico com `<type>/<ack>/<payload>`. O real é `<command>/<ack>/<type>`, com payload no corpo (GWR).
- Área de perfil na EEPROM em 514–520. Hoje 512–520 é `M360NodeConfig` (reservado).
- Stack citando só nRF24.

**O que sobra como capacidade FUTURA** (fontes: SPINE; AF §5.2; API `requestRejoin`; BK §15.2, §15.6):
- **Pareamento guiado** de nó novo, com janela no gateway, acionado pelo portal ou por MQTT.
- **ID e perfil atribuídos pelo backend** por slot UF+CAR.
- **Perfil do nó na EEPROM** (`M360NodeProfile` de 7 bytes). Hoje o perfil é fixado em compilação.
- **Reset remoto do nó** para realocar sem abrir a caixa nem regravar.
- **Isolamento por propriedade**, já redesenhado como: campo `canal` (0..63) no portal + `radioChannel` no heartbeat + comando `SET_RADIO:<k>` nos nós com eco antes de trocar + canal de fábrica k = 8 em todos os binários (BK §15.6). **Nada disso está implementado.**
- Firmware genérico de atuador com MUX (deferido no próprio SPINE).

---

## 6. Pendências reais

**Teste de bancada** (nada foi montado; AF status e §9):
1. **LP:**
   - VSW, MUX e leitura de solo.
   - Bateria pelo A6 com correção pelo bandgap e corte de TX.
   - Reed: leitura forçada e despertar, incluindo a **distância real do ímã** pela parede.
   - Corrente de sono (regulador do Pro Mini).
   - Fuses e BOD via ISP.
2. **ACT:**
   - CE no D2 com o nRF24.
   - /OE com a placa montada (nenhum relé no boot).
   - Repareamento por reed com o gateway.
   - Vazão com as duas variantes do YF-S201.
3. **GW:**
   - LED verde via Q1.
   - CE no D1 com o nRF24.
   - Factory reset por ímã através da parede.
   - Queda do 3,3 V com WiFi e LoRa transmitindo juntos.
4. **RF:** validar a banda do RFM95W (RTL-SDR em 916,8 MHz) antes de comprar em quantidade.
5. **Pino 1 do RJ45:** medir a continuidade antes de crimpar os chicotes. Um espelhamento poria VCC/GND nas linhas de sinal.
6. **Furos A4/A5/A6/A7 do Pro Mini:** medidos só por foto; confirmar com paquímetro antes de fabricar.
7. **Pedido JLCPCB:** conferir rotações no visualizador e o estoque do 10 kΩ C25804.

**Botões e funções faltantes:**
- Pareamento, ID pelo backend e reset remoto de nó (§5).
- Canal de rádio por propriedade: portal, heartbeat e `SET_RADIO` (BK §15.6).
- Validação de UF/CAR no portal (BK §15.2).
- **Autenticação do portal**: inexistente (GWR).
- Atualização de firmware em campo: depende de retirar a placa (AF §9 "Gravação").
- Nenhum nó usa a LP a bateria nem DHT ou 1-Wire na LP. O Nó 4 não tem placa M360.
- Os campos `reportIntervalMin` e `readSamples` existem na struct, mas a lib não os lê (INV §2).

**Riscos conhecidos (AF §9):**
- Enquadramento ANATEL do BW125 fixo (pode forçar BW500 e mudar a grade).
- Sobretensão lenta de 6,5–8 V na GW (aceito).
- R4 aberto no A0 da GW faz o gateway subir em AP sozinho.
- Fuga de entrada do A6 e deriva com umidade na leitura de bateria (aceito, ~0,7 %).
- MUX_OUT da LP corre rente ao SPI (regra de firmware: não amostrar durante transação).
- Curto no chicote da ACT derruba a placa inteira (não há PTC por porta).
- SDA/SCL compartilham um par no cabo: limite de ~2 m.
- Assinatura MySensors sem folga de RAM comprovada em hardware; a chave HMAC é única para a rede (MEM).
- Antena da ACT dentro de quadro metálico (alcance desconhecido).

**Documentação defasada:**
- CL ainda diz "Factory reset: **não implementado**", mas a AF §9 e a GWR registram que foi implementado em 2026-10-17.
- A tabela de perfis de CL cita os Nós 01 e 13 como `LOW_POWER` e o Nó 80 como `ALWAYS_ON`. O INV diz que o Nó 1 é `ALWAYS_ON` e que os nós 13 e 80 foram removidos.

---

## 7. Fora de escopo explícito nas fontes

- RS485/Modbus nas placas novas — AF §2 #10. O Nó 13 foi removido (INV Apêndice B).
- DHT, pH e oxigênio no nó atuador (ACT) — AF §5.2 "Fora do escopo"; INV §7.
- 12 V e 220 V na PCB da ACT (cargas só no quadro) — AF §5.1.
- Gateway compartilhado entre propriedades (Multi-CAR) — MEM `decisao-gateway-por-propriedade`.
- LoRaWAN no gateway: um gateway real usaria SX1302; a opção só fica aberta nos nós — AF §6.
- Migração do Nó 4 e do Nó 11 (Kit Hélio) para placas M360 — AF §1.
- ESP32/ESP8266 em nós de campo (reservados ao gateway) — NF linha 363.
- Pareamento BLE — SPINE "Deferred" 3.
- Múltiplos canais RF físicos com nRF24 — SPINE "Deferred" 1, superado por D-09.
- Chave DIP para frequência — AF §2 (nota).

---

## 8. Detalhe técnico para arquitetura/addendum (não para o PRD)

- **Pinagem:** pinagem por placa e flags `MY_RF24_CE_PIN`/`MY_RFM95_RST_PIN` — AF §4.3, §5.4, §6; DRY "Rádio".
- **Header 2×5:** ordem dos sinais e chaveamento por pino cortado — AF §3.
- **Divisor de bateria:** 2 MΩ/1 MΩ, fórmula de correção pelo bandgap — AF §4.1.
- **VSW:** partida suave e regras de pinos em HIGH com o VSW desligado — AF §4.2.
- **Estado do 74HCT244:** pull-ups/pull-downs de boot e ordem de liberação do /OE — AF §5.2.
- **GW:** R5 no MOSI (SX1276 selecionado no power-up) e NSS sem pull-up — AF §6.1.
- **Topologia de proteção:** PTC + TVS + SS14 e cálculo de limites — AF §5.1, §6.1.
- **RJ45:** atribuição de canais dos TVS por ordem física dos pinos — AF §5.2, §11.8.
- **Fluxo EDA:** roteamento Freerouting, DSN/SES, `paridade.py`, `netdiff.py` — AF §11–§12.
- **BOM:** códigos LCSC e basic/extended — AF §9.1.
- **EEPROM:** mapa (0–511 MySensors, 512–520 node, 521+ device com CRC) — GWR "EEPROM"; CL "Convenções".
- **MQTT:** formato nativo dos tópicos, envelope JSON e regras de validação — GWR "MQTT"; INV §8.1.
- **Payload:** tipos por `V_*` e flags ×100 — INV §2.
- **Lib M360-DRY:** callbacks `onRead`/`onWrite`, `MyMessage` único, rótulos `PROGMEM` — API §1–§2.
- **Warm-up:** por que contar ciclos e não `millis()` — API §4.
- **Sincronizador ACK:** critério de matching do eco — CL "Sincronizador ACK".
- **Modem LoRa:** `RFM95_BW125CR45SF128`, potência e frequência de fábrica — DRY "Rádio"; BK §15.6.
- **Assinatura:** custo de RAM, `MY_SIGNING_SOFT_RANDOMSEED_PIN=A7`, payload ≤ 23 B — MEM; AF §4.3.

---

## 9. Perguntas que as fontes não respondem (e um PM faria)

1. **Alvo do produto:** o PRD cobre só a horta-piloto ou uma oferta replicável para N propriedades? A escolha muda a prioridade de pareamento, ID pelo backend e D-09.
2. **Critério de pronto da família M360:** quantas placas, que testes de bancada e que tempo de soak em campo antes de aposentar os protótipos?
3. **Migração:** os Nós 1, 2 e 99 e o gateway trocam de nRF24 para RFM95W de uma vez (é um rádio por rede) ou haverá convivência? Qual é a janela de corte?
4. **Autonomia:** qual é a meta de autonomia de bateria de um nó LP solar (dias sem sol) e qual corrente de sono a sustenta? Nada foi medido.
5. **Alcance:** qual é o alcance mínimo exigido (metros, com e sem visada) e quando usar o repetidor?
6. **Ambiente:** qual é o grau IP e a faixa de temperatura (estufa, sol direto)? A caixa está pendente.
7. **ANATEL:** quem consulta o laboratório e quando? A homologação é requisito para vender?
8. **Manutenção em campo:** é aceitável retirar a placa para atualizar firmware, ou OTA/bootloader por rádio entra no escopo?
9. **Segurança:** o portal sem autenticação e a falta de assinatura de mensagens são aceitáveis para um nó que aciona relés de 220 V? Qual é o modelo de ameaça?
10. **Failsafe:** os limites (600 s / 120 s / sem limite) são requisito do produto ou parâmetro por instalação? O backend já os trata como configuráveis.
11. **Nó 4:** ele ganha placa M360 (LP solar) ou o protótipo segue como fonte de clima indefinidamente?
12. **Sensores novos:** I2C (qual sensor?), sonda capacitiva e mais canais de solo — quais entram no próximo ciclo?
13. **Kit Hélio:** faz parte do produto ou é instalação à parte fora do PRD?
14. **Custo-alvo** por nó e por propriedade, e volume do primeiro pedido PCBA.
15. **Firmware genérico:** é pré-requisito da oferta multi-propriedade ou basta o comissionamento manual (env por nó + canal no portal)?
