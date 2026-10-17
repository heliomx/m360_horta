---
project_name: 'm360_horta'
user_name: 'Marcelo'
date: '2026-10-17'
sections_completed: ['technology_stack', 'critical_rules', 'planning_pointers']
existing_patterns_found: 24
---

# Contexto do projeto para agentes de IA

_Regras que um agente erraria sem aviso. O detalhe vive nos documentos apontados; este arquivo só lembra o que não se lê no código. Em conflito, valem o `CLAUDE.md` e o `src/DRY/horta/inventario.md`._

---

## Onde está cada verdade

| Assunto | Documento |
|---|---|
| Produto, escopo, FRs | `_bmad-output/planning-artifacts/prds/prd-m360_horta-2026-10-17/prd.md` |
| Decisões de arquitetura (AD-1 a AD-22) | `_bmad-output/planning-artifacts/architecture/architecture-m360_horta-2026-10-17/ARCHITECTURE-SPINE.md` |
| Épicos e histórias | `_bmad-output/planning-artifacts/epics.md` |
| Backend que substitui o Node-RED | `docs/especificacao-backend-m360.md` (v1.3; ajustes pendentes listados no spine) |
| Nós, children, escalas (contrato com a plataforma) | `src/DRY/horta/inventario.md` |
| Placas M360 | `hardware/ARQUITETURA_FAMILIA.md` |
| Regras de código e armadilhas | `CLAUDE.md`; nós e gateway: `.agent/skills/bmad-mysensors-node-coding/SKILL.md`; KiCad: `.agent/skills/m360-kicad-hardware-workflow/SKILL.md` |
| Nó novo | `.agent/workflows/m360-node-factory.md` |

## Stack e versões

- PlatformIO, monorepo com `extra_configs` (`src/DRY/horta`, `src/DRY/kit-helio`) e `src_dir = .`. Builds ficam em `%TEMP%\pio_builds`.
- Nós: ATmega328P com Arduino e MySensors 2.3.2. A lib M360-DRY 2.2.0 está em `lib/M360-DRY/`.
- Gateway: ESP8266 (espressif8266 2.6.3), ArduinoJson 6, PubSubClient 2.8, MQTT em modo nativo (`M360_NATIVE_MQTT=1`).
- Rádio:
  - As placas M360 usam RFM95W a 916,8 MHz.
  - Os protótipos e o Kit Hélio usam nRF24, canal 76.
  - O tipo de rádio é escolhido em `[common] radio_flags`.
- Plataforma: Node-RED em produção. O backend ainda não existe; as decisões de stack D-01 a D-08 estão abertas.
- Hardware: KiCad 10.0.4 e Freerouting 1.9.0, com PCBA na JLCPCB.

## Regras críticas

### Contrato e segurança

- **O inventário é o contrato.** Mudar child, rótulo, `S_*`/`V_*`, failsafe ou perfil exige atualizar, na mesma entrega, o firmware, o `inventario.md` e o consumidor (`flows.json` hoje). Child desconhecido faz o nó ignorar o comando em silêncio.
- **Children reservados:** 253 é debug, 254 é intervalo e 255 é bateria. Comandos administrativos são `V_CUSTOM` no child 0.
- **Solo em ADC bruto 0–1023, alto = seco.** Não existe conversão para 0–100 % na cadeia. Os Nós 1 e 2 usam `V_LEVEL` (37).
- **A segurança da atuação é do nó.**
  - O failsafe é de 600 s nas solenóides e 120 s nas peristálticas; as bombas 38/39 não têm limite.
  - O MUX tem um canal por vez, e nenhum relé liga no boot.
  - A plataforma limita cada rega a `failsafe − 15 s`.
  - O prazo conta do primeiro ON: um ON repetido não rearma (história 3.11, corrigida em 2026-10-17, sem bancada).
- **A confirmação é o eco de aplicação.** Vale `command=1, ack=0, type=2` com o payload igual. O ACK de transporte (`ack=1`) nunca confirma nem grava estado.
- **Payload de `V_STATUS` é só `"0"` ou `"1"`.** Qualquer outro valor desliga o relé (`atoi()`). No modo nativo, o payload MQTT é o valor bruto, nunca JSON.

