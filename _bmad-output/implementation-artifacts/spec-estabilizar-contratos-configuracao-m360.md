---
title: 'Estabilizar contratos e configuracao do M360 Horta'
type: 'refactor'
created: '2026-06-21'
status: 'in-progress'
baseline_commit: 'fed8efa8d266d9862a8c6e8fce2fa34aebaab256'
context: ['AGENTS.md', '.agent/skills/bmad-mysensors-node-coding/SKILL.md', 'docs/architecture.md']
---

<frozen-after-approval reason="human-owned intent - do not modify unless human renegotiates">

## Intent

**Problem:** O ecossistema M360 possui contratos divergentes entre gateway, biblioteca e nos, um ambiente que nao compila, credenciais versionadas e leitura insegura de configuracao da EEPROM.

**Approach:** Consolidar os contratos ativos na biblioteca M360-DRY, externalizar credenciais, robustecer a persistencia, corrigir os sketches e filtros de build e criar verificacoes automatizadas para impedir regressao.

## Boundaries & Constraints

**Always:** Usar `FORCE_UPDATE` como comando unico; manter IDs 254/255; preservar EEPROM MySensors 0-511; manter JSON 512/384/256; comentarios e logs em portugues; usar uma casa decimal para floats.

**Ask First:** Mudanca do formato MQTT publico, dos IDs de nos ou sensores, ou do hardware/pinagem.

**Never:** Manter compatibilidade com `teste_do_gateway`; versionar segredos reais; apagar alteracoes preexistentes; alterar credenciais em producao remotamente.

## I/O & Edge-Case Matrix

| Scenario | Input / State | Expected Output / Behavior | Error Handling |
|----------|--------------|---------------------------|----------------|
| Configuracao inicial | EEPROM invalida | Defaults de `M360Credentials.h` e modo AP | Sem leitura fora dos buffers |
| EEPROM corrompida | Campos sem terminador NUL | Configuracao rejeitada com seguranca | Factory reset controlado |
| Force update | Action MQTT `FORCE_UPDATE` | No realiza leitura e responde | Action invalida e rejeitada |
| Build completo | Seis ambientes padrao | Todos geram firmware | Qualquer falha bloqueia entrega |

</frozen-after-approval>

## Code Map

- `M360Credentials.h(.example)` -- defaults privados e contrato documentado.
- `src/DRY/gateway/ngm/config_utils.*` -- EEPROM, CRC, topicos e factory reset.
- `lib/M360-DRY/src/M360Config.*` -- configuracao compartilhada robusta.
- `lib/M360-DRY/src/M360Node.cpp` -- comandos e telemetria de bateria.
- `src/DRY/nos/*/withLibDRY/*.cpp` -- lifecycle e perfis dos nos ativos.
- `platformio.ini` -- matriz de compilacao.

## Tasks & Acceptance

**Execution:**
- [ ] Criar credenciais locais ignoradas e template seguro versionado.
- [ ] Remover segredos hardcoded e mascarar logs sensiveis.
- [ ] Limitar operacoes de string e validar CRC/configuracao.
- [ ] Unificar `FORCE_UPDATE` sem alias legado.
- [ ] Corrigir presentation/perfil dos nos 01/04 e build do no 13.
- [ ] Corrigir semantica de bateria em V_VOLTAGE.
- [ ] Atualizar SSoT/documentacao e adicionar verificacoes automatizadas viaveis.

**Acceptance Criteria:**
- Given uma instalacao limpa, when o gateway inicia, then usa defaults privados e oferece AP sem expor senhas no log.
- Given EEPROM arbitraria, when a configuracao e carregada, then todos os buffers permanecem terminados e a validacao e limitada.
- Given `FORCE_UPDATE`, when o gateway encaminha a action, then todos os nos ativos usam o mesmo payload.
- Given a matriz PlatformIO, when `pio run` e `pio check` executam, then todos os ambientes passam.

## Spec Change Log

## Design Notes

`M360Credentials.h` fica na raiz e e incluido pelos modulos de gateway via include path do projeto. O template contem as mesmas constantes com valores demonstrativos. EEPROM continua permitindo configuracao pelo portal; as credenciais privadas sao apenas defaults de provisionamento/reset.

## Verification

**Commands:**
- `pio run` -- seis ambientes em SUCCESS.
- `pio check -e check_m360_dry` -- nenhum defeito medio/alto.
- `rg` para segredos conhecidos e `teste_do_gateway` -- nenhuma ocorrencia em arquivos versionaveis ativos.
