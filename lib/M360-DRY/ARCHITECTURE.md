# 🏗️ Arquitetura Interna — LibDRY

A **LibDRY** foi construída sobre três pilares fundamentais: **Desacoplamento**, **Estabilidade de Memória** e **Eficiência Energética**. Este documento detalha como esses pilares são implementados no código.

---

## 1. Desacoplamento via Callbacks

Diferente de bibliotecas monolíticas, o `M360Node` não sabe como ler o seu sensor específico ou como acionar o seu relé. Ele delega essa responsabilidade para você através de **Callbacks**.

### Como funciona:
O `M360Node` possui dois "ganchos" (pointers):
- `_readCb`: Chamado quando o motor precisa de um dado.
- `_writeCb`: Chamado quando o motor recebe um comando do gateway.

**Vantagem:** Você pode trocar o sensor físico (ex: mudar de um DHT11 para um BME280) sem alterar uma única linha de código da lógica de rede ou bateria.

---

## 2. Gestão de Memória (Sem Fragmentação)

O ATmega328P tem apenas 2KB de SRAM. O uso de `std::vector` ou `malloc` causaria fragmentação e crash do dispositivo em poucos dias.

**A LibDRY resolve isso exigindo buffers estáticos:**
Quando você instancia o `M360Node`, você deve passar arrays pré-alocados para:
- `MyMessage messages[]`: Buffer estático para mensagens de rádio.
- `float lastValues[]`: Armazena a última leitura para comparação de mudança significativa.
- `uint8_t nNoUpdates[]`: Contador para forçar reporte em caso de valor estagnado.

---

## 3. Gestão de Energia (Modos de Voo) ⚡

A LibDRY implementa três perfis distintos, selecionados no ato da criação do objeto:

### `M360_LOW_POWER` (O Modo Padrão)
- **Comportamento:** Acorda -> Processa -> Dorme.
- **Diferencial:** Usa `smartSleep()`. Se o gateway enviar uma mensagem enquanto o nó dorme, o MySensors segura a mensagem e a entrega assim que o nó acorda para a próxima leitura.
- **Uso:** Sensores de umidade, temperatura e nível de bateria.

### `M360_ALWAYS_ON`
- **Comportamento:** Nunca dorme.
- **Lógica:** Implementa um timer interno via `millis()`.
- **Uso:** Nós ligados em fonte 12V ou USB que precisam atuar como repetidores da rede.

### `M360_PASSIVE`
- **Comportamento:** O nó acorda a cada intervalo definido, mas NÃO realiza a leitura automática dos sensores.
- **Ação ao Acordar:** Ele utiliza o `smartSleep()` para verificar se o gateway possui mensagens pendentes (como comandos `V_STATUS` ou `V_CUSTOM`).
- **Diferencial:** Se não houver comando no gateway, ele volta a dormir imediatamente após o "check-in" do `smartSleep`. O ciclo de leitura/envio só acontece se o gateway enviar um comando durante a janela de acordado.
- **Uso:** Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.

---

## 4. O Sistema de VCC Controlado

Se o seu hardware tiver um pino que corta a energia dos sensores para economizar bateria (ex: pino D4 ligado a um transistor), a LibDRY sabe lidar com isso.

O motor chama as funções `M360::powerUp()` e `M360::powerDown()` automaticamente. Elas são declaradas como `weak` na biblioteca, o que significa que o seu código principal pode "atropelar" a versão padrão:

```cpp
namespace M360 {
    void powerUp() {
        digitalWrite(VCC_PIN, HIGH);
        delay(500); // Aguarda estabilização térmica
    }
    void powerDown() {
        digitalWrite(VCC_PIN, LOW);
    }
}
```

---
*Retornar ao [README.md](README.md)*
