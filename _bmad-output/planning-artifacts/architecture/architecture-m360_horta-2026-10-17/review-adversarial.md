---
type: review-adversarial
target: ARCHITECTURE-SPINE.md (M360 Horta — sistema, 2026-10-17, draft)
method: pares de unidades um nível abaixo que obedecem a todos os ADs ao pé da letra e ainda assim montam um sistema incompatível
created: '2026-10-17'
status: para triagem do arquiteto (o spine não foi editado)
---

# Revisão adversarial do Architecture Spine — M360 Horta

## Como ler

Cada furo é um **par de unidades** (épico, equipe, módulo ou fase) que cumpre todos os ADs
literalmente e mesmo assim produz incompatibilidade: forma de dado divergente, dois donos da
mesma entidade, dois caminhos de mutação de estado, configuração com dono ambíguo, ou corrida de
tempo/ordem. Para cada um: cenário concreto, por que os ADs atuais não impedem, e a regra nova
ou apertada que fecha o furo.

Fontes conferidas além do spine: `docs/especificacao-backend-m360.md` v1.3,
`src/DRY/horta/inventario.md`, `lib/M360-DRY/src/M360Config.cpp` (`Config::reset`),
`lib/M360-DRY/src/M360Config.h` (janela do smartSleep), `lib/M360-DRY/src/M360Node.cpp`
(sufixo de perfil), `src/DRY/horta/nos/99nodeReles/99nodeReles.cpp` (`armFailsafe`,
`maxOnSecondsFor`), `include/M360Credentials.h`, `platformio.ini` dos dois sub-projetos.

Severidade:

- **CRÍTICA** — efeito físico (água, relé, rede de outra propriedade) ou perda de controle de uma instalação, sem sinal claro.
- **ALTA** — comportamento errado em produção, contrato quebrado em silêncio ou decisão de rega errada.
- **MÉDIA** — divergência que gera retrabalho, alertas contraditórios ou dado inconsistente, com sinal visível.
- **BAIXA** — risco de deriva sem efeito imediato.

## Resumo

| Severidade | Quantidade |
|---|---:|
| CRÍTICA | 5 |
| ALTA | 11 |
| MÉDIA | 12 |
| BAIXA | 1 |
| **Total** | **29** |

| ID | Sev. | Par de unidades | Furo em uma linha |
|---|---|---|---|
| C1 | CRÍTICA | Firmware do gateway (factory reset) × serviço de instalações | Padrão de fábrica do gateway é o prefixo de produção `DF/0000` |
| C2 | CRÍTICA | Backend fase 3 (botões) × Node-RED aba Irrigação | "Grupo" de posse não é o grupo físico do MUX |
| C3 | CRÍTICA | Qualquer produtor em `in/` × gateway | Mensagem retida em `in/` religa relé a cada reconexão do gateway |
| C4 | CRÍTICA | Comissionamento E3 (`SET_RADIO`) × piloto em produção | Canal de fábrica k=8 é o canal do piloto; `SET_RADIO` pode sequestrar o Nó 99 do piloto |
| C5 | CRÍTICA | Firmware do Nó 99 × despachante/regras | Todo ON rearma o failsafe; ON repetido ou runs encadeadas anulam a "última barreira" |
| A1 | ALTA | Node-RED (migração) × backend | Passagem de posse não transfere runs ativas, OFF pendente, `last_irrigation_at`, contador |
| A2 | ALTA | Backend (flag de posse) × Node-RED | Flag de posse só existe no banco do backend; o Node-RED não o lê |
| A3 | ALTA | Agendador durável × regra do Canteiro A | Ação de LIGAR vencida é executada no restart ("atrasado é melhor") |
| A4 | ALTA | UI de comando manual × motor de irrigação | Dois caminhos para ligar o child de um canteiro |
| A5 | ALTA | Firmware Nó 99 × configuração do backend | AD-1 não lista failsafe nem grupo MUX entre os campos do contrato |
| A6 | ALTA | Serviço de instalações (UI/banco) × teste de contrato (YAML/inventário) | Três donos da configuração de atuadores e canteiros |
| A7 | ALTA | Lib M360-DRY (envio por variação) × motor do Canteiro B | Idade da leitura ≠ idade do nó; solo seco estável vira `SEM_LEITURA` |
| A8 | ALTA | Firmware E3 `SET_RADIO` × despachante (admin = `ENVIADO`) | Comando administrativo com efeito persistente não tem confirmação nem ordem de corte |
| A9 | ALTA | Lib (smartSleep) × caixa postal do backend (2 s) | Janela de despertar com dois donos e valores diferentes |
| A10 | ALTA | Estados de instalação/`read_only` × agendador de OFF | Bloqueios de estado bloqueiam também o desligamento |
| A11 | ALTA | Agendador × despachante | AD-5 deixa o agendador publicar direto; a spec diz que só o despachante publica |
| M1 | MÉDIA | Registry do gateway × registro do backend | Status de nó com dois calculadores e dois alertas |
| M2 | MÉDIA | Ingestor do backend × Node-RED na convivência | Eco de comando do Node-RED vira "não solicitado" no backend |
| M3 | MÉDIA | `Translator::validate()` × validação do backend | Duas implementações de validação sem vetor de teste comum |
| M4 | MÉDIA | Gateway (JSON de heartbeat/eventos) × ingestor | AD-4 fixa tópicos, não os esquemas JSON |
| M5 | MÉDIA | Spine × spec (enums) | Listas de estados divergem entre spine e spec |
| M6 | MÉDIA | Firmware LP/ACT (E3) × gateway | AD-12 é um mapa só para duas MCUs; faixa do nó sem dono por byte |
| M7 | MÉDIA | Checklist de comissionamento × posse (AD-6) | C6/C7/C9 acionam relés numa instalação viva |
| M8 | MÉDIA | Notificador × AD-13 | Escopo por instalação incompleto (chat global, `read_only` global, usuários) |
| M9 | MÉDIA | Inventário × comissionamento C3/C4 | Variante de placa (Nano × ACT) não é registrada por instalação |
| M10 | MÉDIA | Integrador de volume do backend × contador do Node-RED | Dois integradores e importação única no momento errado |
| M11 | MÉDIA | Despachante (deadline) × firmware (prazo) × QoS 1 | Margem de 15 s sem instante de referência; carimbo de recepção de mensagem enfileirada |
| M12 | MÉDIA | Protótipo Horta nRF24 × Kit Hélio nRF24 | AD-14 só isola LoRa; redes nRF24 no canal 76 sem regra |
| B1 | BAIXA | Stack (`MySensors ^2.3.2`) × premissas da spec | Faixa semver permite versão que invalida comportamentos verificados em 2.3.2 |

