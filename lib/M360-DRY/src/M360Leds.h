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
#define LED_GREEN   D1   // GPIO5  — Verde (RX)
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
