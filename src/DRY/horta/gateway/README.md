# Documentação: Gateway MySensors MQTT (Modular)

Este projeto implementa um gateway avançado para a rede MySensors, utilizando um ESP8266 (Wemos D1 Mini) como controlador central e um rádio nRF24L01+ para comunicação com os nós de campo. Ao contrário dos exemplos padrão, este gateway utiliza uma arquitetura modularizada para facilitar a manutenção, robustez de hardware e alta disponibilidade de rede.

---

## 1. Arquitetura Modular (`ngm/`)
O projeto está organizado na pasta `src/DRY/gateway/ngm/` ("Next Generation MQTT"), dividindo as responsabilidades em módulos:

| Módulo | Responsabilidade |
| :--- | :--- |
| `config_utils` | Gestão de memória EEPROM (offset 512), cálculo de CRC e estrutura do `DeviceConfig`. |
| `wifi_utils` | Gerenciamento da conexão Wi-Fi (STA) e do ponto de acesso de configuração (AP). |
| `mqtt_utils` | Lógica de conexão com o broker, tradução de mensagens MySensors/JSON e métricas. |
| `webserver` | Interface web para configuração remota do dispositivo. |
| `leds` | Controle visual de status através de LEDs independentes. |

---

## 2. Premissas de Hardware e Layout Elétrico

### A. Alimentação e Regulação de Tensão (`+3V3_RF`)
* **LDO Dedicado AMS1117-3.3V:** O módulo de rádio nRF24L01+ (especialmente versões com PA/LNA e antena externa em `PA_HIGH`) atinge picos de transmissão superiores a 100 mA, o que sobrecarrega o regulador interno do D1 Mini.
* **Redes Separadas:** A linha `+3V3_RF` é uma rede isolada alimentada pelo LDO AMS1117 dedicado (ligado ao barramento 5V), separada da linha `+3V3` do D1 Mini.

### B. Proteção e Lógica do Pino A0 (Resistor R4)
* **Resistor R4 (10 kΩ de A0 para 3V3) é OBRIGATÓRIO:** O Wemos D1 Mini possui um divisor resistivo interno de 220 kΩ / 100 kΩ no pino A0. Em estado flutuante, o ADC lê valor próximo de 0 (`isA0Low() == true`), o que fazia a placa ficar presa infinitamente no Modo AP de fábrica.
* **Comportamento elétrico:** Com o resistor R4 (pull-up de 10 kΩ para 3V3), a leitura de repouso fica estabilizada em ≈1023 (HIGH). O botão de manutenção SW1 puxa A0 para GND (0V / LOW) apenas quando pressionado.

### C. Orientação do D1 Mini (U1) e Otimização SPI
* **Rotação de 180° de U1:** O D1 Mini é montado girado em 180°, posicionando os pinos SPI (D5, D6, D7, D8) de frente para o barramento do rádio nRF24L01. Isso reduz o comprimento das trilhas de ~50 mm para ~25 mm.
* **Impacto no Layout:** O conector Micro-USB/USB-C do D1 Mini fica direcionado para a borda inferior da placa (indicado na serigrafia).

### D. Omissão Deliberada de Componentes
* **Sem Pull-up de CE e Sem Botão de Reset Extra:** O D1 Mini já conta com pull-up interno e botão de reset integrado em seu próprio PCB. Como o D1 Mini fica montado em soquete, o botão de reset é acessível por cima. O pino RST do header está marcado como No-Connect (NC).

### E. Especificações da PCI e Montagem
* **Dimensões e Camadas:** Placa de 100 × 70 mm, 2 camadas, com planos de GND em ambas as faces e recorte de cobre (keepout) sob a antena do rádio nRF24.
* **Zona de Exclusão:** O footprint do D1 Mini possui zona de exclusão de cobre sob a antena onboard do ESP-12.
* **Tecnologia de Montagem:** Módulos (D1 Mini e nRF24L01) em soquetes fêmea; demais componentes THT (passantes para soldagem manual).
* **Arquivos do Projeto:** Projeto KiCad completo, `m360_gateway_esquematico.pdf`, `m360_gateway_bom.csv` e `README.md`.

