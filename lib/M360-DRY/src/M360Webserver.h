/*
 * M360Webserver.h — Portal web de configuração do Gateway M360
 */

#pragma once

#ifndef ESP8266
#  error "M360Webserver.h é exclusivo do ESP8266."
#endif

#include <ESP8266WebServer.h>
#include <functional>
#include "M360Gateway.h"

// onFactoryReset: se informado, o portal mostra o botão "Restaurar padrões de fábrica"
// e registra POST /reset (exige confirmação); o handler deve apagar a configuração e
// reiniciar — no gateway, M360::FactoryReset::trigger(). Sem ele, nem botão nem rota.
void setupWebServer(M360::M360DeviceConfig &cfg, ESP8266WebServer &server,
                    std::function<void()> onFactoryReset = nullptr);
void handleRoot(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleSave(M360::M360DeviceConfig &cfg, ESP8266WebServer &server);
void handleFactoryReset(ESP8266WebServer &server);
String generateIndexHTML(const M360::M360DeviceConfig &cfg);
