/*
 * Power Profile - Perfis de Energia para Nós MySensors
 * 
 * Define diferentes perfis de consumo de energia para otimizar
 * a vida útil da bateria em diferentes cenários de uso.
 */

#pragma once
#include <Arduino.h>

// ===== PERFIS DE ENERGIA =====

// Perfil de baixo consumo - smartSleep ativo
#define POWER_PROFILE_LOW_POWER 1

// Perfil sempre ligado - sem sleep (para desenvolvimento/debug)
#define POWER_PROFILE_ALWAYS_ON 2

// ===== VALIDAÇÃO =====

#if defined(POWER_PROFILE_LOW_POWER) && defined(POWER_PROFILE_ALWAYS_ON)
  #error "Apenas um perfil de energia pode ser definido: POWER_PROFILE_LOW_POWER ou POWER_PROFILE_ALWAYS_ON"
#endif

// Padrão: LOW_POWER se nenhum perfil for especificado
#if !defined(POWER_PROFILE_LOW_POWER) && !defined(POWER_PROFILE_ALWAYS_ON)
  #define POWER_PROFILE_LOW_POWER
#endif

// ===== CONFIGURAÇÕES POR PERFIL =====

#ifdef POWER_PROFILE_LOW_POWER
  #ifndef ENABLE_SMART_SLEEP
  #define ENABLE_SMART_SLEEP true
  #endif
  #ifndef ENABLE_POWER_MANAGEMENT
  #define ENABLE_POWER_MANAGEMENT true
  #endif
  #ifndef DEFAULT_SLEEP_INTERVAL_MIN
  #define DEFAULT_SLEEP_INTERVAL_MIN 5
  #endif
  #ifndef MIN_SLEEP_INTERVAL_MIN
  #define MIN_SLEEP_INTERVAL_MIN 1
  #endif
  #ifndef MAX_SLEEP_INTERVAL_MIN
  #define MAX_SLEEP_INTERVAL_MIN 1440
  #endif
#endif

#ifdef POWER_PROFILE_ALWAYS_ON
  #ifndef ENABLE_SMART_SLEEP
  #define ENABLE_SMART_SLEEP false
  #endif
  #ifndef ENABLE_POWER_MANAGEMENT
  #define ENABLE_POWER_MANAGEMENT false
  #endif
  #ifndef DEFAULT_SLEEP_INTERVAL_MIN
  #define DEFAULT_SLEEP_INTERVAL_MIN 0
  #endif
  #ifndef MIN_SLEEP_INTERVAL_MIN
  #define MIN_SLEEP_INTERVAL_MIN 0
  #endif
  #ifndef MAX_SLEEP_INTERVAL_MIN
  #define MAX_SLEEP_INTERVAL_MIN 0
  #endif
#endif

// ===== MACROS DE CONFIGURAÇÃO =====

// Macro para verificar se sleep está habilitado
#define IS_SLEEP_ENABLED() (ENABLE_SMART_SLEEP)

// Macro para verificar se gerenciamento de energia está habilitado
#define IS_POWER_MANAGEMENT_ENABLED() (ENABLE_POWER_MANAGEMENT)

// ===== FUNÇÕES DE ENERGIA =====

// Função para desligar periféricos antes do sleep
void powerDownPeripherals();

// Função para ligar periféricos após acordar
void powerUpPeripherals();

// Função para configurar pinos de energia
void setupPowerPins();