---

## CRÍTICAS

### C1 — O padrão de fábrica do gateway é o prefixo de produção

- **Par:** firmware do gateway (RF-PRV-09, `M360::FactoryReset`) × serviço de instalações do backend (descoberta, RF-PRV-03).
- **ADs obedecidos:** AD-13 (uma instalação = um prefixo = um gateway), AD-18 (credenciais em `include/M360Credentials.h`), AD-12.
- **Cenário:** `Config::reset()` grava `M360_UF`/`M360_CAR_NUMBER` de `include/M360Credentials.h`, que no repositório de trabalho são `DF`/`0000` — o prefixo do piloto —, além do SSID e da credencial MQTT de compilação. Um gateway reaproveitado para outra propriedade, depois do factory reset, se ninguém concluir o portal, sobe no **prefixo do piloto** com a credencial do piloto (a spec RF-PRV-09 confirma: "se ninguém configurar pelo portal, o boot seguinte os usa"). O mesmo vale para um gateway recém-gravado e para o gateway do Kit Hélio, compilado do mesmo arquivo. O backend não cria `PENDENTE` (o prefixo existe e está `ATIVA`); os nós da outra propriedade (IDs 1..254 recomeçam em cada uma) se fundem ao registro do piloto; dois gateways assinam `m360/DF/0000/in/#` e um `atuar(99,31,1)` sai pelos dois rádios.
- **Por que os ADs não impedem:** AD-13 declara "um gateway por prefixo" mas não diz **como** isso é garantido nem quem detecta violação. A ACL por instalação (RF-PRV-07) não ajuda, porque o firmware usa a mesma credencial de compilação.
- **Regra proposta (AD-13 apertado):**
  1. O padrão de compilação do gateway nunca é um prefixo válido de instalação: `UF`/`CAR` de fábrica são um valor reservado (ex.: `XX`/`0`) que o firmware recusa usar em modo STA; sem configuração do portal o gateway permanece em AP.
  2. A credencial MQTT de compilação não é a de nenhuma instalação; em produção cada gateway recebe a sua pelo portal.
  3. O heartbeat carrega a identidade física do gateway (ID do chip / MAC). A instalação guarda o ID do gateway vinculado; heartbeat de outro ID no mesmo prefixo gera alerta `gateway_conflict` e **suspende comandos** da instalação até o operador resolver.

### C2 — "Grupo" de posse não é o grupo físico do MUX

