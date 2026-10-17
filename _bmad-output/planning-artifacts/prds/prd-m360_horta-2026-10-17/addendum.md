---
title: "Addendum ao PRD — Manejo360 Horta"
status: final
created: 2026-10-17
updated: 2026-10-17
---

# Addendum ao PRD — Manejo360 Horta

Material que fundamenta o PRD mas pertence a documentos a jusante (arquitetura, épicos) ou serve de registro.

## 1. Onde está o "como"

| Assunto | Fonte normativa |
|---|---|
| Tópicos MQTT, heartbeat, eventos, formato nativo | spec do backend §2.2–§2.3; `src/DRY/horta/gateway/README.md` |
| Restrições de firmware que o backend precisa respeitar | spec §2.4 |
| Componentes, esquema relacional, máquina de estados do comando, agendador | spec §5.1–§5.5 |
| Configuração e variáveis de ambiente | spec §5.6 |
| Endpoints REST, Open-Meteo, relatório LLM | spec §6 |
| Confirmação pelo eco (pseudocódigo) | spec §7.1; `CLAUDE.md`, seção do Sincronizador ACK |
| Fórmulas e parâmetros do motor B | spec §7.3, §9.3; `docs/motor_agroclimatico_manejo360.md` |
| Failsafe, escalas, cooldowns | spec §9.1, §9.2, §9.4 |
| Migração do Node-RED (fases e dados) | spec §10 |
| Grade de canais LoRa e requisitos de firmware derivados | spec §15.6 |
| Pinagem, proteção, energia e pendências das placas | `hardware/ARQUITETURA_FAMILIA.md` |
| Nós, children, perfis, escalas | `src/DRY/horta/inventario.md` |
| Lib M360-DRY | `lib/M360-DRY/API_REFERENCE.md` |

## 2. Contradições dos documentos antigos (não herdar)

Os PRDs de junho e os documentos de MVP divergem do sistema real nos pontos abaixo. Vale a coluna "Vigente".

| Tema | Documentos antigos | Vigente |
|---|---|---|
| Papel dos canteiros | A = Manejo360, B (ou "M") = timer | A = timer (child 31), B = Manejo360 (child 32) |
| Rega do controle | 07:30 e 16:30, 10–15 min | 07:00 e 17:00, 300 s |
| Gatilho de rega | Tensão de água 20–30 kPa | ADC bruto 0–1023, alto = seco; limiares 350/650/700 |
| Escala de solo | 0–100 % | ADC bruto; o percentual é só exibição |
| Profundidades | 10 e 25 cm, ou 10/20/30 cm | 10 e 30 cm, a 1, 3 e 5 m |
| Sondas por nó | 3 + DS18B20, "12 sensores", 18 num nó | 6 por nó (Nós 1 e 2), sem DS18B20 |
| Perfil dos nós de solo | LOW_POWER a bateria | ALWAYS_ON, 5 V fixo |
| Fonte de clima | DHT11 do Nó 99 | Nó 4 (children 11/12) desde 2026-10-17 |
| Nó 4 | DHT22 + BH1750, ou DHT11 + pH | DHT11 + DS18B20, solar, 30 min |
| Nó 99 | 2 ou 4 solenóides + pH | 3 solenóides, 4 peristálticas, 2 bombas, vazão |
| Nós existentes | 13 (ZTS), 80 (caixas), 00, 03, lumi… | 1, 2, 4, 99; 11 no Kit Hélio |
| Tópicos MQTT | `m360/out/{gateway_id}/…` | `m360/{UF}/{CAR}/out` e `/in` |
| Onde rodam as regras | No gateway (`rules.json`) | Node-RED hoje; backend depois |
| Buffers da lib | `messages[NODE_ITEMS_COUNT + 2]` | Um `MyMessage` interno (M360-DRY 2.0+) |
| Doc do motor (`motor_agroclimatico_manejo360.md`) | Clima do Nó 99; profundidades 10/20/30 cm; lâmina "FAO-56" | Clima do Nó 4 (D-04); 10 e 30 cm; lâmina por déficit em ADC com fator atmosférico |
| Área do canteiro | 8,4 m² (MVP) × 6,0 m² e 35 plantas (motor e spec) | Em aberto (PRD Q-13) |
| Factory reset do gateway | Spec v1.3: "verificado em bancada" | `ARQUITETURA_FAMILIA.md` §9: falta bancada nas placas M360; o PRD segue esta |

## 3. Spine do firmware genérico (AD-01..AD-07) revisto

O spine de 2026-09-08 (branch `feat/firmware-generico-autoconfig`) propunha um binário único com autoconfiguração. As decisões posteriores mudaram o quadro. A F16 do PRD guarda o que sobrou.

