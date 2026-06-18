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

### 3.1 Node Engine (`shared/`)
Toda a complexidade de rede e ciclo de vida é encapsulada em macros e funções utilitárias:
- **`NODE_ENGINE_DEFINE_GLOBALS()`**: Padroniza as variáveis de estado e mensagens.
- **Gerenciamento de Energia**: Abstração dos perfis `LOW_POWER` (Deep Sleep) e `ALWAYS_ON` (Loops temporizados).

### 3.2 Isolamento de Hardware (`sensorDrivers`)
Cada nó separa a lógica de aplicação da implementação física dos drivers:
- **`readNodeItem(index)`**: Interface comum para o engine coletar dados sem conhecer o hardware.
- **`writeNodeItem(childId, state)`**: Interface comum para atuação.

## 4. Gestão de Energia e Proteção de Sensores

### 4.1 Ciclo de Vida do Sono (Deep Sleep)
Nós de bateria seguem a sequência:
1. `nodeEngine_powerUp()` (ativa pinos de alimentação).
2. Leitura de Sensores.
3. Transmissão.
4. Janela de Escuta (`MIN_AWAKE_TIME_MS`).
5. `nodeEngine_powerDown()` -> `sleep()`.

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