- **Par:** backend na fase 3 (despachante + botões manuais, spec §10) × Node-RED com a aba Irrigação ainda ligada.
- **ADs obedecidos:** AD-6 ("a posse é um flag explícito por grupo"), AD-5, AD-3.
- **Cenário:** a spec §10 desliga na fase 3 só os botões do Node-RED e dá ao backend os botões de 31–39, enquanto a aba Irrigação (Node-RED) continua dona do 31 e do 32 até a fase 4. Uma equipe define "grupo" como a aba (botões manuais = um grupo, irrigação = outro) e cumpre AD-6 à letra. O operador liga a Solenóide C (33) pela UI do backend durante a rega do Canteiro A: o MUX preempta o 31, a rega A é cortada, e o Node-RED segue contando tempo e envia um OFF ao 31 já desligado.
- **Por que os ADs não impedem:** AD-6 diz na mesma regra "um único sistema comanda os children 31–37" e "posse por grupo", sem definir grupo. A fase 3 da spec contraria a primeira leitura e é compatível com a segunda.
- **Regra proposta (AD-6 reescrito):** a unidade de posse é o **grupo de exclusão física** do firmware (`mux99` = 31–37 inteiro; 38 e 39 cada um por si), nunca a aba, o canteiro ou a função. Nenhuma fase da migração divide `mux99` entre dois sistemas: ou os botões 31–37 ficam no Node-RED até a fase 4, ou a aba Irrigação é desligada na fase 3. A spec §10 precisa ser corrigida nesse ponto.

### C3 — Mensagem retida em `in/` religa relé a cada reconexão do gateway

- **Par:** qualquer produtor de comando (despachante do backend, `mqtt out` do Node-RED, ferramenta de teste, simulador) × gateway.
- **ADs obedecidos:** AD-4 (gramática, payload `"0"`/`"1"`, montagem por função), AD-5.
- **Cenário:** uma equipe publica comandos com `retain=true` (um campo esquecido no nó `mqtt out`, ou "para a UI ver o último estado"). O gateway reinicia (reconexão WiFi, factory reset, queda de energia), reassina `in/#` e recebe o último `in/99/38/1/0/2 = 1` retido: a bomba NFT, que não tem failsafe, liga sem comando novo e sem registro no backend. Com um retido `= 1` de solenóide, a rega recomeça a cada reboot do gateway. O callback do PubSubClient não expõe a flag de retenção, então o gateway não tem como filtrar.
- **Por que os ADs não impedem:** AD-4 fixa tópico e payload, não flags de publicação (retain, QoS) nem a semântica de reentrega.
- **Regra proposta (AD-4 ampliado):** comandos em `{prefixo}/in/#` são sempre `retain=false`; o broker é configurado para recusar retenção nessa árvore (verificado no checklist de comissionamento); o backend, que já assina `in/#`, alerta ao receber mensagem retida ali. Fixar também o QoS de `in/` e `out/` e declarar que comando reentregue tarde é comando novo, não repetição inofensiva.

### C4 — Canal de fábrica k=8 é o canal do piloto; `SET_RADIO` pode sequestrar o nó do piloto

- **Par:** comissionamento E3 (backend de instalações + firmware `SET_RADIO`) × instalação piloto `DF/0000` em produção.
- **ADs obedecidos:** AD-14 (grade D-09, fábrica k=8), AD-11 (`SET_RADIO` como `V_CUSTOM`), AD-13.
- **Cenário:** o piloto opera em 916,8 MHz (`MY_RFM95_FREQUENCY=916800000UL`) = k=8, que é também o canal de fábrica de todo binário. Ao comissionar uma propriedade vizinha dentro do alcance de rádio, os nós novos sobem em k=8 e o gateway do piloto confirma seus pacotes (IDs repetidos: um segundo "Nó 99" entra no registro do piloto). Pior: o `SET_RADIO:<k>` enviado ao "Nó 99" da propriedade nova é endereçado por ID; o Nó 99 **do piloto**, no mesmo canal, aceita mensagem destinada ao seu ID independentemente do gateway de origem, troca de canal e some da rede do piloto — com solenóides e bombas.
- **Por que os ADs não impedem:** AD-14 diz que o canal isola propriedades, mas não proíbe uma instalação ativa de ocupar o canal de fábrica nem exige prova de presença física para o comando de provisionamento.
- **Regra proposta (AD-14 ampliado):**
  1. O canal de fábrica é **reservado para comissionamento**: nenhuma instalação `ATIVA` opera nele. O piloto migra de k=8 antes da primeira propriedade nova (a E3 inclui essa migração).
  2. `SET_RADIO` só é aceito pelo nó numa janela de provisionamento aberta por presença física (reed, ou os primeiros N minutos após o boot) e nunca em nó que já tem canal gravado diferente do de fábrica sem essa janela.
  3. O cadastro (RF-PRV-12) recusa o canal de fábrica como canal de operação.

### C5 — Todo ON rearma o failsafe; ON repetido ou runs encadeadas anulam a "última barreira"

