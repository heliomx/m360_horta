# Nó 01 - Monitoramento 3D de Solo (Versão Pro Mini)

Esta pasta contém o código e documentação para o **Nó 01 (Versão Pro Mini)**, configurado em baixo consumo (`M360_LOW_POWER`), projetado para ser alimentado por uma bateria recarregável e painel solar.

## Estratégia de Economia e Eletrólise

1. **Baixo Consumo (`smartSleep`)**:
   O microcontrolador (Arduino Pro Mini, 3.3V/8MHz) permanece a maior parte do tempo em repouso absoluto. Ele acorda apenas no intervalo especificado pelo controlador (salvo na EEPROM), realiza a varredura dos 18 sensores resistivos, envia os dados consolidados e volta a dormir.

2. **Mitigação de Eletrólise**:
   A barra de resistores de pull-up dos sensores de solo é alimentada pelo pino **D3** (`PIN_POWER_SENSORS`). Este pino é mantido em nível lógico `LOW` (0V) durante o repouso. O pino é ativado (`HIGH`) pela função `M360::powerUp()` apenas no momento das leituras e desligado (`LOW`) por `M360::powerDown()` imediatamente antes de dormir, cessando toda e qualquer corrente galvânica pelo solo.

## Acomodação Analógica e Estabilização

Para filtrar ruídos induzidos pelos cabos longos (cabo Ethernet) e capacitâncias do barramento multiplexado:
- Ao ativar a energia dos sensores (`powerUpSensors`), é inserido um atraso de **20ms** para estabilização elétrica.
- Ao comutar cada canal no multiplexador CD74HC4067, aguarda-se **5ms** para estabilização da impedância do canal.
- Uma leitura de descarte (`dummy read`) é realizada no ADC para descarregar o capacitor de Sample and Hold (S/H), seguida por uma pequena pausa de **3ms** antes da leitura analógica definitiva.



1. Quanto tempo leva para o nó adormecer após responder ao gateway?
O tempo total em que o nó permanece acordado após o recebimento de uma mensagem do gateway é de aproximadamente 0,6 segundos (600ms).

Como funciona esse ciclo internamente:

Varredura (Leitura): Ao receber a mensagem FORCE_UPDATE na callback receive(), o nó executa a varredura física de todos os 18 sensores analógicos. Graças à otimização de manter o pino de energia ativado de forma contínua durante a leitura em vez de chavear a cada sensor, esta varredura leva apenas cerca de 110ms.
Transmissão: O nó envia as respostas via rádio ao gateway. Isso ocorre quase instantaneamente (milissegundos).
Janela de Escuta (smartSleep): O fluxo do código retorna ao loop principal e chama a função smartSleep(). A biblioteca MySensors envia uma mensagem de pré-sleep ao gateway e mantém o rádio em modo de recepção por exatamente 500ms para garantir que nenhuma outra mensagem urgente do gateway seja perdida.
Sleep: Passados os 500ms de escuta, o rádio nRF24L01+ é colocado em Power Down e a MCU (ATmega328P) entra em Deep Sleep por interrupção de watchdog, consumindo corrente mínima (~5µA a 15µA).


2. Como implementar o divisor de tensão no CD74HC4067 para sensores resistivos de umidade?
Para ler sensores resistivos com o conversor analógico-digital (ADC) do Arduino, precisamos formar um divisor de tensão. A resistência do solo ($R_{\text{solo}}$) atua como a resistência inferior do divisor, enquanto um resistor fixo de 10kΩ atua como a resistência superior (Pull-up).

Existem duas formas eficientes de implementar isso com o multiplexador CD74HC4067:


Opção B: Pull-up Único Centralizado no pino SIG (Economia de Componentes)
Nesta configuração, você conecta apenas um único resistor de 10kΩ entre o pino D3 e o pino SIG (ligado ao A0). Cada canal do MUX ($C_0$ a $C_{15}$) é conectado diretamente ao eletrodo positivo de seu respectivo sensor, sem resistores intermediários.

Vantagem: Você economiza 15 resistores. Só precisará de 3 resistores no total (1 para o MUX no pino A0/SIG + 2 para as portas nativas A1 e A2).
Desvantagem: A resistência interna do canal ativo do MUX ($R_{\text{on}} \approx 70\Omega$) entra em série com o sensor, alterando levemente a equação do divisor: $$V_{\text{out}} = V_{\text{in}} \times \frac{R_{\text{solo}} + R_{\text{on}}}{R_{\text{pullup}} + R_{\text{solo}} + R_{\text{on}}}$$ Como a resistência do solo molhado varia entre 1kΩ e 10kΩ, a diferença de 70Ω é desprezível e pode ser ignorada ou calibrada via software.
Esquema de ligação 


           D3 (Pino Digital - VCC temporário)
           │
         [10k]
           │
           ├───> A0 (SIG do Multiplexador)
           │
           └───> [ MUX Interno ] ───> Canal C0 (Selecionado) ───> [Solo Sensor 0] ───> GND
                                 ───> Canal C1 (Aberto) ────────> [Solo Sensor 1] ───> GND


Nota de Segurança: Ambas as opções são totalmente seguras contra fuga de corrente durante o sleep. Quando o nó entra em repouso, o pino D3 é colocado em LOW (GND), o que remove toda a tensão do divisor, anulando o consumo elétrico e eliminando o processo de eletrólise nos eletrodos de inox.A leitura e o envio imediato da carga da bateria foram implementados com sucesso!

O que foi feito:
Alteração do Código (01nodeSolo3dMini.cpp): Modifiquei o gancho de finalização de ciclo M360::powerDown() para ler a bateria e transmiti-la via rádio em toda varredura em que o nó acorda:

cpp
void powerDown() {
    powerDownSensors();
    // Envia a carga da bateria sempre depois de acordar (antes de dormir)
    uint8_t batt = readBatteryPercent();
    send(messages[numItems + 1].set(batt));
    #ifdef MY_DEBUG
    Serial.print(F("Bat:"));
    Serial.println(batt);
    #endif
}
Isso contorna a lógica padrão da biblioteca M360-DRY que apenas enviava a bateria a cada 10 ciclos (e somente se houvesse variação de pelo menos 1%). Agora, a bateria é enviada de forma confiável a cada check-in.
Como a bateria é lida (sem pinos extras): A leitura correta da carga já está implementada na biblioteca e utiliza a referência interna de 1.1V (Bandgap) da própria MCU ATmega328P para ler a tensão de VCC (bateria conectada diretamente em VCC, conforme o esquema elétrico da Opção A). Dessa forma, a tensão é convertida linearmente para porcentagem de 0% (3.0V) a 100% (4.2V) de forma precisa e sem gastar pinos de leitura.