---

## 3. Fluxo de Operação e Inicialização

### A. Estágio `before()` (Pré-inicialização)
1. **Carga de Configuração:** Lê a estrutura `DeviceConfig` da EEPROM (offset 512+).
2. **Validação:** Verifica a integridade da configuração (versão + CRC).
3. **Seleção de Modo:**
   * **Modo AP (Configuração):** Ativado se a configuração for inválida ou se A0 estiver em LOW no boot (botão SW1 pressionado). Cria o AP `Manejo360-Config`.
   * **Modo STA (Operação):** Ativa a interface Wi-Fi para conexão com a rede configurada.

### B. Estágio `setup()` & `presentation()`
1. **Servidor Web:** Inicia o servidor HTTP na porta 80.
2. **Conexão MQTT:** Conecta ao broker MQTT com as credenciais salvas.
3. **Identificação MySensors:** Registra o gateway como Node 0 (`Manejo360 Gateway MQTT 2.0`).

### C. Estágio `loop()` (Execução Contínua)
1. `handleWiFiReconnect()` e `handleMQTTReconnect()` mantêm a conectividade.
2. `wait(1)` processa os pacotes do rádio MySensors.
3. `server.handleClient()` atende requisições no portal web.
4. `checkNodeTimeouts()` monitora a atividade dos nós.

---

## 4. Lógica de Comunicação e Tradução

* **Upstream (Rádio → MQTT):** Captura o pacote MySensors do rádio e publica um JSON em `m360/{UF}/{CAR}/out`.
  * *Payload Exemplo:* `{"nodeId":5, "sensorId":1, "command":1, "type":0, "payload":"25.5"}`
* **Downstream (MQTT → Rádio):** Assina o tópico `m360/{UF}/{CAR}/in`. Converte comandos JSON (brutos ou simplificados como `"action": "PUMP_ON"`) em mensagens MySensors para envio via rádio.

---

## 5. Diagnóstico e Sinalização Visual (Painel)

### Ordem Física dos LEDs no Painel
Para otimizar o roteamento de trilhas e evitar cruzamentos (já que o LED Amarelo é acionado pelo pino D0/GPIO16 na coluna oposta do D1 Mini), a disposição física no painel ficou definida como:

**[ Vermelho ] · [ Verde ] · [ Amarelo ]**

| Estado Visual | Cor / Posição | Pino D1 Mini | Significado |
| :--- | :--- | :--- | :--- |
| **Piscando Rápido** | Vermelho (1º) | D2 (GPIO4) | Wi-Fi desconectado ou Modo AP (Configuração) Ativo. |
| **Aceso Contínuo** | Verde (2º) | D1 (GPIO5) | Sistema Online (Wi-Fi + MQTT Conectados e Rádio OK). |
| **Piscando** | Amarelo (3º) | D0 (GPIO16) | Wi-Fi OK, mas Broker MQTT inacessível ou Inicializando. |
| **Flash Verde** | Verde (2º) | D1 (GPIO5) | Mensagem de rádio recebida com sucesso. |
| **Flash Amarelo** | Amarelo (3º) | D0 (GPIO16) | Transmissão de dados MQTT / Heartbeat. |

---

## 6. Verificações Críticas de Hardware Real

Antes da fabricação e montagem final da PCI, conferir fisicamente:
1. **Área da Antena do nRF24:** Garantir que o corpo do módulo nRF24L01 (≈15,3 × 29 mm) encaixe perfeitamente no soquete com a antena posicionada exatamente sobre a área de recorte de cobre (keepout) na placa.
2. **Ordem dos LEDs no Painel:** Confirmar serigrafia e furação do painel no padrão **Vermelho · Verde · Amarelo**.
3. **Resistor R4:** Confirmar soldagem do resistor de 10 kΩ entre A0 e 3V3.