### Firmware

- Nó novo usa `M360::M360Node`, com `NODE_ITEMS[]` declarativo, rótulos em `PROGMEM` por `M360_LABEL()` e drivers por `onRead`/`onWrite`. Não existe `messages[]`: a lib usa um `MyMessage` interno.
- Na M360-LP há um fonte único em `nos/lp/`. Cada nó é um `config/<nó>/nodeConfig.h` mais um env, com exatamente um de `LP_POWER_FIXED_5V`/`LP_POWER_BATTERY`.
- ACT e GW reaproveitam o fonte do protótipo, selecionado pelas flags `NODE99_BOARD_ACT` e `M360_BOARD_GW`.
- Macros `MY_*` ficam só no `platformio.ini`.
- O motor legado `nos/shared/` está congelado; nenhum env o compila.
- O nó envia por mudança ou a cada 10 ciclos. `reportIntervalMin` e `readSamples` não são lidos pela lib.
- Mudança na lib exige `pio check -e check_m360_dry` limpo, build de todos os `default_envs` e pelo menos 512 B de RAM livre em `act_99` e `nano_99reles`.

### Gateway

- **Tópicos:** sempre por `buildTopicOut`/`buildTopicIn`, e cada segmento lido por `parseTopicByte()`.
- **Envio:** todo envio passa por `dispatchCommand()` com `send(outMsg, false)`. O ACK de transporte é publicado à mão, com `targetNodeId`, porque `getSender()` vale 0 no envio.
- **Comando malformado:** gera `reportCommandRejected()` em `.../out/events`.
- **Ordem do boot:** WiFi em `before()`; MQTT e webserver em `setup()`.
- **Modo AP:** nada de `mqttClient.loop()` em modo AP.
- **EEPROM:** 0–511 é do MySensors, 512–520 é reservado e 521+ é `M360DeviceConfig` com CRC, escrita só por `saveConfig()`.
- **Factory reset:** o ímã, afastado e mantido 3 s nos primeiros 30 s, ou o portal (`POST /reset`) apagam a configuração e reiniciam em AP. O portal não tem autenticação.
- **Kit Hélio:** o gateway dele ainda não segue as regras acima (usa `atoi()` e `send(..., true)`).

### Node-RED (até o corte)

- Deploy só por `nr_deploy.py`, na ordem backup → diff → `deploy --tab <ID>` → diff. `deploy --all` só com pedido explícito.
- O servidor costuma estar à frente do repositório: fazer `pull` antes de editar o `flows.json`.
- O `flows.json` é CRLF, com indentação de 4 e `ensure_ascii=False`, sem newline final.
- A fonte de clima é o Nó 4 (children 11/12). Sem leitura, o motor do B cai em 20 °C e 20 %.
- O Canteiro A é o timer (07:00 e 17:00, 300 s, child 31). O B é o motor agroclimático (child 32).

### Backend (quando começar)

- **Só o despachante publica.** Agendador e regras criam comandos.
- **Posse por grupo físico** (`mux99` = 31–37, 38, 39), publicada e trocada com o grupo quieto.
- **Agendamento:** todo OFF é linha persistida; um ON vencido nunca executa; nenhum estado bloqueia OFF.
- **MQTT:** `retain` é proibido em `in/#`.
- **Configuração:** o banco manda; o YAML só semeia.

### Hardware

- O `kicad-cli` é o juiz: netlist antes e depois, `netdiff.py`, ERC zero.
- Nunca usar `replace_schematic_component` do MCP.
- Busca vazia não prova nada.

### Escrita e processo

- Comentários, logs, documentos e respostas em português.
- Segredos ficam em `include/M360Credentials.h` e `%USERPROFILE%\.m360\nodered.env`, nunca no Git nem no chat.
- Nenhuma placa M360 foi montada ou testada em bancada. Não afirmar que algo "funciona" na placa nova.
- O repositório central é o GitHub; trabalho não commitado só existe na máquina local.
