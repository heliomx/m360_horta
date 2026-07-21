# Regras do Projeto 

As regras a seguir aplicam-se a todos os agentes operando neste repositório.

## Sincronização entre Software e Hardware

* **DIRETRIZ CRÍTICA:** Qualquer alteração no software/firmware (ex: alteração de pinagem do Arduino, lógica de watchdog, protocolo serial ou mapeamento de acelerador) DEVE ser analisada e refletida nos arquivos de especificação de hardware (`BOM.md`  (lista de materiais), `esquema_eletrico.md`, 'diagrama_blocos.svg' ).
* **VICE-VERSA:** Qualquer alteração no circuito físico ou na pinagem de hardware DEVE ser analisada e refletida no código do firmware e nos arquivos de arquitetura de software correspondentes para evitar dessincronização entre o projeto físico e lógico.

Da memsa forma, considerar nesta sincronização o arquivo src\nodered\mapa da rede\nodered m360.json

## Sincronização e Integridade de Documentação

* **DIRETRIZ DE CONSISTÊNCIA:** Toda e qualquer alteração de comportamento físico (hardware) ou lógico (firmware/software) que impacte o funcionamento do projeto DEVE ser refletida de forma unificada e consistente em toda a documentação de apoio do projeto. Isso exige que os arquivos explicativos (`README.md` do firmware ou do app), o mapeamento visual no [mapeamento de acelerador) DEVE ser analisada e refletida nos arquivos de especificação de hardware (`BOM.md`  (lista de materiais), `esquema_eletrico.md`, 'diagrama_blocos.svg' e todos os artefatos técnicos de especificação, planejamento ou implementação mantidos na pasta [_bmad-output] (como `BOM.md`entre outros) sejam atualizados simultaneamente para prevenir qualquer divergência de informações no repositório.

Todo firmware (gateway e nós da rede) deve possuir os arquivos de documentação `BOM.md` (lista de materiais), `esquema_eletrico.md` e 'diagrama_blocos.svg'. Assim que detectada sua ausencia, um agente especializado deve ser acionado para providenciar a geração.

## Comunicação e Transparência e decisão do Usuário.

* **DIRETRIZ DE NOTIFICAÇÃO:** O usuário DEVE sempre ser explicitamente informado de forma clara sempre que as regras deste documento forem aplicadas. O agente deve relatar quais arquivos de software, especificações de hardware ou documentação técnica (incluindo READMEs, diagramas de blocos e esquema elétrico) foram analisados, cruzados ou modificados, fornecendo a justificativa de rastreabilidade para garantir a visibilidade total da sincronização do projeto.

## O Usuário deverá descidir, a depender do relatório de impacto dessas nos demais arquivos, se as mesmas deverão aplicadas ou não .


