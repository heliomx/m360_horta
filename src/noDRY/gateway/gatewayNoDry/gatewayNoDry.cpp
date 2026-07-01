/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * The ESP8266 MQTT gateway sends radio network (or locally attached sensors)
 * data to your MQTT broker. The node also listens to MY_MQTT_TOPIC_PREFIX and
 * sends out those messages to the radio network
 *
 * LED purposes:
 * - To use the feature, uncomment any of the MY_DEFAULT_xx_LED_PINs in your
 * sketch
 * - RX (green) - blink fast on radio message received. In inclusion mode will
 * blink fast only on presentation received
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode
 * will blink slowly
 * - ERR (red) - fast blink on error during transmission error or receive crc
 * error
 *
 * See https://www.mysensors.org/build/connect_radio for wiring instructions.
 *
 * If you are using a "barebone" ESP8266, see
 * https://www.mysensors.org/build/esp8266_gateway#wiring-for-barebone-esp8266
 *
 * Inclusion mode button:
 * - Connect GPIO5 (=D1) via switch to GND ('inclusion switch')
 *
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in
// MyConfig.h
#define MY_BAUD_RATE 115200

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
// #define MY_RADIO_RFM69
// #define MY_RADIO_RFM95

#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266

// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "m360/DF/0000/out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "m360/DF/0000/in"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-1"

// Enable these if your MQTT broker requires username/password
#define MY_MQTT_USER "jmm"
#define MY_MQTT_PASSWORD "jmmsqn"

// Set WIFI SSID and password
#define MY_WIFI_SSID "ANTARES"
#define MY_WIFI_PASSWORD "jmSsqA#12"

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "72.62.142.165"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// #define MY_IP_ADDRESS 192,168,178,87

// If using stSubnetatic ip you can define Gateway and  address as well
// #define MY_IP_GATEWAY_ADDRESS 192,168,178,1
// #define MY_IP_SUBNET_ADDRESS 255,255,255,0

// MQTT broker ip address.
#define MY_CONTROLLER_IP_ADDRESS 72, 62, 142, 165

// MQTT broker if using URL instead of ip address.
//  #define MY_CONTROLLER_URL_ADDRESS "test.mosquitto.org"

// The MQTT broker port to to open
#define MY_PORT 1883

// Enable inclusion mode
// #define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
// #define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
// #define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
// #define MY_INCLUSION_MODE_BUTTON_PIN A0

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
#define MY_DEFAULT_ERR_LED_PIN 1 // Error led pin
#define MY_DEFAULT_RX_LED_PIN 2  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN 4  // the PCB, on board LED

#include <MySensors.h>

void setup() {
  // Setup locally attached sensors
}

void presentation() {
  // Present locally attached sensors here
}

void loop() {
  // Send locally attached sensors data here
}