# M360 Horta — Contexto Permanente para Codex

## Projeto
Sistema IoT de monitoramento agrícola M360 Horta.  
**Plataforma:** PlatformIO · Arduino/AVR · MySensors (rádio por rede: nRF24L01+ na estufa; RFM95W 915 MHz para redes LoRa novas) · ESP8266  
**Arquitetura:** Monorepo com dois sub-projetos agregados por `extra_configs`:
- **Horta** — gateway em `src/DRY/horta/gateway/`, nós em `src/DRY/horta/nos/`
- **Kit Hélio** — gateway em `src/DRY/kit-helio/gateway/`, nós em `src/DRY/kit-helio/nos/`

O `platformio.ini` da raiz define `src_dir = .`, portanto todo `build_src_filter`
parte de `src/DRY/...`. Os envs vivem nos `platformio.ini` de cada sub-projeto.

---

## Deploy no Node-RED (`src/DRY/horta/nodered/`)

Procedimento completo (SSoT): `src/DRY/horta/nodered/DEPLOY.md`. Regra para Antigravity: `.agent/rules/nodered-deploy.md`.

- **Sem token.** Autenticação por usuário/senha em `%USERPROFILE%\.m360\nodered.env`
  (fora do repo, ACL só do usuário). Nunca copiar credenciais para repo, config de IDE, chat ou commit.
- **Ferramenta única:** `python src/DRY/horta/nodered/tools/nr_deploy.py <login|backup|diff|pull|deploy>`.
- **Ordem fixa:** `backup` → `diff` → (servidor à frente? `pull` e reaplicar) →
  `deploy --tab <ID>` só das abas alteradas → `diff` de confirmação → commit.
- **`deploy --all` só com pedido explícito.** **Aba Irrigação** só sobe com Nó 99 e gateway gravados com o failsafe.
- Em 2026-09-24 o servidor estava à frente do repo em todas as abas: fazer `pull` antes de editar `flows.json`.

---

## Skill Obrigatória — MySensors Node Coding & Gateway

Ao trabalhar em qualquer arquivo dentro de `src/DRY/` (nós **ou** gateway), aplicar **obrigatoriamente** os padrões definidos em:

- **Referência consolidada (SSoT):** `.agent/skills/bmad-mysensors-node-coding/SKILL.md`

---

## Regras Obrigatórias — Hardware KiCad (`hardware/`)

Ao trabalhar em qualquer arquivo dentro de `hardware/` (`.kicad_sch`, `.kicad_pcb`, `.kicad_mod`,
`.kicad_sym`, `tools/`), aplicar **obrigatoriamente**:

- **Referência consolidada (SSoT):** `.agent/skills/m360-kicad-hardware-workflow/SKILL.md`

O que não se negocia:

| Regra | Por quê |
|---|---|
| **`kicad-cli` é o juiz.** Toda edição de esquemático passa por netlist antes/depois + `hardware/tools/netdiff.py` + ERC com zero violações + comparação de campos e do bloco `lib_symbols` | O ERC **não** acusa fio que funde duas redes: o curto é eletricamente válido |
| **MCP KiCad** serve para catálogo JLCPCB, leitura e lint. Escrita em esquemático só peça a peça, lendo a rotação real no arquivo antes de desenhar fios, e `kicad-cli sch upgrade --force` depois. **Nunca `replace_schematic_component`** | Os mutadores do MCP falham em silêncio: ignoram o ângulo, gravam campos em outro componente e escrevem fora do formato canônico |
| **Busca vazia não é prova** (`grep`, `search_jlcpcb_parts`). Registrar a consulta junto da conclusão | Três conclusões erradas por padrão de busca errado em uma sessão |
| **Datasheets** que fundamentam decisões vão para `docs/datasheets/` com índice, não para pasta temporária | A próxima rodada precisa auditar sem refazer o download |
| **Placas marcadas como defasadas** não são comparadas com o esquemático; a lista "já conhecido" vive em `hardware/ARQUITETURA_FAMILIA.md` §9 | Evita achado repetido e comparação sem sentido |

---

