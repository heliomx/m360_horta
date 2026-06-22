/*
 * M360Webserver.h — Portal web de configuração do Gateway M360
 */

#pragma once

#ifndef ESP8266
#  error "M360Webserver.h é exclusivo do ESP8266."
#endif

#include <ESP8266WebServer.h>
#include "M360Gateway.h"

void setupWebServer(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleRoot(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleSave(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
String generateIndexHTML(const M360::M360DeviceConfig &cfg);
