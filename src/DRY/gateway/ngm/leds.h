#pragma once
#include <Arduino.h>

// === DEFINIÇÃO DE PINOS LED (Common) ===
#define LED_RED     D2   // D2 (GPIO4) - Vermelho (ERR)
#define LED_YELLOW  D0  // D0 (GPIO16) - Amarelo (TX)
#define LED_GREEN   D1   // D1 (GPIO5) - Verde (RX)
#define RESET_PIN A0

// === Estados de LED ===
enum LedState {
  LED_STATE_OFF,
  LED_STATE_ON,
  LED_STATE_BLINK
};

// === Estrutura de controle ===
struct LedControl {
  int pin;
  LedState state;
  unsigned long interval;
  unsigned long lastToggle;
  bool current;
  unsigned long flickerEnd;
};

// === Funções da biblioteca ===
void initLEDs(int redPin, int yellowPin, int greenPin);
void ledBegin();
void setLedState(int pin, LedState state, unsigned long interval = 500);
void updateLEDs();
void ledFlicker(int pin);
