# Technical Architecture (Solution Design) — M360 Horta

## 1. Visão Geral do Sistema
O sistema M360 Horta utiliza uma arquitetura em camadas baseada no protocolo **MySensors** para a rede de rádio e **MQTT/JSON** para a camada de integração e controle. O diferencial técnico reside na aplicação estrita do princípio **DRY** no firmware dos nós e do gateway.

## 2. Pilha Tecnológica
- **MCU (Nodes):** ATMega328P / ESP8266 (dependendo do nó).
- **MCU (Gateway):** ESP8266 / ESP32.
- **Transporte de Rádio:** NRF24L01+ (2.4GHz).
- **Protocolos de Campo:** RS485 / Modbus RTU (Sensores Industriais).
- **Protocolo de Integração:** MQTT via WiFi.
- **Formato de Dados:** JSON (Envelope M360 padrão).

## 3. Padrões de Projeto (Firmware DRY)

### 3.1 Biblioteca M360-DRY
A arquitetura ativa concentra o ciclo de vida em `lib/M360-DRY`:
- **`M360Node`**: apresenta itens, processa leituras, comandos, bateria e perfis de energia.
- **`M360Gateway`**: orquestra WiFi, MQTT, webserver, heartbeat e registro de nós.
- **`M360Translator`**: implementa o contrato JSON/MySensors bidirecional.

O diretório `src/DRY/nos/shared/node_engine.*` é legado e não deve ser usado em novos nós.

### 3.2 Isolamento de Hardware (`sensorDrivers`)
Cada nó separa a lógica de aplicação da implementação física dos drivers:
- **`readNodeItem(index)`**: Interface comum para o engine coletar dados sem conhecer o hardware.
- **`writeNodeItem(childId, state)`**: Interface comum para atuação.

## 4. Gestão de Energia e Proteção de Sensores

### 4.1 Ciclo de Vida do Sono (Deep Sleep)
Nós de bateria seguem a sequência:
1. `M360::powerUp()` (ativa pinos de alimentação).
2. Leitura de Sensores.
3. Transmissão.
4. Janela de Escuta (`MIN_AWAKE_TIME_MS`).
5. `M360::powerDown()` -> `smartSleep()`.

### 4.2 Alimentação Pulsada (`VCC_SENSORS`)
Para evitar a degradação galvânica dos sensores de solo, a alimentação só é ligada durante o milissegundo da leitura, sendo desligada imediatamente após. Para periféricos de 12V (como o ZTS), utiliza-se o chaveamento via Relé.

## 5. Gateway e Mensageria

### 5.1 Transformação Serial-MQTT
O gateway atua como um tradutor transparente:
- **Radio -> MQTT:** Converte o binário MySensors em um JSON estruturado de 512 bytes.
- **MQTT -> Radio:** Aceita comandos simplificados (Actions) ou frames MySensors completos.

### 5.2 Resiliência
- **Heartbeat:** Verificação de saúde a cada 60s.
- **Retry Logic:** O gateway gerencia as tentativas de reconexão WiFi e MQTT sem bloquear o tráfego do rádio.

## 6. Segurança e Manutenção
- **Reset de Fábrica:** Detecção de hardware via pino `A0` (GND) para limpar EEPROM e entrar em modo AP de configuração.
- **IDs Reservados:** Child IDs `254` (Intervalo) e `255` (Bateria) são globais e imutáveis.
- **Credenciais:** `include/M360Credentials.h` contém os defaults locais e nunca é versionado. O arquivo `include/M360Credentials.h.example` documenta todas as constantes obrigatórias.
- **EEPROM:** Configuração de rede usa versão, CRC e strings limitadas; dados inválidos acionam provisionamento seguro.
- **Comando remoto:** `FORCE_UPDATE` é o único payload aceito para forçar leituras.
