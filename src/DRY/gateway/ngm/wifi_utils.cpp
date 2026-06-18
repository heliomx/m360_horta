#include "wifi_utils.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
// ── Ponto único de ativação do AP de configuração ───────────────────────────
// Todos os prints e a chamada softAP ficam aqui.
// AP_SSID e AP_PASSWORD são definidos em config_utils.h
void startConfigAP() {
  // Reset completo do stack WiFi antes de criar o AP.
  // Sem isso, vindo de uma tentativa STA fracassada o driver fica com estado
  // residual e o softAP não autentica corretamente (senha recusada).
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_AP);
  delay(100);

  IPAddress apIP(192, 168, 4, 1);
  bool result = WiFi.softAP(AP_SSID, AP_PASSWORD, 1); // canal 1, fixo
  delay(100); // aguardar AP subir antes de reconfigurar IP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  if (result) {
    Serial.println("📡 Access Point de configuração ativo");
    Serial.println("   SSID : " AP_SSID);
    Serial.println("   Senha: " AP_PASSWORD);
    Serial.print  ("   IP   : ");
    Serial.println(WiFi.softAPIP());
    Serial.println("🌐 Acesse o IP acima para configurar o dispositivo");
  } else {
    Serial.println("❌ FALHA ao iniciar AP de configuração!");
  }
}
// ────────────────────────────────────────────────────────────────────────────

void setupWiFi(const DeviceConfig &cfg) {
  Serial.print("Conectando a: ");
  Serial.println(cfg.ssid);
  Serial.print("Senha sendo usada: ");
  Serial.println(cfg.password);

  // Verificar se é primeira execução (SSID padrão)
  bool isFirstRun = strcmp(cfg.ssid, "ManualConfig") == 0;

  if (isFirstRun) {
    // Primeira execução - ativar modo AP
    Serial.println("Primeira execucao detectada - modo AP");
    startConfigAP();
    return;
  }

  // Tentar conectar ao WiFi configurado com 3 tentativas
  WiFi.mode(WIFI_STA);

  unsigned long lastDot = millis();

  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.printf("Tentativa de conexao WiFi %d/3...\n", attempt);
    WiFi.begin(cfg.ssid, cfg.password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 30000) { // 30 segundos por tentativa
      delay(1000);
      if (millis() - lastDot >= 2000) { // a cada ~2 s, robusto a jitter
        Serial.print(".");
        lastDot += 2000;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi conectado com sucesso!");
      Serial.print("📍 IP: ");
      Serial.println(WiFi.localIP());
      return;
    } else {
      Serial.printf("\n❌ Falha na tentativa %d/3\n", attempt);
      Serial.print("Status do WiFi: ");
      Serial.println(WiFi.status());
      if (attempt < 3) {
        Serial.println(
            "⏳ Aguardando 3 segundos antes da proxima tentativa...");
        delay(3000);
      }
    }
  }

  // Falhou nas 3 tentativas — entrar em modo AP
  Serial.println("\n🔄 3 tentativas de conexao WiFi falharam");
  startConfigAP();
}

void handleWiFiReconnect(const DeviceConfig &cfg) {
  // NÃO tentar reconectar se estiver em modo AP
  if (WiFi.getMode() == WIFI_AP) {
    return;
  }

  static unsigned long lastCheck = 0;
  static int reconnectAttempts = 0;
  static const int MAX_RECONNECT_ATTEMPTS = 3;

  if (millis() - lastCheck > 15000) { // Verificar a cada 15 segundos
    if (WiFi.status() != WL_CONNECTED &&
        reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
      Serial.printf("🔄 Reconectando WiFi... (tentativa %d/%d)\n",
                    reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
      WiFi.disconnect();
      WiFi.begin(cfg.ssid, cfg.password);
      reconnectAttempts++;
    } else if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
      Serial.println("🔄 Maximo de tentativas de reconexao atingido");
      startConfigAP();
      reconnectAttempts = 0; // reset para próxima sessão
    }
    lastCheck = millis();
  }
}
