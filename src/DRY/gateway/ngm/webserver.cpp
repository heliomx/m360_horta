#include "webserver.h"

String generateIndexHTML(const M360::M360DeviceConfig &cfg) {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Manejo360 Gateway</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;}";
  html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
  html += "h2{color:#333;text-align:center;}";
  html += "label{display:block;margin-top:10px;font-weight:bold;color:#555;}";
  html += "input,select{width:100%;padding:8px;margin:5px 0 10px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}";
  html += "input[type='submit']{background:#4CAF50;color:white;padding:12px 20px;border:none;border-radius:4px;cursor:pointer;font-size:16px;margin-top:10px;}";
  html += ".status{background:#e8f4fd;padding:15px;border-radius:5px;margin-bottom:20px;}";
  html += ".network-info{font-size:12px;color:#666;margin-left:10px;}";
  html += "</style></head><body><div class='container'>";
  html += "<h2>🌾 Manejo360 Gateway MQTT</h2>";

  html += "<div class='status'>";
  html += "<h3>📊 Status Atual:</h3>";
  bool isInAPMode = (WiFi.getMode() == WIFI_AP);
  if (isInAPMode) {
    html += "<p><strong>Status:</strong> 🔧 Modo Configuração</p>";
    html += "<p><strong>IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
  } else {
    html += "<p><strong>Status:</strong> ❌ OFFLINE</p>";
  }
  html += "<p><strong>SSID Atual:</strong> " + String(cfg.ssid) + "</p>";
  html += "<p><strong>MQTT Server:</strong> " + String(cfg.mqttServer) + "</p>";
  html += "<p><strong>UF:</strong> " + String(cfg.uf) + "</p>";
  html += "<p><strong>CAR:</strong> " + String(cfg.carNumber) + "</p>";
  html += "<p><strong>Tópico OUT:</strong> " + M360::buildTopicOut(cfg) + "</p>";
  html += "<p><strong>Tópico IN:</strong> " + M360::buildTopicIn(cfg) + "</p>";
  html += "</div>";

  html += "<form action='/save' method='POST'>";
  html += "<label>SSID da Rede WiFi:</label>";

  int n = WiFi.scanComplete();
  html += "<select name='ssid' required>";

  if (n == WIFI_SCAN_RUNNING) {
    html += "<option value='' disabled selected>🔄 Scan em andamento...</option>";
    if (strlen(cfg.ssid) > 0) {
      html += "<option value='" + String(cfg.ssid) + "' selected>" + String(cfg.ssid) + " (configurado)</option>";
    }
  } else if (n <= 0) {
    html += "<option value='' disabled selected>Nenhuma rede encontrada</option>";
    if (strlen(cfg.ssid) > 0) {
      html += "<option value='" + String(cfg.ssid) + "' selected>" + String(cfg.ssid) + " (configurado)</option>";
    }
  } else {
    Serial.print("🔍 Redes encontradas (scan async): ");
    Serial.println(n);

    const int MAX_NETWORKS = 20;
    int sortedIndices[MAX_NETWORKS];
    int netCount = (n < MAX_NETWORKS) ? n : MAX_NETWORKS;
    for (int i = 0; i < netCount; i++) sortedIndices[i] = i;
    for (int i = 0; i < netCount - 1; i++) {
      for (int j = 0; j < netCount - i - 1; j++) {
        if (WiFi.RSSI(sortedIndices[j]) < WiFi.RSSI(sortedIndices[j + 1])) {
          int temp = sortedIndices[j];
          sortedIndices[j] = sortedIndices[j + 1];
          sortedIndices[j + 1] = temp;
        }
      }
    }

    bool currentSSIDFound = false;
    for (int i = 0; i < netCount; i++) {
      if (WiFi.SSID(sortedIndices[i]) == String(cfg.ssid)) { currentSSIDFound = true; break; }
    }
    if (!currentSSIDFound && strlen(cfg.ssid) > 0) {
      html += "<option value='" + String(cfg.ssid) + "' selected>" + String(cfg.ssid) + " (configurado)</option>";
    }

    for (int i = 0; i < netCount; i++) {
      int idx = sortedIndices[i];
      String ssid = WiFi.SSID(idx);
      int rssi = WiFi.RSSI(idx);
      String encType = (WiFi.encryptionType(idx) == ENC_TYPE_NONE) ? "🔓" : "🔒";
      String signal = rssi > -50 ? "📶▂▄▆█" : rssi > -60 ? "📶▂▄▆" : rssi > -70 ? "📶▂▄" : rssi > -80 ? "📶▂" : "📶";
      bool isSelected = (ssid == String(cfg.ssid));
      html += "<option value='" + ssid + "'";
      if (isSelected) html += " selected";
      html += ">" + ssid + " " + encType + " (" + String(rssi) + " dBm) " + signal + "</option>";
      Serial.printf("   %d: %s (%d dBm)\n", i, ssid.c_str(), rssi);
    }

    WiFi.scanDelete();
    WiFi.scanNetworks(/*async=*/true);
  }

  html += "</select>";
  html += "<label>Senha da Rede WiFi:</label>";
  html += "<input name='password' type='password' placeholder='Manter senha atual'>";
  html += "<label>Servidor MQTT:</label>";
  html += "<input name='mqttServer' value='" + String(cfg.mqttServer) + "' required>";
  html += "<label>Porta MQTT:</label>";
  html += "<input name='mqttPort' type='number' value='" + String(cfg.mqttPort) + "' required>";
  html += "<label>Usuário MQTT:</label>";
  html += "<input name='mqttUser' value='" + String(cfg.mqttUser) + "'>";
  html += "<label>Senha MQTT:</label>";
  html += "<input name='mqttPassword' type='password' placeholder='Manter senha atual'>";
  html += "<label>UF (Unidade Federativa):</label>";
  html += "<input name='uf' value='" + String(cfg.uf) + "' maxlength='2' required>";
  html += "<label>Número do CAR:</label>";
  html += "<input name='carNumber' value='" + String(cfg.carNumber) + "' maxlength='8' required>";
  html += "<input type='submit' value='💾 Salvar Configurações'>";
  html += "</form></div></body></html>";

  return html;
}

