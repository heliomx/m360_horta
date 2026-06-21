#include "mqtt_utils.h"
#include "leds.h"

// Variáveis estáticas para métricas e controle de reconexão
static MQTTMetrics metrics = {0, 0, 0, 0, false};
static unsigned long lastAttempt = 0;
static unsigned long retryInterval = 5000;  // Intervalo inicial: 5s
static unsigned long connectionStartTime = 0;
static bool wasConnected = false;

// Backoff exponencial: 5s → 10s → 20s → 40s → 60s (max)
static const unsigned long RETRY_INTERVALS[] = {5000, 10000, 20000, 40000, 60000};
static const int MAX_RETRY_LEVEL = 4;
static int currentRetryLevel = 0;

void setupMQTT(const DeviceConfig &cfg, PubSubClient &client) {
  client.setServer(cfg.mqttServer, cfg.mqttPort);
  client.setCallback(mqttCallback);
  
  // Inicializar métricas
  metrics.isConnected = false;
  metrics.reconnectCount = 0;
  metrics.connectionTime = 0;
  metrics.lastFailureTime = 0;
  metrics.lastErrorCode = 0;
  wasConnected = false;
  currentRetryLevel = 0;
  retryInterval = RETRY_INTERVALS[0];
}

void handleMQTTReconnect(const DeviceConfig &cfg, PubSubClient &client, unsigned long timeoutMs) {
  bool currentlyConnected = client.connected();
  
  // Detectar mudança de estado de conexão
  if (currentlyConnected && !wasConnected) {
    // Conexão estabelecida
    metrics.isConnected = true;
    metrics.connectionTime = millis();
    connectionStartTime = millis();
    metrics.lastErrorCode = 0;
    
    // Resetar backoff após conexão bem-sucedida
    currentRetryLevel = 0;
    retryInterval = RETRY_INTERVALS[0];
    
    Serial.println("✅ MQTT conectado - métricas resetadas");
  } else if (!currentlyConnected && wasConnected) {
    // Conexão perdida
    metrics.isConnected = false;
    metrics.lastFailureTime = millis();
    metrics.reconnectCount++;
    
    Serial.print("⚠️ MQTT desconectado. Total de reconexões: ");
    Serial.println(metrics.reconnectCount);
  }
  
  wasConnected = currentlyConnected;
  metrics.isConnected = currentlyConnected;
  
  // Tentar reconectar se desconectado
  if (!currentlyConnected && (millis() - lastAttempt >= retryInterval)) {
    Serial.print("🔌 Reconectando MQTT (tentativa ");
    Serial.print(metrics.reconnectCount + 1);
    Serial.print(", intervalo: ");
    Serial.print(retryInterval / 1000);
    Serial.println("s)... ");
    
    // Tentar conexão (PubSubClient já tem timeout interno)
    // Client ID único baseado em UF + carNumber para evitar conflito entre gateways
    String clientId = "M360-";
    clientId += cfg.uf;
    clientId += "-";
    clientId += cfg.carNumber;
    bool connected = client.connect(clientId.c_str(), cfg.mqttUser, cfg.mqttPassword);
    
    if (connected) {
      Serial.println("✅ Conectado!");
      
      // Atualizar métricas
      metrics.isConnected = true;
      metrics.connectionTime = millis();
      connectionStartTime = millis();
      metrics.lastErrorCode = 0;
      
      // Resetar backoff após conexão bem-sucedida
      currentRetryLevel = 0;
      retryInterval = RETRY_INTERVALS[0];
      
      // Assinar tópico IN após reconexão
      String topicIn = buildTopicIn(cfg);
      if (client.subscribe(topicIn.c_str())) {
        Serial.print("📡 Assinado tópico IN: ");
        Serial.println(topicIn);
      } else {
        Serial.println("❌ Falha ao assinar tópico IN");
      }
      
      // Publicar métricas após reconexão bem-sucedida
      publishMQTTMetrics(cfg, client);
    } else {
      int errorCode = client.state();
      metrics.lastErrorCode = errorCode;
      metrics.lastFailureTime = millis();
      
      const char* errorDesc = getMQTTErrorDescription(errorCode);
      Serial.print("❌ Falha na conexão (");
      Serial.print(errorCode);
      Serial.print(": ");
      Serial.print(errorDesc);
      Serial.println(")");
      
      // Aumentar backoff exponencial
      if (currentRetryLevel < MAX_RETRY_LEVEL) {
        currentRetryLevel++;
      }
      retryInterval = RETRY_INTERVALS[currentRetryLevel];
      
      Serial.print("   Próxima tentativa em ");
      Serial.print(retryInterval / 1000);
      Serial.println("s");
    }
    
    lastAttempt = millis();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  ledFlicker(LED_GREEN); // Gateway transport RX (MySensors: INDICATION_GW_RX)
  Serial.println("📨 Comando MQTT recebido:");
  Serial.print("   Tópico: ");
  Serial.println(topic);

  // P5: deserializa diretamente do buffer payload, sem construir String intermediária
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("❌ Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Processar comando
  processMQTTCommand(doc);
}

// Obter métricas atuais
MQTTMetrics getMQTTMetrics() {
  // Atualizar tempo de conexão se conectado
  if (metrics.isConnected && connectionStartTime > 0) {
    metrics.connectionTime = millis() - connectionStartTime;
  }
  return metrics;
}

// Publicar métricas via MQTT
void publishMQTTMetrics(const DeviceConfig &cfg, PubSubClient &client) {
  if (!client.connected()) {
    return;
  }
  
  MQTTMetrics currentMetrics = getMQTTMetrics();
  
  // Construir tópico de status: m360/{UF}/{nrCar}/gateway/status
  String topicStatus = "m360/";
  topicStatus += cfg.uf;
  topicStatus += "/";
  topicStatus += cfg.carNumber;
  topicStatus += "/gateway/status";
  
  // Criar documento JSON com métricas
  DynamicJsonDocument doc(512);
  doc["timestamp"] = millis() / 1000;
  doc["isConnected"] = currentMetrics.isConnected;
  doc["connectionTime"] = currentMetrics.connectionTime / 1000; // em segundos
  doc["reconnectCount"] = currentMetrics.reconnectCount;
  doc["lastErrorCode"] = currentMetrics.lastErrorCode;
  
  if (currentMetrics.lastErrorCode != 0) {
    doc["lastError"] = getMQTTErrorDescription(currentMetrics.lastErrorCode);
  }
  
  if (currentMetrics.lastFailureTime > 0) {
    doc["lastFailureTime"] = currentMetrics.lastFailureTime / 1000; // em segundos
    doc["timeSinceLastFailure"] = (millis() - currentMetrics.lastFailureTime) / 1000; // em segundos
  }
  
  // Adicionar informações adicionais
  doc["uptime"] = millis() / 1000;
  doc["source"] = "gateway";
  doc["type"] = "mqtt_metrics";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  bool published = client.publish(topicStatus.c_str(), jsonString.c_str());
  if (published) {
    ledFlicker(LED_YELLOW); // Pisca ao enviar métricas
    Serial.print("📊 Métricas MQTT publicadas em: ");
    Serial.println(topicStatus);
  } else {
    Serial.println("❌ Falha ao publicar métricas MQTT");
  }
}

// Obter descrição do erro MQTT
const char* getMQTTErrorDescription(int errorCode) {
  switch (errorCode) {
    case -4: return "MQTT_CONNECTION_TIMEOUT";
    case -3: return "MQTT_CONNECTION_LOST";
    case -2: return "MQTT_CONNECT_FAILED";
    case -1: return "MQTT_DISCONNECTED";
    case 0: return "MQTT_CONNECTED";
    case 1: return "MQTT_CONNECT_BAD_PROTOCOL";
    case 2: return "MQTT_CONNECT_BAD_CLIENT_ID";
    case 3: return "MQTT_CONNECT_UNAVAILABLE";
    case 4: return "MQTT_CONNECT_BAD_CREDENTIALS";
    case 5: return "MQTT_CONNECT_UNAUTHORIZED";
    default: return "MQTT_UNKNOWN_ERROR";
  }
}

