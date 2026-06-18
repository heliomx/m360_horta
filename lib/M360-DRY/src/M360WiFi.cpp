/*
 * M360WiFi.cpp — Gerenciador de conexão WiFi para M360-DRY
 */

#ifdef ESP8266

#include "M360WiFi.h"

namespace M360 {

void WiFiManager::begin(const M360DeviceConfig& cfg) {
	Serial.print("📡 WiFi: Iniciando conexao para SSID '");
	Serial.print(cfg.ssid);
	Serial.println("'...");

	// Verificar modo AP forçado (ManualConfig)
	if (strcmp(cfg.ssid, "ManualConfig") == 0) {
		Serial.println("🔄 Modo AP: Detectado SSID padrão 'ManualConfig'.");
		WiFi.mode(WIFI_AP);
		WiFi.softAP("Manejo360-Config", "manejo360");
		Serial.print("   IP AP: ");
		Serial.println(WiFi.softAPIP());
		return;
	}

	WiFi.mode(WIFI_STA);
	WiFi.begin(cfg.ssid, cfg.password);

	// Tentar conectar 3 vezes (máximo 30s por tentativa)
	for (int attempt = 1; attempt <= 3; attempt++) {
		Serial.printf("🔍 WiFi: Tentativa %d/3...\n", attempt);
		WiFi.begin(cfg.ssid, cfg.password);
		
		unsigned long start = millis();
		while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
			delay(1000);
			Serial.print(".");
		}
		Serial.println();

		if (WiFi.status() == WL_CONNECTED) {
			Serial.println("✅ WiFi: Conectado com sucesso!");
			Serial.print("   IP STA: ");
			Serial.println(WiFi.localIP());
			return;
		}
		
		Serial.printf("❌ WiFi: Falha na tentativa %d.\n", attempt);
		if (attempt < 3) delay(3000);
	}

	// Se falhar todas as tentativas, ativa AP
	Serial.println("🔄 WiFi: 3 falhas consecutivas. Entrando em modo AP.");
	WiFi.mode(WIFI_AP);
	WiFi.softAP("Manejo360-Config", "manejo360");
	Serial.print("   IP AP: ");
	Serial.println(WiFi.softAPIP());
}

void WiFiManager::process(const M360DeviceConfig& cfg) {
	// Não reconectar se estiver deliberadamente em modo AP
	if (isAPMode()) return;

	unsigned long now = millis();
	
	if (now - _lastCheck > _checkInterval) {
		_lastCheck = now;

		if (WiFi.status() != WL_CONNECTED) {
			if (_reconnectAttempts < _maxReconnectAttempts) {
				Serial.printf("🔄 WiFi: Reconectando... (tentativa %d/%d)\n", 
				              _reconnectAttempts + 1, _maxReconnectAttempts);
				WiFi.disconnect();
				WiFi.begin(cfg.ssid, cfg.password);
				_reconnectAttempts++;
			} else {
				Serial.println("🔄 WiFi: Máximo de reconexões excedido. Mudando para modo AP.");
				WiFi.mode(WIFI_AP);
				WiFi.softAP("Manejo360-Config", "manejo360");
				Serial.print("   IP AP: ");
				Serial.println(WiFi.softAPIP());
			}
		} else {
			// Resetar contador se estiver estável
			_reconnectAttempts = 0;
		}
	}
}

bool WiFiManager::isAPMode() const {
	return WiFi.getMode() == WIFI_AP;
}

} // namespace M360

#endif // ESP8266
