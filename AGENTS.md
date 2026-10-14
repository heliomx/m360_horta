# M360 Horta — Contexto Permanente para Codex

## Projeto
Sistema IoT de monitoramento agrícola M360 Horta.  
**Plataforma:** PlatformIO · Arduino/AVR · MySensors RF24 · ESP8266  
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

### Resumo das regras críticas

**Nunca escrever manualmente** o que já existe como macro no `node_engine.h`:

| Padrão proibido | Substituição obrigatória |
|---|---|
| Corpo manual de `presentation()` | `NODE_ENGINE_PRESENTATION(name, ver)` |
| 6 declarações de globais repetidas | `NODE_ENGINE_DEFINE_GLOBALS()` |
| Bloco validação/save/send de intervalo | `NODE_ENGINE_HANDLE_INTERVAL(msg)` |
| Loop V_STATUS para atuadores | `NODE_ENGINE_HANDLE_ACTUATORS(msg)` |
| Bloco diagnóstico de rede | `NODE_ENGINE_CHECK_TRANSPORT()` |
| Bloco teste de conectividade | `NODE_ENGINE_TEST_CONNECTIVITY()` |
| Ciclo static battCycle manual | `NODE_ENGINE_PROCESS_BATTERY(N)` |
| Loop `pinMode`/`digitalWrite` | `nodeEngine_setupPins()` |
| Loop `lastValues=NAN` / `nNoUpdates=0` | `nodeEngine_initArrays(lv, nu, cnt)` |
| String `"teste_do_gateway"` hardcoded | `CMD_FORCE_UPDATE` (de `config.h`) |
| `MyMessage messages[N]` fixo | `messages[NODE_ITEMS_COUNT + 2]` |
| Código após `sleep()` | mover `request()`/`wait()` para **antes** |

**Antes de criar qualquer novo nó**, ler o template em `SKILLmySensors.md` seção 3.

### Regras críticas do Gateway (`src/DRY/horta/gateway/`)

| Padrão proibido | Regra |
|---|---|
| Tópico MQTT como string literal | Sempre `buildTopicOut(config)` / `buildTopicIn(config)` |
| `EEPROM.put(&config)` direto | `saveConfig()` campo-a-campo com CRC |
| MQTT ou WebServer em `before()` | Exclusivos de `setup()` |
| `setupWiFi()` fora de `before()` | Exclusivo de `before()` |
| `mqttClient.loop()` em modo AP | Checar `WiFi.getMode() == WIFI_AP` primeiro |
| Lógica de infraestrutura em `libDryGatewayMqtt.cpp` | Módulo dedicado em `lib/M360-DRY/` |

---

## Estrutura Relevante

```
src/DRY/
├── horta/                       Sub-projeto Horta (platformio.ini próprio)
│   ├── gateway/                 libDryGatewayMqtt.cpp (usa M360::M360Gateway)
│   ├── nodered/flows.json       Fluxos Node-RED (dashboard + irrigação + ACK)
│   ├── inventario.md            Nós e child IDs — contrato com o Node-RED
│   └── nos/
│       ├── shared/              Motor legado (node_engine.h) — não usar em novos nós
│       ├── 01nodeSolo3dNano/    Solo resistivo — 6 canais nativos (Nó 1, LP)
│       ├── 02nodeSolo3dNano/    Solo resistivo — 6 canais nativos (Nó 2, LP)
│       ├── 04noodeSolarMini/    DHT11 + DS18B20, alimentação solar (Nó 4, LP)
│       └── 99nodeReles/         9 atuadores (7 MUX + 2 nativos) + DHT11 + vazão YF-S201 (Nó 99, ON)
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
- **EEPROM nós:** sempre via `nodeEngine_saveInterval()`, nunca `EEPROM.put()` direto
- **EEPROM gateway:** região 0–511 = MySensors, região 512+ = `DeviceConfig` com CRC
- **Perfil de energia:** definir `POWER_PROFILE_LOW_POWER` **ou** `POWER_PROFILE_ALWAYS_ON` — nunca os dois
- **Precisão float:** 1 casa decimal (`set(val, 1)`)
- **Solo:** escala 0 (seco) → 100 (água)
- **JSON gateway:** `DynamicJsonDocument(512)` para mensagens, `(384)` para heartbeat, `(256)` para eventos
- **Hardware Rádio RF24:** Todos os módulos nRF24L01+ de todos os nós e gateways possuem capacitor de desacoplamento instalado (premissa de hardware confirmada, não questionar).

---

## Diretrizes de Comunicação e Formatação no Chat

- **Proibido usar blocos matemáticos/LaTeX (`$$...$$` ou `$...$`) para formatar texto comum ou destaques:**  
  Nunca utilize notação matemática LaTeX (`$$\text{...}$$`, `\textbf{...}`, etc.) para enfatizar frases, rótulos de botões, badges ou valores de telemetria no chat (ex.: proibido `$$\text{Logo ao lado de }\textbf{Nó físico ID: 4}\text{ está o badge verde: }...$$`).  
  Utilize sempre **Markdown nativo e limpo** (`**negrito**`, `*itálico*`, `` `código/badge inline` `` ou listas), mantendo a renderização fluida, elegante e perfeitamente legível na interface.