- **Par:** firmware do Nó 99 (`armFailsafe()` grava `millis() + limite` a cada ON, `99nodeReles.cpp`) × despachante/motor de regras do backend.
- **ADs obedecidos:** AD-2 (limite `failsafe − 15 s` por rega; "nenhuma camada pode desativá-los"), AD-7, AD-3.
- **Cenário:** três caminhos legítimos reemitem ON ao mesmo child: (a) o retry de RF-CMD-05 quando o eco se perdeu mas o relé já ligou; (b) uma rega manual de 20 min implementada como duas runs de 585 s seguidas, cada uma cumprindo AD-2; (c) uma regra que "renova" a rega em curso ao reavaliar. Em todos, o prazo físico recomeça do zero e o relé pode ficar ligado indefinidamente sem o failsafe disparar. Ninguém "desativou" o failsafe, logo AD-2 está cumprido.
- **Por que os ADs não impedem:** AD-2 limita a duração **de cada rega**, não o tempo contínuo ligado de um child, e não diz se ON repetido rearma o prazo.
- **Regra proposta (AD-2 apertado):** o limite físico vale para o **tempo contínuo ligado do child**. Firmware: ON recebido com o child já ligado **não** rearma o prazo (gravar no inventário §7.3). Plataforma: o despachante recusa ON para child cujo estado é `ON` ou incerto; entre duas runs do mesmo child há OFF confirmado e intervalo mínimo configurado; o motor não "renova" run.

---

## ALTAS

### A1 — Passagem de posse não transfere estado de domínio

- **Par:** Node-RED durante a migração × backend (fases 3–7 e rollback).
- **ADs obedecidos:** AD-6, AD-7.
- **Cenário:** na troca de posse do Canteiro B, o Node-RED tem uma rega em curso com OFF num `setTimeout`; a aba é desligada e o OFF morre com ela. O backend não tem run ativa, logo não agenda OFF; o reconciliador só age após 660 s, depois do failsafe. O `last_irrigation_at` do Node-RED não está na tabela de migração da spec §10: o backend começa sem soak e rega o B logo depois da última rega do Node-RED. No rollback da fase 7 (religar o Node-RED e pôr o backend em `read_only`), os `irrigation_off` pendentes do backend deixam de publicar.
- **Regra proposta (AD-6 ampliado):** a troca de posse de um grupo só ocorre com o grupo **quiescente** (nenhum atuador `ON`, nenhuma run ativa, nenhum OFF pendente) ou com protocolo explícito de entrega: o cedente executa seus OFF pendentes antes de ceder; o cessionário importa `last_irrigation_at`, runs abertas e contadores. Vale nos dois sentidos, inclusive no rollback.

### A2 — O flag de posse só existe de um lado

- **Par:** backend (flag de posse no banco, "visível no health") × Node-RED.
- **ADs obedecidos:** AD-6.
- **Cenário:** o backend respeita o flag; o Node-RED não sabe que ele existe. Basta um `deploy --all` (que restaura abas desativadas) ou um inject de teste da aba MQTT para haver dois escritores, e AD-6 continua cumprido "pelo backend". Nada detecta o segundo escritor.
- **Regra proposta:** a posse é **fato compartilhado** (tópico retido `{prefixo}/control/owner/{grupo}`, fora da árvore `in/`, ou equivalente) que todo produtor consulta antes de publicar; o backend, que já assina `in/#` (RF-ING-01), alerta `foreign_writer` quando vê comando para um grupo do qual é dono e que não emitiu.

### A3 — Ação de LIGAR vencida é executada no restart

- **Par:** agendador durável (spec §5.4: "na subida do processo, ações vencidas são executadas imediatamente") × regra do Canteiro A (cron 07:00/17:00) e motor do B.
- **ADs obedecidos:** AD-7, AD-17.
- **Cenário:** o backend fica fora das 06:50 às 11:10. Na subida, o `cron_fire(A)` das 07:00 executa às 11:10 — rega fora do protocolo da parcela de controle do experimento (F14). Fora por 12 h, os disparos de 07:00 e 17:00 vencidos rodam em sequência. "Um OFF atrasado é melhor que nenhum" foi generalizado para o ON.
- **Regra proposta (AD-7 ampliado):** toda ação persistida declara validade (`expires_at`). Ações de **desligamento e segurança** executam atrasadas; ações que **ligam** ou dosam, vencidas além da validade, viram `PERDIDA` com registro e alerta, nunca execução.

### A4 — Dois caminhos para ligar o child de um canteiro

- **Par:** UI/API de comando manual (`POST /api/commands` `atuar`) × motor de irrigação (`POST /api/irrigation/beds/{id}/run`).
- **ADs obedecidos:** AD-5 (os dois pedem ao despachante), AD-7.
- **Cenário:** `atuar(99,32,1)` pela API de comandos liga a solenóide do Canteiro B sem `irrigation_run`: não há OFF agendado (AD-7 não se aplica: não há "desligamento agendado"), não há soak, não há volume atribuído, e o motor do B não sabe da rega. Quem desliga é o failsafe aos 600 s. O indicador do experimento (F14) perde água sem rastro.
- **Regra proposta:** child vinculado a canteiro (`irrigation_bed.child`) só liga por run. O despachante rejeita `atuar(…,1)` direto nesses children ou o converte em rega manual (RF-IRR-08). Em geral: todo ON de atuador com failsafe nasce com OFF persistido.

