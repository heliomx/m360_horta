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
- `float lastValues[]`: Armazena a última leitura para comparação de mudança significativa.
- `uint8_t nNoUpdates[]`: Contador para forçar reporte em caso de valor estagnado.

**E ocupa o mínimo possível da SRAM (2.0.0):**
- **Um único `MyMessage` de envio**, montado (child + tipo) a cada `send()`. O valor sempre vem de `lastValues[]` e a mensagem nunca é lida de volta — um buffer por item era só cabeçalho repetido: 33 B × (itens + 3).
- **Rótulos e textos na Flash:** `label` é `const __FlashStringHelper*` (array `PROGMEM` + `M360_LABEL()`), e `begin()` recebe `F()`.

Medido no Nó 99 com a assinatura MySensors ligada: 1945 → 1235 B de RAM estática, e a pilha livre sobe de 103 para 813 B. É o que permite ligar a assinatura, que sozinha custa ~370 B mais a pilha do HMAC.

---

## 3. Gestão de Energia (Modos de Voo) ⚡

A LibDRY implementa quatro perfis, selecionados no ato da criação do objeto:

### `M360_LOW_POWER` (O Modo Padrão)
- **Comportamento:** Acorda -> Processa -> Dorme.
- **Diferencial:** Usa `smartSleep()`, que avisa o controlador e abre uma janela de escuta de 500 ms antes de dormir. Durante o sono o rádio fica **desligado** e o gateway **não retém** mensagens: um comando enviado nesse intervalo se perde, e cabe ao controlador reenviá-lo quando o nó reaparecer (é o papel da "caixa postal" do Node-RED).
- **Uso:** Sensores de umidade, temperatura e nível de bateria.

### `M360_ALWAYS_ON`
- **Comportamento:** Nunca dorme.
- **Lógica:** Implementa um timer interno via `millis()`.
- **Uso:** Nós em fonte fixa que precisam responder a comando na hora (atuadores, Nós 1, 2 e 99). Não encaminha mensagens de outros nós: para isso, `M360_REPEATER`.

### `M360_PASSIVE`
- **Comportamento:** O nó acorda a cada intervalo definido, mas NÃO realiza a leitura automática dos sensores.
- **Ação ao Acordar:** Ele utiliza o `smartSleep()` para verificar se o gateway possui mensagens pendentes (como comandos `V_STATUS` ou `V_CUSTOM`).
- **Diferencial:** Se não houver comando no gateway, ele volta a dormir imediatamente após o "check-in" do `smartSleep`. O ciclo de leitura/envio só acontece se o gateway enviar um comando durante a janela de acordado.
- **Uso:** Sensores de altíssimo consumo (ex: RS485 Modbus) que só devem ser ativados quando o sistema central realmente precisar do dado.

### `M360_REPEATER`
- **Comportamento:** Igual ao `M360_ALWAYS_ON`, com `-D MY_REPEATER_FEATURE` no env: o MySensors mantém a tabela de rotas e encaminha mensagens de nós fora do alcance do gateway.
- **Uso:** Nó em fonte fixa numa posição central — o Nó 99 nos envs `nano_99reles_rep` e `act_99_rep`.

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