void handleRoot(M360::M360DeviceConfig &cfg, ESP8266WebServer &server) {
  int scanState = WiFi.scanComplete();
  if (scanState == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(/*async=*/true);
  }
  String html = generateIndexHTML(cfg);
  server.send(200, "text/html", html);
}

void handleSave(M360::M360DeviceConfig &cfg, ESP8266WebServer &server) {
  Serial.println("💾 Salvando configuração...");

  String ssidStr        = server.arg("ssid");
  String passwordStr    = server.arg("password");
  String mqttServerStr  = server.arg("mqttServer");
  String mqttUserStr    = server.arg("mqttUser");
  String mqttPasswordStr = server.arg("mqttPassword");
  String ufStr          = server.arg("uf");
  String carNumberStr   = server.arg("carNumber");

  if (ssidStr.length() == 0) {
    server.send(400, "text/html", "<h3>❌ Erro: SSID não pode estar vazio!</h3><a href='/'>Voltar</a>");
    return;
  }
  if (mqttServerStr.length() == 0) {
    server.send(400, "text/html", "<h3>❌ Erro: MQTT Server não pode estar vazio!</h3><a href='/'>Voltar</a>");
    return;
  }
  if (ufStr.length() == 0) {
    server.send(400, "text/html", "<h3>❌ Erro: UF não pode estar vazio!</h3><a href='/'>Voltar</a>");
    return;
  }
  if (carNumberStr.length() == 0) {
    server.send(400, "text/html", "<h3>❌ Erro: Número do CAR não pode estar vazio!</h3><a href='/'>Voltar</a>");
    return;
  }
  int port = server.arg("mqttPort").toInt();
  if (port <= 0 || port > 65535) {
    server.send(400, "text/html", "<h3>❌ Erro: Porta MQTT deve estar entre 1 e 65535!</h3><a href='/'>Voltar</a>");
    return;
  }

  strncpy(cfg.ssid, ssidStr.c_str(), sizeof(cfg.ssid) - 1);
  cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
  if (passwordStr.length() > 0) {
    strncpy(cfg.password, passwordStr.c_str(), sizeof(cfg.password) - 1);
    cfg.password[sizeof(cfg.password) - 1] = '\0';
  }
  strncpy(cfg.mqttServer, mqttServerStr.c_str(), sizeof(cfg.mqttServer) - 1);
  cfg.mqttServer[sizeof(cfg.mqttServer) - 1] = '\0';
  cfg.mqttPort = (uint16_t)port;
  strncpy(cfg.mqttUser, mqttUserStr.c_str(), sizeof(cfg.mqttUser) - 1);
  cfg.mqttUser[sizeof(cfg.mqttUser) - 1] = '\0';
  if (mqttPasswordStr.length() > 0) {
    strncpy(cfg.mqttPassword, mqttPasswordStr.c_str(), sizeof(cfg.mqttPassword) - 1);
    cfg.mqttPassword[sizeof(cfg.mqttPassword) - 1] = '\0';
  }
  strncpy(cfg.uf, ufStr.c_str(), sizeof(cfg.uf) - 1);
  cfg.uf[sizeof(cfg.uf) - 1] = '\0';
  strncpy(cfg.carNumber, carNumberStr.c_str(), sizeof(cfg.carNumber) - 1);
  cfg.carNumber[sizeof(cfg.carNumber) - 1] = '\0';

  Serial.printf("   SSID='%s' MQTT='%s:%d' UF='%s' CAR='%s'\n",
                cfg.ssid, cfg.mqttServer, cfg.mqttPort, cfg.uf, cfg.carNumber);

  M360::Config::save(cfg);
  server.send(200, "text/html", "<h3>✅ Configuração salva com sucesso! Reiniciando...</h3>");
  delay(1000);
  ESP.restart();
}

void setupWebServer(M360::M360DeviceConfig &cfg, ESP8266WebServer &server) {
  server.on("/", [&]() { handleRoot(cfg, server); });
  server.on("/save", HTTP_POST, [&]() { handleSave(cfg, server); });
  server.begin();
  Serial.println("🌐 Servidor Web iniciado em porta 80");
}