## Regra Obrigatória — Manter o Inventário Sincronizado

Qualquer alteração de código em `src/DRY/horta/` **deve** ser refletida em
[`src/DRY/horta/inventario.md`](src/DRY/horta/inventario.md) na **mesma** entrega.
O inventário é a referência única de nós e child IDs, e o contrato com o
`flows.json` do Node-RED — se ficar defasado, comandos passam a ser descartados
em silêncio pelos nós, sem erro de compilação nem de log.

**Gatilhos que exigem atualizar o inventário:**

| Mudança no código | O que atualizar |
|---|---|
| Incluir, remover ou renumerar um `childId` | Tabela do nó + §1 (faixa de children) + §8 se for endereçado pelo Node-RED |
| Alterar `label`, `S_*`, `V_*`, `pin`, `reportIntervalMin`, `wakeOnRadio` ou `flags` | Linha correspondente na tabela do nó |
| Incluir ou remover um nó (env / `MY_NODE_ID`) | §1, seção própria do nó e Apêndice B |
| Trocar o perfil de energia | Cabeçalho da seção do nó e §1 |
| Mudar escala, unidade ou tipo do payload | §2 (tipo do payload) e §9 (escalas) |
| Alterar pinagem do nó | Coluna de pino + `esquema_eletrico.md` do nó |

**Verificação antes de entregar** — conferir que cada `childId` de
`NODE_ITEMS[]` / `nodeItems[]` aparece no inventário com o mesmo `label`,
`S_*`, `V_*` e demais atributos. Divergir aqui é o mesmo que quebrar o contrato
com o Node-RED.

> Alterar um `childId` sem atualizar o `flows.json` faz o nó **ignorar o comando
> silenciosamente** — `M360Node::handleMessage()` casa `childId` exato e não
> responde a IDs desconhecidos. O sintoma é timeout no Sincronizador ACK.

### Nós novos: biblioteca M360-DRY

Todo nó novo usa `lib/M360-DRY` (`M360::M360Node`): `NODE_ITEMS[]` em `static const`, rótulos em
`PROGMEM` com `M360_LABEL()`, **sem `messages[]`**, `node.begin(F(nome), F(versão))`, intervalo por
`M360::saveInterval()`. Perfis: `M360_LOW_POWER`, `M360_ALWAYS_ON`, `M360_PASSIVE`, `M360_REPEATER`.
Na placa M360-LP o nó é só um `nodeConfig.h` em `src/DRY/horta/nos/lp/config/`. Procedimento:
`.agent/workflows/m360-node-factory.md`; padrões: `.agent/skills/bmad-mysensors-node-coding/SKILL.md`;
detalhes e armadilhas MySensors ↔ MQTT ↔ Node-RED: `CLAUDE.md`.

**Antes de criar qualquer novo nó**, seguir o workflow `.agent/workflows/m360-node-factory.md` (Passo 0 decide entre a placa M360-LP e hardware próprio).

### Regras críticas do Gateway (`src/DRY/horta/gateway/`)

| Padrão proibido | Regra |
|---|---|
| Tópico MQTT como string literal | Sempre `buildTopicOut(config)` / `buildTopicIn(config)` |
| `EEPROM.put(&config)` direto | `saveConfig()` campo-a-campo com CRC |
| MQTT ou WebServer em `before()` | Exclusivos de `setup()` |
| `setupWiFi()` fora de `before()` | Exclusivo de `before()` |
| `mqttClient.loop()` em modo AP | Checar `WiFi.getMode() == WIFI_AP` primeiro |
| Lógica de infraestrutura em `libDryGatewayMqtt.cpp` | Módulo dedicado em `lib/M360-DRY/` |
| `atoi()` em segmento de tópico | `parseTopicByte()` — `atoi("xx")` vale 0 e o comando some sem rastro |
| Comando malformado descartado com `println` | `reportCommandRejected()` — publica o motivo em `.../out/events` |
| `send()` fora de `dispatchCommand()` | Ponto único: valida, envia e reporta sucesso **ou** falha |
| Montar `JsonDocument` no caminho nativo | `Translator::fromNative()` — os campos já vieram do tópico |

