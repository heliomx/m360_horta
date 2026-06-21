/*
Não apagar este comentário
Organização dos Arquivos e Documentação DRY (Don't Repeat Yourself): 
Recomenda-se criar arquivos de configuração compartilhados 
(ex.: pinos, intervalos, constantes do projeto) para evitar duplicidade 
na pasta /nos/.
/nos/
│
├── /shared/
│   ├── config.h
│   ├── powerProfile.h
│   ├── node_engine.h
│   └── node_engine.cpp
│
├── /nodePump/
│   ├── nodePump.cpp
│   ├── sensorDrivers.cpp
│   └── sensorDrivers.h
│
└── /nodeDHT11/
│   ├── nodeDHT.cpp
│   ├── sensorDrivers.cpp
│   └── sensorDrivers.h

*/

#pragma once
#include <Arduino.h>

// ===== CONSTANTES COMPARTILHADAS =====

// EEPROM
#define EEPROM_INTERVAL_ADDRESS 512

// Configurações de intervalo (em minutos)
#define DEFAULT_INTERVAL 1
#define MIN_INTERVAL 1
#define MAX_INTERVAL 1440

// Child IDs especiais
#define CHILD_ID_INTERVAL 254
#define CHILD_ID_BATTERY 255

// Timing
#define MIN_AWAKE_TIME_MS 3000

// Constantes de bateria
#define MIN_VOLTAGE 3.0
#define MAX_VOLTAGE 4.2

// Sensor inválido
#define SENSOR_INVALID_READING -32768

// Comandos remotos
#define CMD_FORCE_UPDATE "FORCE_UPDATE"

// Controle de acordar por rádio
extern volatile bool wokeUpByRadio;
