---
stepsCompleted: ["engenharia-reversa-autonoma"]
inputDocuments: ["docs/architecture.md", "codebase"]
inputDocumentsRemovidos: "docs/prd.md e docs/epics.md também foram entrada deste artefato. Removidos em 28/08/2026 (commit b612c0c) por serem cópias divergentes e defasadas destes mesmos arquivos — descreviam nodePump, nodeSelenoieVazao, nodeZTS_UmidadeHall, main_lumi_10 e o diretório ngm/, todos inexistentes, e davam a escala de solo como 0-100% quando é ADC bruto 0-1023. Conteúdo recuperável no histórico do Git."
workflowType: 'epics-and-stories'
---

# Epics & Story Backlog — M360 Horta
**Autor:** John (BMAD PM Agent)
**Data:** 2026-06-21

Este documento lista os épicos e histórias do projeto M360 Horta, gerados a partir da estrutura tecnológica implementada na base de código. Representa os "clusters de valor" que o projeto entrega, organizando as funcionalidades de campo e de orquestração.

---

## Epic 1: [Core Infrastructure] O Motor Compartilhado (DRY Node Engine)
**Descrição:** O alicerce que padroniza o ciclo de vida dos nós e a mensageria do sistema, eliminando a repetição de lógicas base. Cobre o Gateway e a biblioteca `M360-DRY`.

- **Story 1.1:** Motor Core M360-DRY (`M360Node`)
  - **Contexto:** Nós precisam de setup, gerenciamento de tempo de escuta e processamento seguro de mensagens MySensors em rotinas simplificadas.
  - **Implementação Base:** `lib/M360-DRY/src/M360Node.h`
  - **Cenário de Aceitação:** Dado um nó inicializado com `M360Node`, Quando for o momento de sua apresentação, Então ele deve enviar os metadados de Node ID (254) e Bateria (255) automaticamente, liberando a função `presentation()` do nó apenas para sensores específicos.

- **Story 1.2:** Gerenciamento Adaptativo de Alimentação
  - **Contexto:** Nós sensores devem conservar bateria, enquanto atuadores precisam estar sempre atentos à rede.
  - **Implementação Base:** `src/DRY/nos/shared/powerProfile.h`
  - **Cenário de Aceitação:** Dado um nó em `LOW_POWER`, Quando finalizar a leitura dos sensores, Então ele deve forçar o estado de Deep Sleep (`sleep()`) usando seu intervalo configurado, medindo a tensão da bateria se o ciclo `PROCESS_BATTERY` bater a quota estipulada.

- **Story 1.3:** Gateway de Conversão e Roteamento
  - **Contexto:** Transformar o ecossistema fechado (rádio MySensors) em uma infraestrutura global consumível por sistemas modernos via IP e MQTT.
  - **Implementação Base:** `src/DRY/gateway/newGatewayMqtt.cpp` / `libDryGatewayMqtt.cpp`
  - **Cenário de Aceitação:** Dado um payload de sensor na malha local, Quando o gateway recebê-lo e estiver online via WiFi, Então deve converter para JSON (tamanho máximo 512 bytes) e publicar no tópico específico `m360/{uf}/{carNumber}/out`.