### A5 — O contrato de AD-1 não inclui failsafe nem grupo MUX

- **Par:** firmware do Nó 99 (`maxOnSecondsFor()`, `IS_MUX_CH`) × configuração do backend (`failsafe_s`, `mux_group`).
- **ADs obedecidos:** AD-1 lista "child ID, rótulo, `S_*`, `V_*`, escala e perfil".
- **Cenário:** a equipe de firmware muda o failsafe das solenóides para 300 s (por exemplo, numa variante ACT) e não toca a configuração do backend: AD-1 não exige. O backend continua limitando a 585 s; cada rega do B é cortada aos 300 s pelo failsafe e vira `failsafe_or_preempt`, com alerta a cada rega. Mover um atuador do MUX para pino nativo muda a exclusão sem tocar nenhum campo listado.
- **Regra proposta (AD-1 ampliado):** os campos do contrato incluem `kind` (sensor/atuador), **failsafe**, **grupo de exclusão** e **janela de despertar** do perfil. Um teste de contrato em CI compara `maxOnSecondsFor()`/`NODE_ITEMS[]` com o inventário, e o inventário com a configuração do backend.

### A6 — Três donos da configuração de atuadores e canteiros

- **Par:** serviço de instalações (banco, editável pela UI — RF-PRV-01, RF-UI-07) × teste de contrato da spec §14 (lê a configuração `actuators`/`beds` do YAML) × inventário (AD-1).
- **ADs obedecidos:** convenção "Banco em produção; YAML só como semente", AD-1.
- **Cenário:** a equipe X implementa a semente como "aplica o YAML a cada start" (o YAML é versionado, então seria a verdade); a equipe Y como "insere se ausente". O operador ajusta `failsafe_s` ou `area_m2` na UI: no primeiro caso a edição some no próximo deploy; no segundo, o teste de contrato verde compara o inventário com um YAML que já não é a verdade. Um operador pode gravar `failsafe_s = 900` e o limite de rega sobe para 885 s, acima do firmware.
- **Regra proposta:** separar por natureza. **Espelho do firmware** (child, `S_*`/`V_*`, failsafe, grupo MUX): dono é o inventário; entra no banco só por migração gerada dele; somente leitura na UI. **Operacional** (cron, parâmetros do motor, área, coordenadas, assinantes): dono é o banco; o YAML é semente aplicada uma vez, por versão. O teste de contrato roda contra o banco (endpoint de health), não contra o YAML.

### A7 — Idade da leitura não é idade do nó

- **Par:** lib M360-DRY (envia só com variação > 0,05 ou após 10 ciclos sem mudança) × motor do Canteiro B (`reading_max_age_s = 1800`, `max_age_s = 3600` do clima).
- **ADs obedecidos:** AD-16, AD-10, AD-1 (o intervalo não é campo de AD-1).
- **Cenário:** o administrador muda o intervalo do Nó 2 para 5 min pelo painel (`set_interval`, permitido). Com o solo seco e estável, o nó fica até 50 min sem mandar leitura; aos 30 min o motor declara `SEM_LEITURA` e **não rega um canteiro seco**. No Nó 4 (`LOW_POWER`, 30 min, DHT11 de resolução inteira), uma noite estável deixa até 5 h sem leitura de 11/12; o motor cai na Open-Meteo com o nó vivo.
- **Regra proposta:** frescor = último contato do nó no ciclo (qualquer mensagem, inclusive notificação de sono), não carimbo da última leitura; ou a lib envia todo ciclo os children marcados como fonte de regra. A validação de configuração recusa `max_age < 10 × intervalo + margem` para nó-fonte, e `set_interval` em nó-fonte é validado contra isso.

### A8 — Comando administrativo com efeito persistente não tem confirmação nem ordem de corte

- **Par:** firmware E3 (`SET_RADIO:<k>` com "eco antes de trocar", spec §15.2) × despachante (spec §5.3/RF-CMD-07: administrativos terminam em `ENVIADO`) × portal do gateway (canal gravado localmente).
- **ADs obedecidos:** AD-3 (só define confirmação de atuação), AD-11, AD-14 ("o canal é estado da instalação na plataforma **e** da configuração do gateway").
- **Cenário:** o firmware ecoa `RADIO:k` no child 253; o despachante marca `ENVIADO` ao publicar e o serviço de instalações troca o canal do gateway. Um nó LP dormindo recebe o `SET_RADIO` pela caixa postal horas depois, quando o gateway já mudou: fica órfão no canal antigo sem caminho de volta. Ou o gateway troca pelo portal (o segundo dono do canal) antes de o backend mandar `SET_RADIO`: nenhum nó é alcançável. AD-14 dá literalmente dois donos ao canal.
- **Regra proposta:** (1) AD-3 estendido a todo comando com efeito persistente (`set_interval`, `SET_RADIO`, reset remoto): mensagem de confirmação definida (child, tipo, payload) no inventário e `CONFIRMADO` obrigatório. (2) O canal tem **um** dono, o backend; o portal só o grava no comissionamento inicial. (3) Troca de canal é transação: todos os nós confirmam com ativação diferida → o gateway troca por comando do backend → nó que perde o uplink por N ciclos volta ao canal anterior (fallback no firmware).

