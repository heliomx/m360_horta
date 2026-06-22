/*
 * M360Translator.cpp — Implementação da tradução MySensors <-> JSON
 */

#ifdef ESP8266

#define MY_CORE_ONLY
#include <ESP8266WiFi.h>
#include "M360Translator.h"
#include "M360MQTT.h" // para MQTTMetrics se necessário
#include "M360Constants.h"

namespace M360 {

	String Translator::toJSON(const MyMessage& msg, bool isAck) {
		DynamicJsonDocument doc(DOC_SIZE_MSG);
		doc["nodeId"]	  = msg.getSender();
		doc["sensorId"]	= msg.getSensor();
		doc["command"]	 = msg.getCommand();
		doc["ack"]		 = isAck ? 1 : 0;
		doc["type"]		= msg.getType();
		doc["payload"]	 = msg.getString();
		doc["timestamp"]   = millis() / 1000;
		doc["description"] = getTypeDescription(msg.getType());
		doc["direction"]   = isAck ? "ack" : "sensor";

		String json;
		serializeJson(doc, json);
		return json;
	}

	String Translator::buildHeartbeat(const M360DeviceConfig& cfg, int rssi, const char* version) {
		DynamicJsonDocument doc(DOC_SIZE_HB);
		doc["nodeId"]	  = 0;
		doc["sensorId"]	= CHILD_ID_BATTERY;
		doc["command"]	 = 3;   // C_INTERNAL
		doc["ack"]		 = 0;
		doc["type"]		= 22;  // I_HEARTBEAT_RESPONSE
		doc["payload"]	 = "";
		doc["timestamp"]   = millis() / 1000;
		doc["description"] = "heartbeat";
		doc["event"]	   = "heartbeat";
		doc["uptime"]	  = millis() / 1000;
		doc["rssi"]		= rssi;
		doc["wifiRssi"]	= rssi;
		doc["batteryLevel"]= 100;
		doc["ip"]		  = WiFi.localIP().toString();
		doc["version"]	 = version;
		doc["source"]	  = "gateway";

		String json;
		serializeJson(doc, json);
		return json;
	}

	String Translator::buildEvent(const char* event, int nodeId, const char* details, int rssi) {
		DynamicJsonDocument doc(DOC_SIZE_EVENT);
		doc["event"]	 = event;
		doc["nodeId"]	= nodeId;
		doc["timestamp"] = millis() / 1000;
		doc["rssi"]	  = rssi;
		if (details && strlen(details) > 0) doc["details"] = details;

		String json;
		serializeJson(doc, json);
		return json;
	}

	String Translator::buildMetrics(const M360DeviceConfig& cfg, const struct MQTTMetrics& metrics) {
		DynamicJsonDocument doc(DOC_SIZE_MSG);
		doc["timestamp"] = millis() / 1000;
		doc["isConnected"] = metrics.isConnected;
		doc["connectionTime"] = metrics.connectionTime / 1000;
		doc["reconnectCount"] = metrics.reconnectCount;
		doc["lastErrorCode"] = metrics.lastErrorCode;
		
		if (metrics.lastErrorCode != 0) {
			doc["lastError"] = MQTTManager::getErrorDescription(metrics.lastErrorCode);
		}
		
		doc["uptime"] = millis() / 1000;
		doc["source"] = "gateway";
		doc["type"] = "mqtt_metrics";

		String json;
		serializeJson(doc, json);
		return json;
	}

	bool Translator::fromJSON(const String& json, MyMessage& outMsg, uint8_t& targetNode) {
		DynamicJsonDocument doc(DOC_SIZE_MSG);
		DeserializationError error = deserializeJson(doc, json);
		if (error) return false;

		if (!doc.containsKey("nodeId")) return false;
		int rawNodeId = doc["nodeId"].as<int>();
		if (rawNodeId < 1 || rawNodeId > 254) return false;
		targetNode = (uint8_t)rawNodeId;

		// Formato MySensors completo
		if (doc.containsKey("sensorId") && doc.containsKey("command") && doc.containsKey("type")) {
			int rawSensorId = doc["sensorId"].as<int>();
			if (rawSensorId < 0 || rawSensorId > 255) return false;
			outMsg.setSensor((uint8_t)rawSensorId);
			outMsg.setType(doc["type"]);
			outMsg.setCommand((mysensors_command_t)(int)doc["command"]);
			
			if (doc.containsKey("payload")) {
				outMsg.set(doc["payload"].as<const char*>());
			} else if (doc.containsKey("value")) {
				if (doc["value"].is<bool>())  outMsg.set(doc["value"].as<bool>());
				else if (doc["value"].is<int>())   outMsg.set(doc["value"].as<int>());
				else if (doc["value"].is<float>()) outMsg.set(doc["value"].as<float>(), 2);
			}
			return true;
		}

		// Formato simplificado (Actions)
		if (doc.containsKey("action")) {
			String action = doc["action"];
			
			if (action == CMD_PUMP_ON || action == "1") {
				outMsg.setSensor(0); outMsg.setCommand(C_SET); outMsg.setType(V_STATUS); outMsg.set(true);
			} else if (action == CMD_PUMP_OFF || action == "0") {
				outMsg.setSensor(0); outMsg.setCommand(C_SET); outMsg.setType(V_STATUS); outMsg.set(false);
			} else if (action == CMD_PUMP_TOGGLE) {
				outMsg.setSensor(0); outMsg.setCommand(C_SET); outMsg.setType(V_CUSTOM); outMsg.set(CMD_PUMP_TOGGLE);
			} else if (action == CMD_FORCE_UPDATE) {
				outMsg.setSensor(0); outMsg.setCommand(C_SET); outMsg.setType(V_CUSTOM); outMsg.set(CMD_FORCE_UPDATE);
			} else if (action == "SET_INTERVAL" && doc.containsKey("interval")) {
				outMsg.setSensor(CHILD_ID_INTERVAL); outMsg.setCommand(C_SET); outMsg.setType(V_VAR1); outMsg.set(doc["interval"].as<int>());
			} else {
				return false;
			}
			return true;
		}

		return false;
	}

	const char* Translator::getTypeDescription(uint8_t type) {
		switch (type) {
			case V_TEMP:	  return "Temperature";
			case V_HUM:	   return "Humidity";
			case V_STATUS:	return "Status";
			case V_PERCENTAGE:return "Percentage";
			case V_PRESSURE:  return "Pressure";
			case V_VOLTAGE:   return "Voltage";
			case V_CURRENT:   return "Current";
			case V_VAR1:	  return "Variable 1";
			case V_CUSTOM:	return "Custom";
			default:		  return "Unknown";
		}
	}

} // namespace M360

#endif // ESP8266
