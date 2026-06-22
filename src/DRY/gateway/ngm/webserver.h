#pragma once
#include <ESP8266WebServer.h>
#include <M360Gateway.h>

void setupWebServer(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleRoot(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleSave(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
String generateIndexHTML(const M360::M360DeviceConfig &cfg);