### A9 — Janela de despertar com dois donos e valores diferentes

- **Par:** lib/MySensors (`smartSleep`, espera de 500 ms; `M360_MIN_AWAKE_MS` 3 s em `M360Config.h`) × caixa postal do backend (`lp_awake_window_s = 2`, publicação em < 500 ms).
- **ADs obedecidos:** AD-10, AD-5.
- **Cenário:** a equipe da lib ajusta o tempo acordado para economizar bateria; o backend mantém 2 s fixos e publica comandos que chegam com o nó já dormindo. Comandos administrativos "terminam em `ENVIADO`", então ninguém percebe a perda. Somando a latência broker → gateway → rádio (e um salto pelo repetidor do Nó 99), a janela útil é menor que a nominal.
- **Regra proposta:** a janela de despertar é parâmetro do contrato por perfil (AD-1/inventário), com dono na lib; o backend a lê da configuração, nunca a fixa, e só considera entregue comando a nó `LP` pela confirmação (A8).

### A10 — Bloqueios de estado bloqueiam também o desligamento

- **Par:** máquina de estados da instalação (`SUSPENSA`: "comandos recusados", RF-PRV-06; `PENDENTE`), modo `read_only`, chave `confirmacao_habilitada` × agendador de `irrigation_off` e reconciliador.
- **ADs obedecidos:** AD-7, AD-13, AD-6.
- **Cenário:** o operador suspende a instalação durante uma rega (manutenção): o `irrigation_off` é recusado e a solenóide fica até o failsafe; bombas 38/39, sem failsafe, ficam ligadas indefinidamente. O mesmo ao virar `read_only` no rollback (A1).
- **Regra proposta:** nenhum estado de instalação, modo ou chave operacional bloqueia **OFF**. As restrições valem só para ON/dosagem. A transição para `SUSPENSA`/`read_only` executa antes os OFF pendentes.

### A11 — O agendador publica direto?

- **Par:** agendador durável × despachante.
- **ADs obedecidos:** AD-5 ("só o despachante e o agendador publicam no broker"); a semente estrutural desenha `AG -- publish --> BR`.
- **Cenário:** a equipe do agendador publica `in/99/32/1/0/2 = 0` direto: o OFF não entra na fila do Nó 99, não respeita a serialização do grupo MUX, não tem retry nem confirmação e não gera registro `command`. Se o eco se perde, o OFF nunca é reenviado e o banco mostra `ON` (ou `OFF` sem prova). A spec RF-CMD-01 diz o contrário ("nenhum publish fora do despachante"); duas equipes, dois documentos, ambas "conformes".
- **Regra proposta (AD-5 corrigido):** só o despachante publica comandos. Agendador e reconciliador **criam comandos** no despachante, como regra e botão. Corrigir a semente estrutural.

---

## MÉDIAS

### M1 — Status de nó com dois calculadores

- **Par:** `Registry` do gateway (eventos `node_lost`/`node_reconnected` com timeout próprio) × registro do backend (RF-REG-09/10, que "espelha" o do gateway).
- **Cenário:** RF-REG-08 exclui da cadência as acordadas por reed; o gateway não. Os timeouts divergem, e RF-WDG-02 manda os eventos do gateway **também** alimentarem alerta: o Telegram diz "nó perdido" enquanto a UI mostra `ONLINE`. Um reboot do gateway zera seu registro e gera rajada de `node_discovered`. A convenção diz "calculados num só ponto"; os ADs não dizem qual.
- **Regra proposta:** o status de nó tem um dono, o backend; eventos de nó do gateway são evidência persistida e nunca disparam alerta por si.

### M2 — Eco de comando do Node-RED vira "não solicitado" no backend

- **Par:** ingestor/despachante do backend × Node-RED dono de um grupo durante a convivência.
- **Cenário:** o Node-RED fecha a rega do A com `31=0`; o backend não tem comando em voo e aplica RF-CMD-11: `actuator_unsolicited_off`, "failsafe falando", alerta.
- **Regra proposta:** comando observado em `in/#` que o backend não emitiu é registrado como comando externo (`origin = external`) e participa da correlação; eco que casa com ele não é "não solicitado".

### M3 — Duas validações de comando sem vetor de teste comum

- **Par:** `Translator::validate()` no gateway × validação "espelhada" do backend (spec §2.3).
- **Cenário:** a spec do backend aceita `node ∈ 1..255`; o inventário §8.1 descreve 0–255 no gateway. O backend aceita `ler()` (`C_REQ`) em atuador, que a spec §2.4 diz nunca responder; o comando fica `ENVIADO` para sempre. AD-4 exige "função equivalente", sem dizer contra o quê.
- **Regra proposta:** tabela normativa de validação num lugar só (inventário §8.1) e um arquivo de vetores (tópico, payload, resultado esperado) executado pelo teste da lib e pela CI do backend.

