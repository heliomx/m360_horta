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

	String Translator::toJSON(const MyMessage& msg, bool isAck, int nodeIdOverride) {
		DynamicJsonDocument doc(DOC_SIZE_MSG);
		doc["nodeId"]	   = (nodeIdOverride >= 0) ? (uint8_t)nodeIdOverride : msg.getSender();
		doc["sensorId"]	 = msg.getSensor();
		doc["destination"] = msg.getDestination();
		doc["command"]	 = msg.getCommand();
		doc["ack"]		 = isAck ? 1 : 0;
		doc["type"]		= msg.getType();
		doc["payloadType"] = msg.getPayloadType();
		// getString() sem buffer retorna nullptr para payloads não-string (P_FLOAT32,
		// P_LONG32, etc.), gerando payload:null. getString(buf) converte qualquer tipo.
		char payloadBuf[MAX_PAYLOAD + 1];
		doc["payload"]	 = msg.getString(payloadBuf);
		doc["timestamp"]   = millis() / 1000;
		doc["description"] = getTypeDescription(msg.getCommand(), msg.getType());
		doc["direction"]   = isAck ? "ack" : "sensor";

		String json;
		serializeJson(doc, json);
		return json;
	}

	String Translator::buildHeartbeat(const M360DeviceConfig& cfg, int rssi, const char* version) {
		DynamicJsonDocument doc(DOC_SIZE_HB);
		doc["nodeId"]	  = 0;
		doc["sensorId"]	= CHILD_ID_BATTERY;
		doc["command"]	 = C_INTERNAL;
		doc["ack"]		 = 0;
		doc["type"]		= I_HEARTBEAT_RESPONSE;
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

	M360CommandStatus Translator::validate(const MyMessage& msg, uint8_t targetNode) {
		// 0 é o próprio gateway; 255 é broadcast MySensors e é legítimo.
		if (targetNode < 1) {
			return M360_CMD_ERR_NODE_ID;
		}

		const uint8_t cmd = msg.getCommand();
		if (cmd != C_SET && cmd != C_REQ && cmd != C_INTERNAL) {
			return M360_CMD_ERR_COMMAND;
		}

		// C_SET/V_STATUS é o caminho de atuação. O nó resolve o valor com
		// MyMessage::getBool(), que para payload P_STRING faz atoi(): "true",
		// "ON" ou qualquer lixo viram 0 e DESLIGAM o relé em vez de ligar,
		// indistinguíveis de um OFF legítimo em qualquer log. Só "0" e "1".
		if (cmd == C_SET && msg.getType() == V_STATUS) {
			char buf[MAX_PAYLOAD_SIZE + 1];
			const char* value = msg.getString(buf);
			if (value == NULL ||
			    (strcmp(value, "0") != 0 && strcmp(value, "1") != 0)) {
				return M360_CMD_ERR_PAYLOAD_VALUE;
			}
		}

		return M360_CMD_OK;
	}

	M360CommandStatus Translator::fromNative(int nodeId, int sensorId,
	                                         int command, int type,
	                                         const char* payload,
	                                         MyMessage& outMsg,
	                                         uint8_t& targetNode) {
		if (nodeId < 1 || nodeId > 255) {
			return M360_CMD_ERR_NODE_ID;
		}
		if (sensorId < 0 || sensorId > 255) {
			return M360_CMD_ERR_SENSOR_ID;
		}
		if (command != C_SET && command != C_REQ && command != C_INTERNAL) {
			return M360_CMD_ERR_COMMAND;
		}
		if (type < 0 || type > 255) {
			return M360_CMD_ERR_TYPE;
		}
		// MyMessage::set() trunca em MAX_PAYLOAD_SIZE sem avisar. Rejeitar é
		// melhor que enviar meia string: nenhum comando legítimo chega perto
		// (o mais longo, "FORCE_UPDATE", tem 12 bytes).
		if (payload != NULL && strlen(payload) > MAX_PAYLOAD_SIZE) {
			return M360_CMD_ERR_PAYLOAD_SIZE;
		}

		targetNode = (uint8_t)nodeId;
		outMsg.setSensor((uint8_t)sensorId);
		outMsg.setCommand((mysensors_command_t)command);
		outMsg.setType((uint8_t)type);
		outMsg.set(payload != NULL ? payload : "");

		return validate(outMsg, targetNode);
	}

	const char* Translator::describeStatus(M360CommandStatus status) {
		switch (status) {
			case M360_CMD_OK:                return "ok";
			case M360_CMD_ERR_NODE_ID:       return "nodeId fora de faixa (1-255)";
			case M360_CMD_ERR_SENSOR_ID:     return "sensorId fora de faixa (0-255)";
			case M360_CMD_ERR_COMMAND:       return "command invalido (1=SET, 2=REQ, 3=INTERNAL)";
			case M360_CMD_ERR_TYPE:          return "type fora de faixa (0-255)";
			case M360_CMD_ERR_PAYLOAD_SIZE:  return "payload excede MAX_PAYLOAD_SIZE";
			case M360_CMD_ERR_PAYLOAD_VALUE: return "payload de V_STATUS deve ser 0 ou 1";
			default:                         return "erro desconhecido";
		}
	}

	bool Translator::fromJSON(const String& json, MyMessage& outMsg, uint8_t& targetNode) {
		DynamicJsonDocument doc(DOC_SIZE_MSG);
		DeserializationError error = deserializeJson(doc, json);
		if (error) return false;

		if (!doc.containsKey("nodeId")) return false;
		int rawNodeId = doc["nodeId"].as<int>();
		if (rawNodeId < 1 || rawNodeId > 255) return false; // 255 = broadcast MySensors
		targetNode = (uint8_t)rawNodeId;

		// Formato MySensors completo (sensorId é opcional; default=0 para broadcast/comando geral)
		if (doc.containsKey("command") && doc.containsKey("type")) {
			int rawSensorId = doc.containsKey("sensorId") ? doc["sensorId"].as<int>() : 0;
			if (rawSensorId < 0 || rawSensorId > 255) return false;
			outMsg.setSensor((uint8_t)rawSensorId);
			outMsg.setType(doc["type"]);
			outMsg.setCommand((mysensors_command_t)(int)doc["command"]);
			
			if (doc.containsKey("payload")) {
				// as<const char*>() devolve NULL quando o payload não é string
				// (ex.: {"payload": 1}), e MyMessage::set(NULL) faz
				// strncpy(data, NULL, 0) — UB formal, e resultado vazio que o
				// nó lê como 0. Converter por String cobre número, bool e texto.
				JsonVariantConst pv = doc["payload"];
				if (pv.isNull()) {
					return false;
				}
				if (pv.is<bool>()) {
					// "true"/"false" seriam lidos como 0 pelo atoi() do nó.
					outMsg.set(pv.as<bool>() ? "1" : "0");
				} else {
					outMsg.set(pv.as<String>().c_str());
				}
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

	String Translator::buildNativeTopic(const String& prefix, const MyMessage& msg) {
		String topic = prefix;
		topic += '/'; topic += msg.getSender();
		topic += '/'; topic += msg.getSensor();
		topic += '/'; topic += (uint8_t)msg.getCommand();
		topic += '/'; topic += (msg.isAck() ? 1 : 0);
		topic += '/'; topic += msg.getType();
		return topic;
	}

	String Translator::toNativePayload(const MyMessage& msg) {
		char buf[MAX_PAYLOAD + 1];
		return String(msg.getString(buf));
	}

	// V_* e I_* compartilham o mesmo espaço numérico (ex: V_TEMP=0 == I_BATTERY_LEVEL=0),
	// por isso só é seguro resolver a descrição de mensagens C_INTERNAL separadamente.
	static const char* getInternalTypeDescription(uint8_t type) {
		switch (type) {
			case I_BATTERY_LEVEL:         return "Battery Level";
			case I_TIME:                  return "Time";
			case I_VERSION:               return "Version";
			case I_ID_REQUEST:            return "ID Request";
			case I_ID_RESPONSE:           return "ID Response";
			case I_CONFIG:                return "Config Request";
			case I_PRESENTATION:          return "Presentation";
			case I_HEARTBEAT_REQUEST:     return "Heartbeat Request";
			case I_HEARTBEAT_RESPONSE:    return "Heartbeat Response";
			case I_DISCOVER_REQUEST:      return "Discover Request";
			case I_DISCOVER_RESPONSE:     return "Discover Response";
			case I_REGISTRATION_REQUEST:  return "Registration Request";
			case I_REGISTRATION_RESPONSE: return "Registration Response";
			case I_LOG_MESSAGE:           return "Log Message";
			case I_REBOOT:                return "Reboot";
			default:                      return "Internal";
		}
	}

	const char* Translator::getTypeDescription(uint8_t command, uint8_t type) {
		if (command == C_INTERNAL) {
			return getInternalTypeDescription(type);
		}
		switch (type) {
			case V_TEMP:              return "Temperature";
			case V_HUM:               return "Humidity";
			case V_STATUS:            return "Status";       // V_LIGHT alias (=2)
			case V_PERCENTAGE:        return "Percentage";   // V_DIMMER alias (=3)
			case V_PRESSURE:          return "Pressure";
			case V_FORECAST:          return "Forecast";
			case V_RAIN:              return "Rain";
			case V_RAINRATE:          return "Rain Rate";
			case V_WIND:              return "Wind Speed";
			case V_GUST:              return "Wind Gust";
			case V_DIRECTION:         return "Wind Direction";
			case V_UV:                return "UV Index";
			case V_WEIGHT:            return "Weight";
			case V_DISTANCE:          return "Distance";
			case V_IMPEDANCE:         return "Impedance";
			case V_ARMED:             return "Armed";
			case V_TRIPPED:           return "Tripped";
			case V_WATT:              return "Power (W)";
			case V_KWH:               return "Energy (kWh)";
			case V_SCENE_ON:          return "Scene On";
			case V_SCENE_OFF:         return "Scene Off";
			case V_HVAC_FLOW_STATE:   return "HVAC Flow State"; // V_HEATER alias (=21)
			case V_HVAC_SPEED:        return "HVAC Speed";
			case V_LIGHT_LEVEL:       return "Light Level (%)";
			case V_VAR1:              return "Variable 1";
			case V_VAR2:              return "Variable 2";
			case V_VAR3:              return "Variable 3";
			case V_VAR4:              return "Variable 4";
			case V_VAR5:              return "Variable 5";
			case V_UP:                return "Up";
			case V_DOWN:              return "Down";
			case V_STOP:              return "Stop";
			case V_IR_SEND:           return "IR Send";
			case V_IR_RECEIVE:        return "IR Receive";
			case V_FLOW:              return "Flow";
			case V_VOLUME:            return "Volume";
			case V_LOCK_STATUS:       return "Lock Status";
			case V_LEVEL:             return "Level";
			case V_VOLTAGE:           return "Voltage";
			case V_CURRENT:           return "Current";
			case V_RGB:               return "RGB";
			case V_RGBW:              return "RGBW";
			case V_ID:                return "ID";
			case V_UNIT_PREFIX:       return "Unit Prefix";
			case V_HVAC_SETPOINT_COOL:return "HVAC Cool Setpoint";
			case V_HVAC_SETPOINT_HEAT:return "HVAC Heat Setpoint";
			case V_HVAC_FLOW_MODE:    return "HVAC Flow Mode";
			case V_TEXT:              return "Text";
			case V_CUSTOM:            return "Custom";
			case V_POSITION:          return "GPS Position";
			case V_IR_RECORD:         return "IR Record";
			case V_PH:                return "pH";
			case V_ORP:               return "ORP";
			case V_EC:                return "EC";
			case V_VAR:               return "Reactive Power (var)";
			case V_VA:                return "Apparent Power (VA)";
			case V_POWER_FACTOR:      return "Power Factor";
			default:                  return "Unknown";
		}
	}

	String Translator::formatHumanDescription(const NodeRegistry& registry,
	                                          uint8_t nodeId,
	                                          uint8_t childId,
	                                          uint8_t command,
	                                          uint8_t type,
	                                          const char* payload,
	                                          bool isSending) {
		// 1. Identificação amigável do Nó
		const char* nodeNameFound = registry.getNodeName(nodeId);
		String nodeStr;
		if (nodeNameFound != nullptr && strlen(nodeNameFound) > 0) {
			nodeStr = String(nodeNameFound) + " [Nó " + String(nodeId) + "]";
		} else {
			nodeStr = "Nó " + String(nodeId);
		}

		// 2. Identificação amigável do Child (sensor/atuador)
		const char* childLabelFound = registry.getChildLabel(nodeId, childId);
		String childStr;
		if (childLabelFound != nullptr && strlen(childLabelFound) > 0) {
			childStr = String(childLabelFound);
		} else {
			if (childId == CHILD_ID_DEBUG) childStr = "Debug Remoto";
			else if (childId == CHILD_ID_INTERVAL) childStr = "Intervalo de Reporte";
			else if (childId == CHILD_ID_BATTERY) childStr = "Nível de Bateria";
			else childStr = "Child " + String(childId);
		}

		const char* p = (payload != nullptr) ? payload : "";
		String actionDesc;

		if (command == C_INTERNAL) {
			actionDesc = String("Mensagem interna ") + getInternalTypeDescription(type);
			if (strlen(p) > 0) {
				actionDesc += " (" + String(p) + ")";
			}
		} else if (type == V_STATUS) {
			bool isOne = (strcmp(p, "1") == 0 || strcmp(p, "true") == 0);
			if (isSending) {
				actionDesc = (isOne ? "Ligar " : "Desligar ") + childStr + " (Child " + String(childId) + ")";
			} else {
				actionDesc = childStr + (isOne ? " ligada/acionada" : " desligada/desativada") + " (Child " + String(childId) + ")";
			}
		} else if (type == V_FLOW) {
			actionDesc = childStr + " = " + String(p) + " L/min (Child " + String(childId) + ")";
		} else if (type == V_TEMP) {
			actionDesc = childStr + " = " + String(p) + " °C (Child " + String(childId) + ")";
		} else if (type == V_HUM || type == V_LEVEL) {
			actionDesc = childStr + " = " + String(p) + " % (Child " + String(childId) + ")";
		} else if (type == V_VOLTAGE) {
			actionDesc = childStr + " = " + String(p) + " V (Child " + String(childId) + ")";
		} else if (type == V_VAR1) {
			if (isSending) {
				actionDesc = "Ajustar " + childStr + " = " + String(p) + " min (Child " + String(childId) + ")";
			} else {
				actionDesc = childStr + " = " + String(p) + " min (Child " + String(childId) + ")";
			}
		} else if (type == V_CUSTOM) {
			actionDesc = "Comando '" + String(p) + "' em " + childStr + " (Child " + String(childId) + ")";
		} else if (type == V_TEXT) {
			actionDesc = childStr + ": \"" + String(p) + "\" (Child " + String(childId) + ")";
		} else {
			actionDesc = childStr + " = " + String(p) + " (Child " + String(childId) + ")";
		}

		if (isSending) {
			return "Mensagem enviada para o " + nodeStr + ": " + actionDesc;
		} else {
			return "Mensagem recebida do " + nodeStr + ": " + actionDesc;
		}
	}

} // namespace M360

#endif // ESP8266
