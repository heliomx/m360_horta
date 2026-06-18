# Epics & Story Backlog — M360 Horta

Este arquivo mapeia os grandes épicos (clusters de valor) e suas Histórias de Usuário, deduzidos a partir do arcabouço tecnológico do projeto M360 Horta.

---

## Epic 1: [Core Infrastructure] O Motor Compartilhado (DRY Node Engine)
**Descrição:** O alicerce MySensors para eliminar duplicação de lógicas de repouso, telemetria e rádio em nós IoT.
**Pasta Base:** `src/DRY/nos/shared/` e `src/DRY/gateway/`
- **Story 1.1:** Motor Core e Macros Inteligentes (`node_engine`)
  - *Contexto:* Configuração base de pinos, setup, heartbeat e leitura segura do payload base para radio RF24.
  - *Implementação:* `src/DRY/nos/shared/node_engine.h/.cpp`
  - *Cenário 1 (Identificação):* **Dado** um novo nó configurado com `NODE_ENGINE_PRESENTATION`, **Quando** ele é ligado, **Então** ele deve apresentar automaticamente seu Nome e Versão ao Gateway e registrar os Child IDs reservados (254, 255).
- **Story 1.2:** Gerenciamento de Alimentação Variável
  - *Contexto:* Definir e processar os perfis LOW_POWER (bateria) e ALWAYS_ON (tomada), além do cálculo e leitura (`NODE_ENGINE_PROCESS_BATTERY()`).
  - *Implementação:* `src/DRY/nos/shared/powerProfile.h/.cpp`
  - *Cenário 1 (Deep Sleep):* **Dado** o perfil `LOW_POWER`, **Quando** o ciclo de leitura termina, **Então** o nó deve processar a carga da bateria e entrar em modo `sleep()` pelo tempo definido em `updateInterval`.
- **Story 1.3:** Padrão de Gateway e MQTT (`ngm/`)
  - *Contexto:* Transformação dos estados de child node para o Broker, conversão string, e reconexão autônoma no Gateway.
  - *Implementação:* `src/DRY/gateway/` e `src/DRY/gateway/ngm/`
  - *Cenário 1 (Tradução):* **Dado** um comando recebido via rádio do Nó 5, **Quando** o Gateway processa, **Então** ele deve publicar um JSON no tópico `m360/{UF}/{CAR}/out` com o formato padrão.

---

## Epic 2: [Atores Físicos] Controle Hídrico e Fluxo (Atuadores)
**Descrição:** Os nós operacionais que modulam e executam as tarefas pesadas de bombeamento.
**Pasta Base:** `src/DRY/nos/`
- **Story 2.1:** Acionamento de Bomba Principal (`nodePump`)
  - *Contexto:* Através de comandos "PUMP_ON/OFF" e falhas de segurança (`timeout`), o nó responde pelo coração hídrico. Necessita perfil `ALWAYS_ON`.
  - *Implementação:* `src/DRY/nos/nodePump/`
  - *Cenário 1 (Comando remoto):* **Dado** o recebimento de `V_STATUS` = 1 para o Child ID da bomba, **Quando** processado pelo `NODE_ENGINE_HANDLE_ACTUATORS`, **Então** o pino do relé deve ser acionado e o estado retornado via rádio.
- **Story 2.2:** Gerenciamento de Válvulas e Litragem (`nodeSelenoieVazao`)
  - *Contexto:* Solenoides de setor aliadas a contagem do medidor de fluxo (YF-S201), calculando volume exato derramado naquela área através de interrupções e cálculo de pulsos.
  - *Implementação:* `src/DRY/nos/nodeSelenoieVazao/`
  - *Cenário 1 (Medição):* **Dado** a válvula em estado ON, **Quando** houver pulsos no sensor de fluxo, **Então** o valor de volume acumulado deve ser incrementado e reportado em litros (V_VOLUME).

---

## Epic 3: [Sensores Field] Telemetria de Solo e Ambiente
**Descrição:** O sistema nervoso sensitivo espalhado pelo terreno (Perfil `LOW_POWER` estrito).
**Pasta Base:** `src/DRY/nos/`
- **Story 3.1:** Sensoreamento Aéreo (`nodeDHT11`)
  - *Contexto:* Umidade de ar e temperatura padrão para prever calor em excesso e acionar a lógica do core.
  - *Implementação:* `src/DRY/nos/nodeDHT11/`
  - *Cenário 1 (Leitura):* **Dado** o ciclo de leitura ativo, **Quando** o sensor DHT11 é consultado, **Então** os valores de temp/hum devem ser enviados com 1 casa decimal de precisão.
- **Story 3.2:** Sensoreamento Avançado de Substrato (`nodeZTS_UmidadeHall` / `nodeUmidadeTemperatura` / `nodeUmidadeTemperatura2`)
  - *Contexto:* Resistividade profunda do solo e temperatura da raiz. Inclui o nó `nodeUmidadeTemperatura2` (sem DHT). 
  - *Implementação:* `src/DRY/nos/nodeZTS_UmidadeHall/`, `src/DRY/nos/nodeUmidadeTemperatura/`, `src/DRY/nos/nodeUmidadeTemperatura2/`
  - *Cenário 1 (Multi-parâmetro ZTS):* **Dado** o início da leitura, **Quando** o relé de 12V é ativado, **Então** o driver RS485 deve ler todos os 7 registros Modbus (NPK, PH, etc) e enviar os dados nos Child IDs correspondentes antes de desligar o relé.
- **Story 3.3:** Percepção Climática Solar (`main_lumi_10`)
  - *Contexto:* Controle preciso de incidência de UV/Luz e lux medidos contínua ou alternadamente para regras de restrição à ensolação cruzada na irrigação.
  - *Implementação:* `src/DRY/nos/main_lumi_10.cpp`
  - *Cenário 1 (Luminosidade):* **Dado** o sensor LDR/Luz, **Quando** a leitura é solicitada, **Então** o nível de iluminação deve ser escalonado e reportado para o gateway.

---

## Epic 4: [Integração Nuvem] Mapeamento Digital e Controle App
**Descrição:** Consolidação final dos dados com a nuvem utilizando os nós mapeados.
**Pasta Base:** `src/DRY/gateway/`
- **Story 4.1:** Serialização e Transporte MQTT Resiliente
  - *Contexto:* Garantir JSONs (≤ 512b) na porta serial TCP do ESP/Gateway contendo `nodeId`, `sensorId` e `batteryLevel`.
  - *Implementação:* `src/DRY/gateway/ngm/mqtt_utils.cpp`
  - *Cenário 1 (Encapsulamento):* **Dado** a recepção de dados via rádio, **Quando** o objeto JSON é criado, **Então** o tamanho final não deve exceder 512 bytes para garantir compatibilidade com o buffer MQTT.
- **Story 4.2:** Roteamento Reverso (App → Horta)
  - *Contexto:* Traduzir os payloads simplificados de Nuvem (ex: `{"nodeId": 5, "action": "SET_INTERVAL", "interval": 5}`) em requisições genuínas tratadas pelo `NODE_ENGINE_HANDLE_INTERVAL(msg)` em cada dispositivo da horta.
  - *Implementação:* `src/DRY/gateway/newGatewayMqtt.cpp`
  - *Cenário 1 (Comando Simplificado):* **Dado** uma "Action" recebida via MQTT, **Quando** for `FORCE_UPDATE`, **Então** o Gateway deve disparar um comando `V_VAR1` = `CMD_FORCE_UPDATE` para o rádio.