---

## Estrutura Relevante

```
src/DRY/
├── horta/                       Sub-projeto Horta (platformio.ini próprio)
│   ├── gateway/                 libDryGatewayMqtt.cpp (usa M360::M360Gateway)
│   ├── nodered/flows.json       Fluxos Node-RED (dashboard + irrigação + ACK)
│   ├── inventario.md            Nós e child IDs — contrato com o Node-RED
│   └── nos/
│       ├── lp/                  Firmware comum da placa M360-LP + config/<nó>/nodeConfig.h (envs lp_01, lp_02)
│       ├── 01nodeSolo3dNano/    Solo resistivo — 6 canais nativos no Nano (Nó 1, ON)
│       ├── 02nodeSolo3dNano/    Solo resistivo — 6 canais nativos no Nano (Nó 2, ON)
│       ├── 04noodeSolarMini/    DHT11 + DS18B20, alimentação solar (Nó 4, LOW_POWER)
│       └── 99nodeReles/         9 atuadores + vazão YF-S201; DHT11 só no Nano (Nó 99, ON; M360-ACT via NODE99_BOARD_ACT)
└── kit-helio/                   Sub-projeto Kit Hélio (platformio.ini próprio)
    ├── gateway/                 libDryGatewayMqtt.cpp
    └── nos/miniDHT/             DHT11 (Nó 11, ON)
```

> A infraestrutura do gateway (LEDs, webserver, WiFi, MQTT) vive em `lib/M360-DRY/`.
> O antigo `gateway/ngm/` e o subdiretório `withLibDRY/` não existem mais.

---

## Convenções de Código

- **Linguagem dos comentários e logs Serial:** português
- **IDs reservados:** 253 = Debug remoto (`V_TEXT`), 254 = Intervalo (`V_VAR1`), 255 = Bateria (`V_VOLTAGE`)
- **EEPROM nós:** endereços 512–515 via `M360::saveInterval()`, nunca `EEPROM.put()` direto
- **EEPROM gateway:** 0–511 = MySensors, 512–520 = `M360NodeConfig`/reservado, **521+** = `M360DeviceConfig` com CRC
- **Perfil de energia:** exatamente um `M360::M360_*` no construtor (`LOW_POWER`, `ALWAYS_ON`, `PASSIVE`, `REPEATER`); na M360-LP, exatamente um `LP_POWER_*` no `nodeConfig.h`
- **Precisão float:** 1 casa decimal (`set(val, 1)`)
- **Solo (Nós 1 e 2):** `V_LEVEL` (37) em **ADC bruto 0–1023, valor alto = seco** (sonda resistiva); o Motor de Regras do Node-RED opera nessa escala (`inventario.md` §9)
- **JSON gateway:** `DynamicJsonDocument(512)` para mensagens, `(384)` para heartbeat, `(256)` para eventos
- **Rádio:** escolhido em `[common] radio_flags` (hoje RFM95W) e seguido só pelas placas da família M360 (`lp_*`, `act_99*`, `d1_mini_gw`). Os protótipos em campo e o Kit Hélio são fixos em nRF24 (`[proto_rf24]` e `[kit_helio]`). Quando o nRF24 estiver ativo: todos os módulos nRF24L01+ possuem capacitor de desacoplamento instalado (premissa de hardware confirmada, não questionar).

---

## Diretrizes de Comunicação e Formatação no Chat

- **Proibido usar blocos matemáticos/LaTeX (`$$...$$` ou `$...$`) para formatar texto comum ou destaques:**  
  Nunca utilize notação matemática LaTeX (`$$\text{...}$$`, `\textbf{...}`, etc.) para enfatizar frases, rótulos de botões, badges ou valores de telemetria no chat (ex.: proibido `$$\text{Logo ao lado de }\textbf{Nó físico ID: 4}\text{ está o badge verde: }...$$`).  
  Utilize sempre **Markdown nativo e limpo** (`**negrito**`, `*itálico*`, `` `código/badge inline` `` ou listas), mantendo a renderização fluida, elegante e perfeitamente legível na interface.
