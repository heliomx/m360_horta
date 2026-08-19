A pasta src\DRY\nos\shared contém o Node Engine, um framework interno desenvolvido para o projeto M360 que segue a filosofia DRY (Don't Repeat Yourself). Ele centraliza toda a lógica complexa e repetitiva dos nós MySensors, permitindo que novos sensores sejam adicionados com o mínimo de código possível.

Aqui está o detalhamento técnico de como esse motor funciona:

1. O Conceito do "Node Engine" (node_engine.h/cpp)
O Engine é o "maestro" de cada nó. Em vez de cada sensor implementar seu próprio setup(), loop() e presentation(), eles delegam essas tarefas ao Engine.

Abordagem Declarativa: O programador define apenas um array de estruturas NodeItemDef (que descreve os IDs dos sensores, pinos e intervalos) e o Engine cuida da comunicação com o rádio.
Gestão de Intervalos: O motor gerencia automaticamente quando cada sensor deve ser lido e enviado, suportando intervalos dinâmicos que podem ser alterados remotamente via Gateway (salvos na EEPROM do nó).
Encapsulamento MySensors: Funções como nodeEngine_presentation() garantem que todos os nós se apresentem de forma padronizada na rede.
2. Perfis de Energia (powerProfile.h/cpp)
Este módulo é vital para nós operados por bateria. Ele define como o nó deve se comportar para economizar energia:

POWER_PROFILE_LOW_POWER: Ativa o smartSleep, permitindo que o nó durma profundamente, mas acorde periodicamente para verificar se há comandos pendentes do Gateway.
Gerenciamento de Periféricos: Provê as funções powerDownPeripherals() e powerUpPeripherals(). Isso é usado para desligar a alimentação de sensores (via transistores ou pinos digitais) enquanto o nó dorme, eliminando o "consumo fantasma".
3. Configurações Compartilhadas (config.h)
Centraliza as constantes globais para garantir que todos os nós falem a "mesma língua":

IDs Especiais: Define que o Child ID 255 é sempre para bateria e o 254 para configuração de intervalo.
Limites de Voltagem: Define 3.0V como mínimo e 4.2V como máximo (padrão para baterias Li-ion/LiPo), usados para o cálculo de porcentagem de carga.
EEPROM: Mapeia os endereços fixos de memória para que configurações não sejam perdidas durante resets.
4. Gestão de Bateria
O motor inclui lógica embutida para monitorar a saúde do nó:

Faz a leitura da voltagem VCC.
Converte para porcentagem (0-100%).
Envia automaticamente o nível de bateria para o Gateway no Child ID 255.
Resumo do Fluxo de um Nó com o Engine:
Boot: O nó carrega as configurações do config.h.
Power Up: O powerProfile ativa os sensores.
Read: O Engine chama a função de leitura específica do sensor.
Send: Os dados são enviados via MySensors.
Sleep: O Engine calcula o tempo de sono, chama o powerDown para economizar energia e entra em smartSleep.
Vantagem do shared: Se você descobrir um bug na forma como os nós dormem ou quiser mudar a lógica de cálculo de bateria, você altera em um só lugar (dentro de shared/) e todos os nós do projeto (Umidade, Temperatura, Pump, etc.) serão atualizados automaticamente.