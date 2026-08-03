# Product Requirements Document (PRD) — M360 Horta Inteligente

## 1. Visão do Produto e Objetivos
O projeto **M360 Horta** visa criar um ecossistema de automação inteligente e telemetria para gestão hídrica, controle preditivo e monitoramento da saúde de cultivos.
Ao integrar sensores avançados e atuadores de grande porte (bombas e válvulas), a Horta elimina o desperdício de água, maximiza a saúde do solo e previne perdas sazonais, entregando automação descentralizada através de redes de rádio (MySensors) sem depender puramente de conectividade em nuvem (operação local forte).

**Principais Problemas Resolvidos:**
- Desperdício hídrico por irrigação no escuro.
- Necessidade de ação manual em ligar/desligar bombas.
- Falta de visibilidade de métricas críticas (clima, umidade pontual, nível de luminosidade).

## 2. Arquitetura Conceitual do Ecossistema

O M360 Horta baseia-se num paradigma **DRY (Don't Repeat Yourself)** para o parque de hardware (nodes) e Gateway.
- **Biblioteca M360-DRY (`lib/M360-DRY`)**: Motor central da inteligência de borda. `M360Node` gerencia transporte, sono, bateria e comandos; o diretório `shared` permanece apenas como legado.
- **Master Gateway (`src/DRY/gateway`)**: Cérebro central que ponteia o mundo do rádio MySensors e o mundo IP TCP/MQTT, rodando regras unificadas de envio de estados, detecção JSON padrão e tratamento de pacotes perdidos.

### A "Frota" de Dispositivos (Atores Físicos e Nomenclatura de IDs)
A frota de dispositivos obedece às faixas padronizadas de **Node ID** (1 a 254) e **Child ID** (0 a 40+):
- **Faixas de Node ID:** `0` (Gateway), `1–50` (Estações Clima), `51–150` (Solo/Talhões), `151–200` (Atuação/Irrigação), `201–254` (Nós Especiais/Reservatórios/Água).
- **Faixas de Child ID:** `0` (Bateria/Status), `1–10` (Solo), `11–20` (Clima/Ambiente), `21–30` (Fluxo/Hidrometria), `31–40` (Atuadores/Relés).

1. **Actuators (Atuadores - Perfil ALWAYS_ON - IDs 151 a 200)**:
   - `nodePump`: Controla e supervisiona a bomba d'água principal. Exige tempos máximos de segurança (timeout) para evitar queima a seco.
   - `nodeSelenoieVazao`: Coordena a vazão fina de setores, medindo o fluxo (Flow Meter) enquanto a via está aberta.
2. **Monitors (Sensores - Perfil LOW_POWER - IDs 1 a 150 e 201 a 254)**:
   - `nodeDHT11`: Clima do ar (Temperatura/Umidade Relativa).
   - `nodeUmidadeTemperatura`: Perfil misto para checagem base.
   - `nodeUmidadeTemperatura2`: Variante dedicada de telemetria base de solo/temperatura, com a exclusão otimizada do sensor aéreo (sem DHT11).
   - `nodeZTS_UmidadeHall`: Nó de alta complexidade. Utiliza comunicação RS485/Modbus para extrair 7 parâmetros do solo (Umidade, Temp, Condutividade, pH, N, P, K). Integra sensor Hall local e LDR para luminosidade.
   - `main_lumi_10`: Controle e captura de índices de luz.

## 3. Requisitos Funcionais de Domínio

1. **Gestão de Energia:** Sensores periféricos operam sob baterias recarregáveis/solos com janelas estritas de operação (apenas 3000ms acordados via `MIN_AWAKE_TIME_MS`) para maximizar sobrevida a campo.
2. **Orquestração Sub-rede:** O gateway age com Heartbeats a cada 60s, roteando comandos de infraestrutura como `SET_INTERVAL` ou `FORCE_UPDATE`. Nenhuma regra restrita deve trancar a rede.
3. **Pulsar Sensores Analógicos e Controle de Carga 12V:** O solo e resistores (ZTS) devem receber voltagem apenas via `VCC_SENSORS` ou Relés dedicados (no caso do ZTS que exige 12V) por ~500ms durante a leitura. Isso mitiga eletrólise e degradação física. O controle de relés para válvulas e sensores pesados utiliza lógica *Active LOW*.
4. **Precisão da Escala e Comunicação Industrial:** Dados do solo fluem num formato racional `0 a 100%`. O sistema suporta integração com protocolo Modbus RTU sobre RS485 para sensores profissionais de agricultura de precisão.
5. **Autodescoberta (Auto-Discovery) e Registro Dinâmico:** O backend e o Proxy MCP interceptam mensagens de boot (`C_PRESENTATION` e `C_INTERNAL`) nos tópicos MySensors (`m360/DF/0000/out/#`), construindo dinamicamente o mapa completo de nós/childs e vinculando ao modelo MCP v2.0 sem cadastro manual.
6. **Diagnóstico e Failsafe de Infraestrutura:** O Motor de Inferência v2.0 monitora o nível de bateria (`bateria_porcentagem`) e RSSI de cada nó. Se a bateria atingir nível crítico (< 15%) ou se o nó ficar offline, o Proxy MCP invalida temporariamente as regras que dependem do nó para impedir acionamentos indevidos.

## 4. Requisitos Não Funcionais
- **Padronização Código Base**: Os desenvolvedores não farão implementações customizadas do ciclo de vida NRF. Novos nós usam `M360Node` e buffers `NODE_ITEMS_COUNT + 2`.
- **Topologia Resiliente JSON**: Toda carga útil de rádio que atinge a nuvem (MQTT) se converte ativamente em pacotes JSON robustos para o Broker. Tamanho máximo do documento é `512 bytes`.
- **Prevenção de Dados Estagnados**: Através da implementação de `FORCE_UPDATE_N_READS` (10 iterações normais), o sistema jamais permitirá congelamento de status visuais da horta.

