documentação completa e detalhada do funcionamento do Gateway MySensors MQTT (Modular).

Documentação: Gateway MySensors MQTT (Modular)
Este projeto implementa um gateway avançado para a rede MySensors, utilizando um ESP8266 (NodeMCU/D1 Mini) como controlador central e um rádio NRF24L01 para comunicação com os nós campo. Ao contrário dos exemplos padrão, este gateway utiliza uma arquitetura modularizada para facilitar a manutenção e a robustez.

1. Arquitetura Modular (ngm/)
O projeto está organizado na pasta src/DRY/gateway/ngm/ (provavelmente "Next Generation MQTT"), dividindo as responsabilidades em módulos:

Módulo	Responsabilidade
config_utils	Gestão de memória EEPROM (offset 512), cálculo de CRC e estrutura do DeviceConfig.
wifi_utils	Gerenciamento da conexão WiFi (STA) e do ponto de acesso de configuração (AP).
mqtt_utils	Lógica de conexão com o broker, tradução de mensagens MySensors/JSON e métricas.
webserver	Interface web para configuração remota do dispositivo.
leds	Controle visual de status através de LEDs RGB ou independentes.
2. Fluxo de Operação e Inicialização
O ciclo de vida do gateway é dividido em três estágios principais:

A. Estágio before() (Pré-inicialização)
Este estágio ocorre antes do rádio MySensors ser ativado:

Carga de Configuração: Lê a estrutura DeviceConfig da EEPROM (posição 512+).
Validação: Verifica se a configuração é válida (versão + CRC).
Seleção de Modo:
Modo AP (Configuração): Ativado se a config for inválida ou se o pino A0 for aterrado (GND) no boot. Cria o WiFi Manejo360-Config.
Modo STA (Operação): Ativa o WiFi para conectar-se ao roteador configurado.
B. Estágio setup() & presentation()
Servidor Web: Inicia o servidor na porta 80 para permitir reconfiguração via navegador.
Conexão MQTT: O módulo mqtt_utils inicia a conexão com o broker usando as credenciais da EEPROM.
Identificação MySensors: O gateway se registra na rede como Node 0 ("Manejo360 Gateway MQTT 2.0").
C. Estágio loop() (Execução Contínua)
O loop principal executa tarefas não bloqueantes:

handleWiFiReconnect() e handleMQTTReconnect() mantêm a conexão viva.
wait(1) processa pacotes do rádio MySensors com baixa latência.
server.handleClient() atende requisições web.
checkNodeTimeouts() verifica se algum nó parou de responder.
3. Lógica de Comunicação e Tradução
O gateway atua como um tradutor bidirecional:

Upstream (Rede Rádio → Broker MQTT)
A função receive() captura o pacote MySensors do rádio.
A função sendMQTT() converte o pacote para um envelope JSON.
O JSON é publicado no tópico: m360/{UF}/{CAR}/out.
Exemplo de Payload: {"nodeId":5, "sensorId":1, "command":1, "type":0, "payload":"25.5"}
Downstream (Broker MQTT → Rede Rádio)
O gateway assina o tópico: m360/{UF}/{CAR}/in.
Ao receber um comando via mqttCallback, ele tenta parsear o JSON.
Formato MySensors: Se o JSON contiver nodeId, sensorId, type, etc., ele envia o comando bruto via rádio.
Formato Simplificado: O gateway aceita ações como "action": "PUMP_ON", mapeando-as internamente para comandos V_STATUS ou V_CUSTOM conforme a necessidade do projeto M360.
4. Monitoramento e Manutenção
Rastreamento de Nós (Node Tracking)
O gateway mantém um registro de até 10 nós (nodeRegistry).

Se um nó envia mensagem, ele é marcado como active.
Se um nó não se comunica por 5 minutos (NODE_TIMEOUT_MS), o gateway publica um evento "node_lost" no tópico MQTT de eventos para alertar o backend.
Diagnóstico por LEDs
Estado Visual	Significado
Verde Aceso	Sistema Online (WiFi + MQTT OK).
Yellow Piscando	WiFi OK, mas Broker MQTT inacessível.
Red Piscando	WiFi desconectado (Tentando reconectar).
Flash Verde	Recebendo mensagem do rádio MySensors.
Flash Amarelo	Enviando dados para o Broker ou Heartbeat.
Heartbeat Diário/Periódico
A cada 60 segundos, o gateway publica um pacote de telemetria contendo:

Uptime (tempo ligado).
Força do sinal WiFi (RSSI).
Endereço IP Local.
Versão do Firmware.
5. Detalhes Técnicos da EEPROM
A memória é segmentada para evitar conflitos:

0 - 511: Reservado para a biblioteca MySensors (Node IDs, routing info, etc.).
512+: Configurações customizadas do Manejo360 (SSID, Password, MQTT Server, UF, CAR).
Para realizar um Factory Reset, o pino A0 deve ser mantido em GND por 3 segundos durante os primeiros 30 segundos após o ligamento. Isso limpa a EEPROM e retorna o gateway ao Modo AP de fábrica.