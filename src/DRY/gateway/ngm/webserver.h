#pragma once
#include <ESP8266WebServer.h>
#include "config_utils.h"

void setupWebServer(DeviceConfig &cfg, ESP8266WebServer &server);
void handleRoot(DeviceConfig &cfg, ESP8266WebServer &server);
void handleSave(DeviceConfig &cfg, ESP8266WebServer &server);
String generateIndexHTML(const DeviceConfig &cfg);