- **Story 1.4:** Rastreamento de Nós e Detecção de Inatividade (`M360Registry`)
  - **Contexto:** O gateway precisa saber quais nós estão vivos sem exigir heartbeat dedicado, e um limiar fixo não serve: o motor do nó só transmite um sensor quando o valor muda ou a cada 10 ciclos, então o silêncio legítimo de um nó `ALWAYS_ON` chega a ~11 × o intervalo declarado.
  - **Implementação Base:** `lib/M360-DRY/src/M360Registry.{h,cpp}`
  - **Cenário de Aceitação:** Dado um nó já conhecido e ativo, Quando chegar uma mensagem dele após um intervalo maior que 15 s, Então o registro deve atualizar a cadência observada por média móvel (0,7 anterior / 0,3 amostra) e recalcular o limiar como `max(intervalo declarado, cadência observada) + max(2 min, 50 % da base)`, limitado a 2 h; e Quando o nó estiver inativo, Então o intervalo desde a última mensagem **não** deve alimentar a cadência, por ser tempo de queda e não ritmo de reporte.
  - **Cenário de Aceitação (perfil sem sleep):** Dado um nó cujo sketch name traz o sufixo `[ON]` ou `[REP]`, Quando o limiar for calculado, Então a base deve ser o intervalo declarado multiplicado por `STALE_FORCE_CYCLES` (10), que é o pior caso de silêncio legítimo imposto por `M360Node::_readAndSendAll()`.
  - **Cenário de Aceitação (descoberta de perfil):** Dado que o gateway reiniciou e não conhece o sketch name de um nó, Quando chegar qualquer mensagem desse nó, Então o gateway deve enviar `I_PRESENTATION` (`C_INTERNAL`, tipo 19) para que o nó execute `presentNode()`, repetindo o pedido a cada 5 min enquanto o nome faltar — o primeiro envio pode receber NACK.
  - **NFR associado:** o mesmo cálculo vive no `Mapeia nós` do Node-RED — são dois relógios independentes sobre os mesmos dados, e divergir faz o nó aparecer ONLINE de um lado e perdido do outro. Ver `src/DRY/horta/nodered/funcionalidades_nodered.md` §9.

---

## Epic 2: [Atores Físicos] Controle Hídrico e Fluxo (Atuadores)
**Descrição:** Os nós operacionais que modulam e executam as tarefas de alto impacto: ligar/desligar a passagem de fluidos baseando-se em comandos vindos da rede ou timeouts internos de segurança.

- **Story 2.1:** Acionamento e Segurança de Bomba Principal
  - **Contexto:** Gerenciar a bomba-mestre do sistema com proteção contra trabalho a seco, necessitando escuta de comandos contínua (`ALWAYS_ON`).
  - **Implementação Base:** `src/DRY/nos/nodePump/`
  - **Cenário de Aceitação:** Dado um comando para acionar a bomba (`V_STATUS = 1`), Quando decorrido o tempo de timeout de segurança configurado, Então a bomba deve desligar-se automaticamente independentemente de novas mensagens de rádio.

- **Story 2.2:** Gerenciamento Fino de Válvulas Setoriais e Litragem
  - **Contexto:** Abrir e fechar vias de água (solenoides) enquanto mede ativamente a vazão (sensores YF-S201).
  - **Implementação Base:** `src/DRY/nos/nodeSelenoieVazao/` / `src/DRY/nos/80nodeAqua/`
  - **Cenário de Aceitação:** Dado que uma solenoide está em estado de fluxo ativo, Quando os pulsos na interrupção `PCINT` baterem a marcação, Então o volume processado deverá ser publicado à rede sem falhas de contagem ocasionadas por bloqueios de código (`delay()`).

---

## Epic 3: [Sensores Field] Telemetria de Solo e Ambiente
**Descrição:** O sistema nervoso sensitivo espalhado pelo terreno (Perfil `LOW_POWER` estrito). Os nós são despertados em horários determinados, excitam os canais elétricos apenas durante a amostragem, emitem os dados e retornam ao sono profundo.

- **Story 3.1:** Sensoriamento Aéreo e Climático Rápido
  - **Contexto:** Coleta rápida da temperatura de ambiente e índice higrométrico do ar (DHT11/DHT22) com intuito de prever déficit hídrico aéreo.
  - **Implementação Base:** `src/DRY/nos/nodeDHT11/`
  - **Cenário de Aceitação:** Dado o ciclo de wakeup do nó, Quando ler o sensor DTH, Então ele deve despachar as flutuações truncadas a uma casa decimal e ignorar falhas de checksum com uma segunda tentativa.

