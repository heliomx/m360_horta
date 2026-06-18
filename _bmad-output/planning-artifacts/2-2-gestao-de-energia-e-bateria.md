# Story 2.2: Gestão de Energia e Bateria

Como Sistema de Monitoramento,
Eu quero entrar em Deep Sleep profundo e gerenciar a energia dos periféricos,
Para que o nó opere por meses em bateria sem intervenção humana.

**Critérios de Aceitação:**

**Dado** que o nó terminou de enviar os dados
**Quando** transcorrido o tempo de processamento mínimo
**Então** desligar sensores (VCC) e entrar em `sleep()` pelo tempo do `updateInterval`.

**Dado** o sensor de umidade do solo
**Quando** o nó acordar
**Então** ligar o pino de energia (VCC_SENSORS), ler o dado e desligar imediatamente (evitando corrosão).

## Tarefas/Subtasks
- [ ] Definir pino de controle de energia (VCC_SENSORS_PIN) no `sensorDrivers.h`.
- [ ] Implementar `powerDownPeripherals()` e `powerUpPeripherals()` em `sensorDrivers.cpp`.
- [ ] Ajustar `setupPowerPins()` para configurar o pino de controle como OUTPUT.
- [ ] Validar consumo térmico/estabilização após ligar o VCC (`delay` necessário).
- [ ] Testar ciclo de vida da bateria via reporte `V_VOLTAGE` (Node ID 255).

## Status: completed

## Dev Agent Record
### Implementation Plan
Utilizamos o pino D4 para chavear o VCC dos sensores analógicos e DHT11. Implementamos uma janela de escuta de 3s após o envio para garantir o recebimento de comandos (FORCE_UPDATE e SET_INTERVAL) antes do Deep Sleep.

### Completion Notes
- Pino D4 configurado como sensor VCC.
- Delay de 1.2s adicionado para estabilização do DHT11 após power-up.
- Handler de receive() agora suporta mudança dinâmica de intervalo via Child 254.
