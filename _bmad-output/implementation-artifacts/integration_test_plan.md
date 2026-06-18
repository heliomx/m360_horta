# Plano de Testes de Integração: m360_horta

Este plano visa validar a comunicação fim-a-fim entre os **Nós Sensores**, o **Gateway MQTT** e o **Broker MQTT**, garantindo que as correções aplicadas nos firmwares funcionem no ambiente real.

---

## 🏗️ Ambiente de Teste
- **Nós:** `nodeUmidadeTemperatura` e `nodeSelenoieVazao`.
- **Gateway:** `Manejo360 Gateway MQTT v2.0` (ESP8266).
- **Broker:** Mosquitto ou similar (Node-RED/Home Assistant).
- **Ferramenta de Monitoramento:** MQTT Explorer ou CLI (`mosquitto_sub/pub`).

---

## 🧪 Casos de Teste (Integration Tests)

### 1. Descoberta e Registro (Provisioning)
- **Cenário:** Energizar um nó novo (ex: `nodeId 22`) e observar o tópico `.../gateway/events`.
- **Sucesso:** O Gateway deve publicar um evento JSON `node_presentation` e `node_discovered` contendo o ID do nó.

### 2. Telemetria de Sensores (Uplink)
- **Cenário:** Provocar mudanças nos sensores de solo e vazão.
- **Sucesso:** 
    - No tópico de saída (`.../out/`), os valores devem aparecer como JSON.
    - **Vazão:** Verificar se o reporte de vazão no `nodeSelenoieVazao` ocorre em menos de 5 segundos após o início do fluxo (Fast Sampling).

### 3. Controle de Atuadores (Downlink)
- **Cenário:** Publicar no tópico de entrada do Gateway (`.../in/`):
    ```json
    { "nodeId": 22, "sensorId": 2, "command": 1, "type": 2, "payload": "1" }
    ```
- **Sucesso:** A solenoide 1 do `nodeSelenoieVazao` deve abrir fisicamente (e o nó imprimir `[DRV] Sol 1 -> ABERTO`).

### 4. Configuração Remota (Payloads Customizados)
- **Cenário:** Enviar comando simplificado via MQTT:
    ```json
    { "nodeId": 22, "action": "FORCE_UPDATE" }
    ```
- **Sucesso:** O nó deve responder imediatamente enviando todos os sensores registrados.

---

## 🔍 Inconsistências Identificadas (Atenção!)

Durante a revisão do Gateway (`newGatewayMqtt.cpp`), identifiquei um desalinhamento com os novos firmwares:

| Feature | Gateway (ngm) | Nós (DRY) | Status |
| :--- | :--- | :--- | :--- |
| **SET_INTERVAL** | Usa `sensorId = 1` | Usa `sensorId = 254` | ❌ Incompatível |
| **VOLUME_RESET** | Mapeado apenas como "RESET" | Espera "RESET" em `V_VAR1` | ⚠️ Requer comando manual |

> [!IMPORTANT]
> **Recomendação:** Devemos atualizar o `newGatewayMqtt.cpp:553` para usar o `CHILD_ID_INTERVAL` global (254) em vez do 1.

---

## 📋 Lista de Verificação de Campo (Field Check)
- [ ] O LED Verde do Gateway pisca ao receber dados do rádio?
- [ ] O LED Amarelo do Gateway pisca ao publicar no MQTT?
- [ ] Os buffers de 32 bits de vazão estão reportando números crescentes sem pulos (devido à correção de atomicidade aplicada)?

---

**Deseja que eu implemente o "Patch de Compatibilidade" no Gateway agora para alinhar o `SET_INTERVAL` com os nós?**