- **Story 3.2:** Sensoriamento Básico de Solo 3D e Raízes
  - **Contexto:** Nós descentralizados medindo umidade por resistência local e/ou sensores de solo via resistores dedicados. Ex: DHT e Sondas de penetração A0/A1.
  - **Implementação Base:** `src/DRY/nos/01nodeSolo3d/`, `src/DRY/nos/nodeUmidadeTemperatura2/`
  - **Cenário de Aceitação:** Dado que a rotina `readNodeItem` for disparada, Quando mapeada para os Child IDs do solo, Então o pino de excitação (energia do sensor) deve ficar High apenas enquanto o analogRead consolida a leitura.

- **Story 3.3:** Sensoriamento Avançado Multi-parâmetro via Modbus RTU
  - **Contexto:** Para solos complexos, uso do ZTS para extração de perfis como NPK, Umidade e pH usando transceivers RS485 isolados, exigindo tempo de boot.
  - **Implementação Base:** `src/DRY/nos/13nodeZTS_UmidadeHall/`
  - **Cenário de Aceitação:** Dado que o ciclo entra no status de boot do sensor, Quando `powerUp()` rodar ativando o relé de 12V, Então deve aguardar 500ms através do método `wait()` (não-bloqueante à malha MySensors) antes de ler pela via de hardware Serial o protocolo Modbus.

---

## Epic 4: [Controle Nuvem] Comandos Administrativos e Resiliência
**Descrição:** Facilita as tarefas de provisionamento offline da horta, além de implementar ganchos cruciais via Web Server para configuração primária em áreas remotas.

- **Story 4.1:** Provisionamento Offline (Captive Portal Local)
  - **Contexto:** Nem todas as fazendas têm Wi-Fi padronizado. O Gateway deve permitir configuração simplificada de SSID e Servidor MQTT através do celular sem precisar de cabo.
  - **Implementação Base:** `src/DRY/gateway/ngm/webserver.cpp`
  - **Cenário de Aceitação:** Dado um Reset de Fábrica (Pino A0 aterrado) ou um EEPROM corrompido, Quando o Gateway bootar, Então deve forçar o modo AP, criar uma rede WiFi local de setup e exibir um portal web para inserir porta MQTT e credenciais da fazenda.

- **Story 4.2:** Roteamento de Comando Global (`FORCE_UPDATE` e `SET_INTERVAL`)
  - **Contexto:** Garantir que o admin possa ordenar atualizações síncronas de todos os nós a despeito dos loops que eles estejam programados.
  - **Implementação Base:** Gateway + `lib/M360-DRY/src/M360Node.cpp`
  - **Cenário de Aceitação:** Dado o recebimento do JSON `{"action": "CMD_FORCE_UPDATE"}` pela nuvem, Quando o Gateway decodificar, Então deve rotear a string apropriada `CMD_FORCE_UPDATE` no payload nativo à criança designada, desencadeando a sub-rotina de update bypass em M360Node.

## Epic 5: [Manutenção Remota] Atualização de Firmware OTA
**Descrição:** Elimina a necessidade de acesso físico com cabo USB para atualizar firmware, no gateway e nos nós. É pré-requisito da expansão para RFM95W/LoRa: alcance maior significa nós mais distantes, e sem OTA cada correção de regra embarcada passa a custar um deslocamento. Estudo de viabilidade com medições em `_bmad-output/planning-artifacts/research/ota-firmware-m360.md`.

