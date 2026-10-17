/*
 * M360Leds.h — Controle de LEDs de status para Gateway M360 (ESP8266 D1 Mini)
 */

#pragma once

#ifndef ESP8266
#  error "M360Leds.h é exclusivo do ESP8266."
#endif

#include <Arduino.h>

#define LED_RED     D2   // GPIO4  — Vermelho (ERR)
#define LED_YELLOW  D0   // GPIO16 — Amarelo (TX)
// Verde (RX): depende da placa. Na M360-GW (-D M360_BOARD_GW no env) o D1 é o
// DIO0 do RFM95W / CE do nRF24 legado (J4), e o verde sai pelo D4 (GPIO2), que
// comanda o gate do 2N7002 Q1 — ativo em HIGH como os outros dois. No protótipo
// (envs d1_mini_gateway e d1_mini_kit_helio_gateway) continua no D1.
#ifdef M360_BOARD_GW
#  define LED_GREEN D4   // GPIO2  — Verde (RX) via Q1, M360-GW
#else
#  define LED_GREEN D1   // GPIO5  — Verde (RX), protótipo
#endif
#define RESET_PIN   A0

enum LedState {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINK
};

struct LedControl {
    int pin;
    LedState state;
    unsigned long interval;
    unsigned long lastToggle;
    bool current;
    unsigned long flickerEnd;
};

void initLEDs(int redPin, int yellowPin, int greenPin);
void ledBegin();
void setLedState(int pin, LedState state, unsigned long interval = 500);
void updateLEDs();
void ledFlicker(int pin);