| AD | Proposta | Situação | Destino |
|---|---|---|---|
| AD-01 | Binário único com perfil por máscara na EEPROM | Superado por "firmware por placa, configuração por nó" (2026-10-14); o monolito com assinatura não cabia na RAM | Descartado |
| AD-02 | Backend define `nodeId` e perfil por slot UF+CAR | Futuro; hoje o ID vem do env | FR-58 |
| AD-03 | Janela de pareamento de 60 s no gateway | Futuro; o repareamento da ACT não atribui ID | FR-58 |
| AD-04 | Tópico `m360/<UF>/<CAR>/config` retido | Futuro; o tópico não existe | FR-58, FR-59 |
| AD-05 | Reset de EEPROM do nó pelo child 254 | Conflita com o contrato: o 254 é o intervalo | FR-60, em outro child |
| AD-06 | `tenantId` de 16 bits em toda mensagem | Superado por um gateway por propriedade + canal D-09; o driver dá ACK antes da aplicação, então filtrar na aplicação não isola | Guarda residual, se houver |
| AD-07 | Canal RF24 76 único | Superado pelo LoRa 915 MHz e pela grade de 64 canais | FR-16, FR-55 |

Também divergiam do sistema real: a ordem dos segmentos do tópico (o real é `<command>/<ack>/<type>`, com payload no corpo), a área de perfil na EEPROM em 514–520 (hoje 512–520 é `M360NodeConfig`) e a pilha citando só nRF24.

## 4. Decisões de projeto vigentes

1. MCU Pro Mini 3,3 V / 8 MHz soldado nas placas novas.
2. Rádio em mezanino comum; um rádio por placa (RFM95W ou nRF24 pelo header 1×8).
3. Atuador a 3,3 V com 74HCT244 para os sinais de 5 V.
4. Carga solar por módulo TP4056+DW01 externo.
5. 74HC4051 na LP: oito canais de solo num pino.
6. RJ45 dentro da caixa, TVS em todas as linhas.
7. Placas novas com envs próprios; os envs antigos seguem servindo o hardware em campo.
8. RS485 fora do escopo.
9. PCBA nas quatro placas.
10. Reed switch nas três placas carrier.
11. Um gateway por propriedade (UF+CAR); Multi-CAR descartado em 2026-09-10.
12. Firmware por placa, configuração por nó (2026-10-14).
13. Fonte de clima = Nó 4 (2026-10-17).
14. Canal nRF24 76 validado; não trocar sem medição.
15. Separação de rádio entre propriedades por frequência (D-09).

## 5. Origem das entregas contratuais

Os relatórios em `docs/relatorio/` registram três marcos com nota fiscal (13/02, 04/05 e 07/08/2026) e um consolidado de 23/09/2026. Eles servem de histórico para os épicos concluídos, e não de requisito.

## 6. Fases da migração e decisões abertas do backend

| Fase | Entrega | Node-RED | Critério de saída |
|---|---|---|---|
| 0 | Infra, banco, broker de teste, simulador, CI | ligado | Simulador reproduz eco, preempção, failsafe e sono; testes verdes |
| 1 | Ingestor, persistência, registro, só leitura | ligado | 7 dias com contagem e status iguais aos do Node-RED |
| 2 | API, fluxo em tempo real, UI de leitura | ligado | Painel do backend usado no dia a dia |
| 3 | Despachante, botões manuais, painel admin | botões desligados | 20 comandos confirmados pelo eco; falha simulada vira `FALHOU` em 35 s com alerta |
| 4 | Agendador durável, Canteiro A, reconciliador | aba Irrigação desligada antes | 3 dias de regas A com OFF confirmado; teste de kill no meio da rega |
| 5 | Motor B, Open-Meteo, soak | idem | 7 dias de avaliações conferidas com o histórico do Node-RED |
| 6 | Telegram: alertas, bot, boletim, relatório | abas Telegram e IA desligadas | Mensagens equivalentes; assinantes migrados |
| 7 | Corte e backup final | desligado | 14 dias sem regressão (SM-6); rollback documentado |

| Decisão | Recomendação da spec | Alternativa |
|---|---|---|
| D-01 Linguagem e runtime | Python 3.12, asyncio, FastAPI | Node/TypeScript |
| D-02 Banco | PostgreSQL 16 particionado | SQLite |
| D-03 UI | Servida pelo backend, HTMX ou Vue leve | SPA ou Grafana |
| D-05 Hospedagem | Mesmo servidor atual, em containers | — |
| D-06 Autenticação | Usuário e senha locais + tokens, papel único `admin` | OIDC |
| D-07 Peristálticas na UI | Atrás de confirmação | Omitir |
| D-08 Kit Hélio | Segunda instalação, só leitura | Instância separada |
