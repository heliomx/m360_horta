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

## Formatação de Respostas e Fórmulas Matemáticas

* **PROIBIÇÃO DE SINTAXE LATEX:** É estritamente proibido utilizar marcação LaTeX (como `$ ... $`, `$$ ... $$`, `\text{...}`, `\frac{...}`, etc.) nas respostas ao usuário.
* **PADRÃO DE FORMATAÇÃO CLARA:** Todas as equações, fórmulas matemáticas ou expressões técnicas devem ser formatadas exclusivamente em texto legível, utilizando marcadores em negrito/itálico, pseudo-código ou blocos de código Markdown (`code block`).
  * **Exemplo proibido (NÃO USAR):** `Vazão em L/min: $Q (\text{L/min}) = \frac{\text{Pulsos} \times 60}{\text{Intervalo (s)} \times 450}$`
  * **Exemplo correto (USAR):** `Vazão (L/min) = (Pulsos * 60) / (Intervalo_segundos * 450)` ou em bloco de código:
    ```text
    Vazao_L_min = (Pulsos * 60) / (Intervalo_segundos * 450)
    ```

## Fonte da Verdade e Documentação Externa (Manejo360)

* **REPOSITÓRIO COMO FONTE ÚNICA DA VERDADE (SSoT):** O repositório `c:\Users\jmarc\Documents\PlatformIO\Projects\m360_horta` é a Fonte Única da Verdade para todas as especificações de firmware, pinagem de hardware, tópicos MQTT, motor de nós (`node_engine.h`), abstrações DRY e automações.
* **SINCRONIZAÇÃO DA DOCUMENTAÇÃO EXTERNA:** A pasta de documentação técnica em `d:\Meu Drive\GDrive Meus Documentos\Projetos (1)\ViridIoTech\Projetos\Manejo360\Documentação` é parte integrante e oficial das especificações do ecossistema Manejo360. Toda e qualquer alteração no código-fonte, arquitetura de software, pinagem ou circuito físico mantido no repositório DEVE ser obrigatoriamente refletida e sincronizada nos arquivos correspondentes dessa pasta externa (ex: `00. Documento Técnico Manejo360.md`, `01. API do Backend e Estrutura de Tópicos MQTT - v1.0.md`, `05. Especificação de Hardware IoT.md`, `Arquitetura IOT.md`, etc.), garantindo 100% de integridade e ausência de divergências.