### M4 — Esquemas JSON de heartbeat e eventos sem dono nem versão

- **Par:** gateway (`M360MQTT`, `M360Translator`) × ingestor.
- **Cenário:** o C5 do comissionamento exige `radioChannel` no heartbeat; a equipe de firmware o adiciona com outro nome ou renomeia `nodeId`. AD-4 cobre só tópicos; o ingestor quebra ou ignora em silêncio. O envelope JSON alternativo também é "suportado" pelos dois lados sem esquema.
- **Regra proposta:** esquema versionado (`schema: n`) para heartbeat, `gateway/status`, `events` e envelope; fixtures geradas do firmware e consumidas pela CI do backend; campo obrigatório ausente gera métrica e alerta.

### M5 — Listas de estados divergem entre spine e spec

- **Par:** equipes de UI/API (usam o spine) × equipes de domínio/banco (usam a spec §5).
- **Cenário:** o spine lista estados de comando sem `AGUARDANDO_DESPERTAR` e `CANCELADO`, que a spec §5.3 tem; põe `RESETADO` como estado de **instalação**, mas na spec (RF-PRV-09) é estado do **gateway** e o esquema de `installation` não o tem; omite `NEW` (RF-REG-01) dos estados de nó. Enum de banco e enum de UI não batem.
- **Regra proposta:** o spine não copia enums: referencia a spec §5 como dona única. Corrigir as três listas agora.

### M6 — AD-12 é um mapa só para duas MCUs

- **Par:** firmware LP/ACT da E3 (canal, perfil, reset remoto) × gateway.
- **Cenário:** AD-12 mistura o mapa do ATmega328P (nós: 512–515 intervalo) e do ESP8266 (521+ `M360DeviceConfig`). Os bytes 516–520 de "`M360NodeConfig`" no nó não têm dono por byte. Duas equipes (LP `SET_RADIO`, ACT reset remoto) escolhem o mesmo byte em PRs paralelos, ambas "documentando a faixa aqui". Não está definido o que cada reset de nó apaga.
- **Regra proposta:** um mapa por MCU, com dono e CRC por faixa; reservar já a faixa do canal no nó; definir o que cada tipo de reset apaga.

### M7 — Checklist de comissionamento aciona relés numa instalação viva

- **Par:** serviço de instalações (C6, C7, C9) × posse (AD-6) e o piloto já em produção.
- **Cenário:** para levar `DF/0000` a `ATIVA` no backend, C6/C7 ligam e desligam 31–39 e C9 deixa a solenóide ligada 600 s, enquanto o Node-RED é dono da irrigação. Ou o piloto é semeado direto como `ATIVA` sem checklist, e a regra "regras só em `ATIVA`" passa a valer de modo diferente em cada ambiente.
- **Regra proposta:** item do checklist que aciona atuador só roda com posse do backend sobre o grupo; o piloto entra como `ATIVA` por migração, com dispensa registrada dos itens já provados em produção.

### M8 — Escopo por instalação incompleto

- **Par:** notificador/configuração × AD-13 ("toda entidade é escopada por instalação").
- **Cenário:** `TELEGRAM_CHAT_ID` é variável de ambiente global; o "chat administrativo" recebe alertas de todas as instalações; `read_only` é chave global do YAML; `user_account`/`api_token` não têm instalação. Uma equipe manda alertas de `GO/0001` ao chat do dono de `DF/0000` e cumpre a spec.
- **Regra proposta:** separar o que é **da plataforma** (administrador, chat de operação, credencial de serviço) do que é **da instalação** (chat principal, assinantes, posse, somente leitura); nenhuma notificação de instalação vai a destino de plataforma, exceto as de provisionamento.

### M9 — Variante de placa por instalação não registrada

- **Par:** inventário (lista Nano e ACT para o Nó 99) × comissionamento C3/C4 e teste de contrato.
- **Cenário:** o contrato do Nó 99 depende da variante (11/12 só no Nano). Nada registra qual env está em campo; o teste de contrato não sabe contra qual comparar, e o gráfico "Clima (Nó 99)" fica vazio sem explicação.
- **Regra proposta:** o nó da instalação registra o env/variante (inferido do `sketch_name` ou cadastrado); o contrato é por env.

### M10 — Contador de volume com dois integradores

- **Par:** integrador do backend (RF-VOL-01, ativo desde a fase 1) × contador do Node-RED (`flow_vol_node_99_21`, migrado uma vez).
- **Cenário:** o backend integra desde a fase 1; a migração única copia o valor do Node-RED por cima (salto) ou não copia (dois totais diferentes nos relatórios). Não há dono definido do contador durante a convivência.
- **Regra proposta:** o contador oficial é do backend desde a fase 1; o valor do Node-RED entra como **deslocamento inicial** datado, nunca como sobrescrita.

