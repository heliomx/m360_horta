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

### 5.3 Padrão de Nomenclatura para IDs e Tópicos MQTT

Para integração no ecossistema Manejo360 (respeitando o barramento MQTT e o modelo Proxy MCP v2.0), os tópicos de publicação e subscrição utilizam os prefixos:
- **Publicação (Out):** `#define MY_MQTT_PUBLISH_TOPIC_PREFIX "m360/DF/0000/out"`
- **Subscrição (In):** `#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "m360/DF/0000/in"`

O MySensors constrói os tópicos MQTT nativamente no formato:
$$\text{PREFIXO} / \text{node-id} / \text{child-sensor-id} / \text{command} / \text{ack} / \text{type}$$

#### Mapeamento de IDs de Nó (node-id: 0 a 254)
| Faixa de ID | Tipo de Nó / Aplicação | Exemplo de Uso |
|---|---|---|
| 0 | Gateway / Broker Hub | Gateway Central / MQTT Proxy |
| 1 – 50 | Estações Meteorológicas / Clima | Pluviômetro, Pyranômetro, Temp/Umidade Ar |
| 51 – 150 | Monitoramento de Solo (Talhões) | Sondas de Umidade RS485/NPK, Impedância de Solo |
| 151 – 200 | Atuação / Controle de Irrigação | Quadros de bombas, Solenoides, Válvulas de setor |
| 201 – 254 | Nós Especiais / Reservatórios | Nível de caixa d'água, pH, Condutividade Elétrica |

#### Mapeamento de IDs de Filho (child-sensor-id: 0 a 254)
| Faixa de Child | Tipo de Dispositivo | Macro MySensors Assasina (S_TYPE) | Tipo de Dado (V_TYPE) |
|---|---|---|---|
| 0 | Status do Próprio Nó / Bateria | `S_MULTIMETER` / Internal | `V_VOLTAGE`, `V_LEVEL` |
| 1 – 10 | Sensores de Solo | `S_MOISTURE` | `V_LEVEL` (%) ou `V_IMPEDANCE` |
| 11 – 20 | Sensores Ambientais / Clima | `S_TEMP`, `S_HUM`, `S_LIGHT` | `V_TEMP`, `V_HUM`, `V_LIGHT_LEVEL` |
| 21 – 30 | Sensores de Fluxo / Hidrometria | `S_WATER` | `V_FLOW`, `V_VOLUME` |
| 31 – 40 | Atuadores / Relés / Válvulas | `S_BINARY` | `V_STATUS` ($0=$OFF, $1=$ON) |

## 6. Segurança e Manutenção
- **Reset de Fábrica:** Detecção de hardware via pino `A0` (GND) para limpar EEPROM e entrar em modo AP de configuração.
- **IDs Reservados:** Child IDs `254` (Intervalo) e `255` (Bateria) são globais e imutáveis.
- **Credenciais:** `include/M360Credentials.h` contém os defaults locais e nunca é versionado. O arquivo `include/M360Credentials.h.example` documenta todas as constantes obrigatórias.
- **EEPROM:** Configuração de rede usa versão, CRC e strings limitadas; dados inválidos acionam provisionamento seguro.
- **Comando remoto:** `FORCE_UPDATE` é o único payload aceito para forçar leituras.

## 7. Autodescoberta (Auto-Discovery), Grafo Dinâmico e Proxy MCP v2.0

### 7.1 Mecanismo de Autodescoberta Nativo
A fase de apresentação (`C_PRESENTATION` e `C_INTERNAL`) nativa do MySensors permite que a topologia da rede seja autodescoberta pelo backend sem a necessidade de cadastros manuais exaustivos.
1. **Recepção de Informações do Nó (`I_SKETCH_NAME` / `I_SKETCH_VERSION`):** Tópico `m360/DF/0000/out/<node_id>/255/3/0/11`.
2. **Recepção da Lista de Childs (`C_PRESENTATION`):** Tópico `m360/DF/0000/out/<node_id>/<child_id>/0/0/<S_TYPE>`.
3. **Monitoramento de Saúde e Telemetria (`I_BATTERY_LEVEL`, `I_SIGNAL_REPORT`):** Tópico `m360/DF/0000/out/<node_id>/255/3/0/0`.

```
┌─────────────────┐       MQTT (out)       ┌──────────────────┐       Popula       ┌─────────────────┐
│ Nó MySensors    │ ─────────────────────► │ Backend / Proxy  │ ─────────────────► │ Banco / Grafo   │
│ (Boot & Config) │                        │ MCP              │                    │ de Dispositivos │
└─────────────────┘                        └──────────────────┘                    └─────────────────┘
```

### 7.2 Mapeamento e JSON de Instanciação (Proxy MCP v2.0 Binding)
No JSON de Instanciação do Talhão (Binding), o mapeamento traduz essa nomenclatura física MySensors diretamente nos Atributos e Funções da especificação MCP v2.0:

```json
{
  "mapeamento_atributos": [
    {
      "atributo_conceitual": "umidade_solo_avg",
      "estrategia": "MEDIA",
      "fontes": [
        { "topico_mqtt": "m360/DF/0000/out/51/1/1/0/35" },
        { "topico_mqtt": "m360/DF/0000/out/51/2/1/0/35" }
      ]
    }
  ],
  "mapeamento_funcoes": [
    {
      "funcao_conceitual": "RELE_BOMBA_IRRIG",
      "topico_comando_mqtt": "m360/DF/0000/in/51/31/1/0/2",
      "payload_ligar": "1",
      "payload_desligar": "0"
    }
  ]
}
```

### 7.3 Diagnóstico e Segurança no Motor de Inferência
Com os dados de saúde e topologia capturados dinamicamente pelo backend, o Motor de Inferência v2.0 executa regras de diagnóstico e segurança da irrigação (failsafe). Caso um nó perca sinal ou apresente bateria criticamente baixa (< 15%), o Proxy MCP invalida temporariamente as regras que dependem do nó, impedindo acionamentos indevidos.

