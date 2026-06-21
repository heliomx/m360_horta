/*
 * M360.h — Cabeçalho centralizador da biblioteca M360-DRY
 *
 * Inclua este arquivo nos nós do projeto para ter acesso a todas as
 * funcionalidades da biblioteca M360 sem precisar importar múltiplos headers.
 *
 * USO:
 *   #include <Arduino.h>
 *   #include <MySensors.h>
 *   #include <M360.h>
 */

#pragma once

// Inclui dependências principais
#include "M360Config.h"
#include "M360Constants.h"
#include "M360Power.h"
#include "M360Node.h"

// Gateway ESP8266 — incluído apenas quando compilando para ESP8266
#ifdef ESP8266
#include "M360Gateway.h"
#endif

// Facilita o uso do namespace para o desenvolvedor final
// (opcional, mas recomendado seguir o template)
// using namespace M360;
