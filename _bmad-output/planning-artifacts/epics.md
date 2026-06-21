---
stepsCompleted: ["engenharia-reversa-autonoma"]
inputDocuments: ["docs/prd.md", "docs/epics.md", "docs/architecture.md", "codebase"]
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

---
*Artefato finalizado via engenharia reversa e análise de codebase.*