- **Story 5.1:** OTA do Gateway ESP8266
  - **Contexto:** O gateway é o alvo sem pré-requisito — ~346 KB de folga sobre o exigido pelo OTA em 2 passos, e `ESP8266httpUpdate` / `ESP8266HTTPUpdateServer` já vêm no core. A infraestrutura precisa nascer em `lib/M360-DRY` porque os dois `libDryGatewayMqtt.cpp` (Horta e Kit Hélio) são cópias independentes que divergiriam na primeira correção.
  - **Implementação Base:** `lib/M360-DRY/src/M360OTA.{h,cpp}` (novo, sob `#ifdef ESP8266`), hook em `lib/M360-DRY/src/M360Gateway.cpp`, envs em `src/DRY/horta/platformio.ini` e `src/DRY/kit-helio/platformio.ini`
  - **Cenário de Aceitação:** Dado um gateway em modo STA com MQTT conectado, Quando chegar o comando de atualização com a URL do binário, Então ele deve baixar, validar e gravar a imagem, publicando `ota_start`, `ota_progress` e `ota_end` em `.../out/events` via `publishTransportEvent()`, e reiniciar na versão nova.
  - **Cenário de Aceitação (recuperação):** Dado um gateway caído em modo AP por config inválida, Quando o operador acessar a rota `/update` autenticada do portal, Então o upload manual do `.bin` deve funcionar — o hook de OTA roda fora do `if (!isAPMode())`, como já fazem `_webHandler` e `_ledUpdate`.
  - **NFR associado:** A rota `/update` exige autenticação por senha distinta das credenciais de AP e WiFi. O estado de OTA persistido usa os bytes de EEPROM 516–520; acrescentar campo a `M360DeviceConfig` invalida o CRC e derruba toda a frota para modo AP.

- **Story 5.2:** Passagem de `C_STREAM` no Gateway (transporte do FOTA)
  - **Contexto:** Mesmo com bootloader e memória de staging resolvidos nos nós, o FOTA não passa pelo gateway como ele está escrito: `Translator::validate()` e `Translator::fromNative()` aceitam apenas `C_SET`, `C_REQ` e `C_INTERNAL`, e o payload trafega como `const char*` — blocos binários de 16 B contêm `0x00` e não sobrevivem a um caminho de string.
  - **Implementação Base:** `lib/M360-DRY/src/M360Translator.cpp:95-153`, `src/DRY/horta/nodered/funcionalidades_nodered.md` §9
  - **Cenário de Aceitação:** Dado um bloco de firmware de 16 bytes contendo `0x00`, Quando trafegar pelo gateway em qualquer sentido, Então deve chegar íntegro ao destino, transportado em codificação hexadecimal no payload MQTT — padrão que `MyMessage.h:316-317` já prevê.
  - **Cenário de Aceitação (não-regressão):** Dado um comando `C_SET`/`V_STATUS` com payload diferente de `"0"` ou `"1"`, Quando o `Translator` validar, Então deve continuar rejeitando — a passagem de `C_STREAM` é um caminho separado e não afrouxa a validação de atuação, que existe porque `getBool()` faz `atoi()` e qualquer lixo desliga o relé.
  - **NFR associado:** Altera o contrato com o Node-RED. A §9 de `funcionalidades_nodered.md` e o `Decodificador Nativo` devem ser atualizados na mesma entrega — mudar um lado sem o outro quebra o sistema em silêncio.

- **Story 5.3:** Bootloader e Memória de Staging nos Nós `ALWAYS_ON`
  - **Contexto:** Todos os nós rodam o `ATmegaBOOT` legado de 2 KB e nenhuma placa tem memória não-volátil externa. A escolha é `DualOptiboot` + flash SPI, e não `MYSBootloader`: este último embute um driver nRF24 e só funciona com esse rádio, exigindo uma segunda visita com gravador ISP a cada nó na migração para RFM95/RFM69. O `DualOptiboot` nunca toca no transceptor e atravessa a migração — com ele, trocar de rádio vira plugar o adaptador no soquete.
  - **Implementação Base:** Bootloader gravado por ISP, `board_bootloader.*` e `MY_OTA_FIRMWARE_FEATURE` nos envs de `src/DRY/horta/platformio.ini` e `src/DRY/kit-helio/platformio.ini`, `esquema_eletrico.md` de cada nó
  - **Cenário de Aceitação:** Dado um nó com `DualOptiboot` gravado e flash SPI soldada, Quando o controlador anunciar uma versão diferente da que ele roda, Então o nó deve baixar a imagem bloco a bloco para a memória de staging, validar o CRC e reiniciar já na versão nova.
  - **NFR associado:** O orçamento de flash e RAM do Nó 99 (8.888 B e 547 B livres) deve ser medido com a feature ligada antes de qualquer compromisso — a RAM é o risco maior que a flash. Nos Nós 01 e 02 o caminho I2C está barrado: A4/A5 são canais de umidade de solo, logo o staging é obrigatoriamente SPI. `MY_OTA_FLASH_JDECID` deve casar com o chip usado — o padrão `0x1F65` não reconhece um W25Q80.
  - **Cenário de Aceitação (Pro Mini):** Dado um nó Pro Mini, Quando o bootloader for gravado por ISP pelos headers laterais (D11/D12/D13 + RST), Então o módulo de rádio deve ter sido desplugado do soquete antes da gravação, e o `upload_speed` do env deve ser atualizado para a velocidade do novo bootloader — os 57600 atuais são do ATmegaBOOT.
  - **NFR associado (Pro Mini):** Confirmar a placa física do Nó 04 antes de gravar. O código-fonte e o `esquema_eletrico.md` declaram 3.3V/8MHz, mas `platformio.ini:201` usa `board = pro16MHzatmega328`. Os fuses das duas placas são idênticos (0xFF/0xDA/0xFD), mas o binário do bootloader carrega o divisor de UART — gravar o errado quebra a via serial de recuperação em silêncio. Não desabilitar o BOD (`efuse = 0xFD`, 2,7 V): ele é o que impede brick por queda de tensão durante a cópia do staging no boot.