### M11 — Margem de 15 s sem instante de referência; carimbo de mensagem enfileirada

- **Par:** despachante/motor (deadline da run) × firmware (prazo a partir da recepção) × broker com sessão persistente (RNF-10).
- **Cenário:** uma equipe conta o deadline a partir de `created_at`, outra de `confirmed_at`; com o eco perdido e confirmado no 3º retry (35 s), o relé está ligado desde a 1ª tentativa. Mensagens enfileiradas na sessão persistente chegam em rajada após reconexão e recebem `received_at` errado (AD-17 manda carimbar na recepção), o que distorce o `dt` do volume e a cadência dos nós.
- **Regra proposta:** o deadline conta do **primeiro publish** do ON; leituras entregues de fila após reconexão são marcadas (`late`) e excluídas da cadência e da integração de volume.

### M12 — AD-14 só isola LoRa; redes nRF24 sem regra

- **Par:** gateway protótipo Horta (`d1_mini_gateway`, nRF24 canal 76) × gateway Kit Hélio (nRF24 canal 76) e demais protótipos em campo.
- **Cenário:** AD-15 fixa protótipos e Kit Hélio em nRF24; o CLAUDE.md proíbe mudar o canal 76 sem medição. Duas redes nRF24 ao alcance uma da outra compartilham canal e endereço de base; o nó 11 do Kit Hélio pode ser confirmado pelo gateway da Horta. Nenhum AD trata isso.
- **Regra proposta:** AD-14 declara o escopo: redes nRF24 não têm isolamento, logo nunca operam ao alcance uma da outra (registrado na instalação e verificado no C5), ou recebem `MY_RF24_BASE_RADIO_ID` distinto por rede.

---

## BAIXA

### B1 — Faixa semver da MySensors

- **Par:** tabela Stack (`MySensors ^2.3.2`) × premissas da spec verificadas no código da 2.3.2 (broadcast de reapresentação que não funciona, tratamento de internas, `getBool()`).
- **Cenário:** um build limpo resolve outra versão compatível pela faixa, com comportamento diferente; as armadilhas documentadas deixam de valer sem aviso. A versão do Node-RED também não é fixada.
- **Regra proposta:** fixar a versão exata (`2.3.2`) e exigir revalidação das armadilhas ao subir.

---

## Emendas propostas por AD

| AD | Emenda |
|---|---|
| AD-1 | Incluir `kind`, failsafe, grupo de exclusão, janela de despertar, intervalo de nó-fonte e variante de placa; teste de contrato contra o banco (A5, A6, A7, A9, M9) |
| AD-2 | Limite sobre tempo contínuo ligado do child; ON repetido não rearma; sem ON em child já ligado; intervalo mínimo entre runs (C5) |
| AD-3 | Estender a todo comando com efeito persistente; comandos externos participam da correlação (A8, M2) |
| AD-4 | Flags de publicação (retain proibido em `in/`, QoS); esquemas JSON versionados; vetores de validação comuns (C3, M3, M4) |
| AD-5 | Só o despachante publica; agendador e reconciliador criam comandos (A11) |
| AD-6 | Unidade de posse = grupo físico; posse como fato compartilhado; troca só com grupo quiescente ou com entrega de estado; detecção de escritor estranho (C2, A1, A2, M7) |
| AD-7 | Validade (`expires_at`) em toda ação; ON vencido nunca executa; todo ON com failsafe nasce com OFF; nenhum estado bloqueia OFF (A3, A4, A10) |
| AD-12 | Um mapa por MCU, dono por byte, semântica de reset (M6) |
| AD-13 | Padrão de fábrica nunca é prefixo válido; identidade física do gateway vinculada à instalação; separar escopo de plataforma e de instalação (C1, M8) |
| AD-14 | Canal de fábrica reservado; `SET_RADIO` só com presença física; um dono do canal; troca transacional com fallback; escopo nRF24 explícito (C4, A8, M12) |
| Convenções | Enums referenciados da spec, não copiados; um dono do status de nó (M1, M5) |
| Stack | Versão exata da MySensors (B1) |

## Pendências para a spec do backend (não editadas aqui)

- §10, fase 3: divide `mux99` entre Node-RED e backend (C2).
- §5.4: "ações vencidas executadas imediatamente" sem distinguir ON de OFF (A3).
- §5.3 / RF-CMD-07: administrativos terminam em `ENVIADO` (A8).
- §10, migração de dados: sem `last_irrigation_at` nem runs abertas (A1).
- RF-PRV-06: "comandos recusados" em `SUSPENSA` sem exceção para OFF (A10).
- RF-PRV-09: o boot seguinte ao factory reset usa os padrões de compilação, que hoje são o prefixo do piloto (C1).
