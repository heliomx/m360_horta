## Deferred from: code review of story 2-1 (2026-04-02)
- Calibração de Solo Linear em Sensor Não-Linear [sensorDrivers.cpp:132]. O uso de map(1023, 300, 0, 100) assume linearidade em sensores que são notavelmente não-lineares.

## Deferred from: code review (2026-06-17)
- Uso de delays bloqueantes (delay()) no loop principal [sensores.cpp:50, 85]. O uso de delay() bloqueia a CPU do microcontrolador e impede o processamento de outras tarefas de background se esse código for transposto para nós MySensors de produção.