- **Story 5.4:** Janela Acordada para OTA em Nó `LOW_POWER`
  - **Contexto:** O Nó 04 é onde OTA mais vale a pena — é solar e tende a ficar mais distante — e é o único estruturalmente incapaz de recebê-lo: acorda ~3 s por hora, e uma campanha de 20 KB são 1.280 blocos com ida e volta cada.
  - **Implementação Base:** `lib/M360-DRY/src/M360Node.cpp:200-233` (dispatch de `V_CUSTOM`), `M360Node.cpp:108-143` (`process()`), `lib/M360-DRY/src/M360Constants.h:47-49`
  - **Cenário de Aceitação:** Dado um nó `M360_LOW_POWER` em sono, Quando receber o comando `V_CUSTOM` de abrir janela de OTA na sua janela de escuta, Então deve substituir o `smartSleep()` por `wait()` e permanecer acordado até concluir a campanha.
  - **Cenário de Aceitação (failsafe):** Dado que a campanha falhou ou o controlador sumiu, Quando o prazo máximo absoluto da janela expirar, Então o nó deve voltar a dormir sozinho, sem depender de nenhuma mensagem — a mesma lógica do failsafe de tempo máximo ligado do Nó 99.
  - **NFR associado:** Não iniciar campanha abaixo de um limiar de tensão de bateria, já reportada pelo child 255.

- **Story 5.5:** Servidor de Firmware no Controlador — Requisito
  - **Contexto:** Story de requisito, não de implementação. FOTA não funciona só com firmware: alguém precisa indexar firmwares por `type`/`version`, fazer o parse do Intel HEX do PlatformIO e servir blocos de 16 B respondendo a `ST_FIRMWARE_CONFIG_REQUEST` e `ST_FIRMWARE_REQUEST`. Hoje não existe nenhum `ST_FIRMWARE_*` no `flows.json`.
  - **Implementação Base:** A definir — aba dedicada em `src/DRY/horta/nodered/flows.json` ou MYSController externo
  - **Cenário de Aceitação:** Dado o PoC da Story 5.3, Quando a pilha for validada, Então a decisão entre aba Node-RED e MYSController deve estar registrada com a estimativa de esforço de cada uma — a recomendação do estudo é usar MYSController no PoC, para isolar a validação de bootloader, staging e rádio do risco de software de controlador.
  - **NFR associado:** A latência de resposta do servidor precisa caber na janela de desistência do nó: `(MY_OTA_RETRY + 1) x MY_OTA_RETRY_DELAY` = 3 s. Em RFM95 com spreading factor alto, o tempo de ar sozinho já estoura essa janela e `MY_OTA_RETRY_DELAY` terá de ser elevado por env.

---
---
*Artefato finalizado via engenharia reversa e análise de codebase.*
