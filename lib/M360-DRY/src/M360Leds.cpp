/*
 * M360Leds.cpp — Controle de LEDs de status para Gateway M360 (ESP8266 D1 Mini)
 */

#ifdef ESP8266

#include "M360Leds.h"

static LedControl redLED, yellowLED, greenLED;

void initLEDs(int redPin, int yellowPin, int greenPin) {
    redLED    = {redPin,    LED_STATE_OFF, 500, 0, false, 0};
    yellowLED = {yellowPin, LED_STATE_OFF, 500, 0, false, 0};
    greenLED  = {greenPin,  LED_STATE_OFF, 500, 0, false, 0};

    pinMode(redPin,    OUTPUT);
    pinMode(yellowPin, OUTPUT);
    pinMode(greenPin,  OUTPUT);

    digitalWrite(redPin,    LOW);
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin,  LOW);
}

void ledBegin() {
    for (int i = 0; i < 3; i++) {
        setLedState(redLED.pin,    LED_STATE_ON);
        setLedState(yellowLED.pin, LED_STATE_ON);
        setLedState(greenLED.pin,  LED_STATE_ON);
        delay(200);
        setLedState(redLED.pin,    LED_STATE_OFF);
        setLedState(yellowLED.pin, LED_STATE_OFF);
        setLedState(greenLED.pin,  LED_STATE_OFF);
        delay(200);
    }
}

void setLedState(int pin, LedState state, unsigned long interval) {
    LedControl *led = nullptr;
    if      (pin == redLED.pin)    led = &redLED;
    else if (pin == yellowLED.pin) led = &yellowLED;
    else if (pin == greenLED.pin)  led = &greenLED;
    else return;

    led->state    = state;
    led->interval = interval;
    if (state == LED_STATE_ON) {
        digitalWrite(pin, HIGH);
        led->current = true;
    } else if (state == LED_STATE_OFF) {
        digitalWrite(pin, LOW);
        led->current = false;
    }
}

void updateLEDs() {
    unsigned long now = millis();
    LedControl *leds[3] = {&redLED, &yellowLED, &greenLED};

    for (auto l : leds) {
        if (now < l->flickerEnd) {
            digitalWrite(l->pin, HIGH);
            continue;
        }
        if (l->state == LED_STATE_BLINK) {
            if (now - l->lastToggle >= l->interval) {
                l->current = !l->current;
                digitalWrite(l->pin, l->current);
                l->lastToggle = now;
            }
        } else if (l->state == LED_STATE_ON) {
            digitalWrite(l->pin, HIGH);
        } else {
            digitalWrite(l->pin, LOW);
        }
    }
}

void ledFlicker(int pin) {
    LedControl *led = nullptr;
    if      (pin == redLED.pin)    led = &redLED;
    else if (pin == yellowLED.pin) led = &yellowLED;
    else if (pin == greenLED.pin)  led = &greenLED;
    else return;

    led->flickerEnd = millis() + 60;
    digitalWrite(pin, HIGH);
}

#endif // ESP8266
