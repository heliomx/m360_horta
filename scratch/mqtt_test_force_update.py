import sys
import time
import json
import paho.mqtt.client as mqtt

MQTT_SERVER = "72.62.142.165"
MQTT_PORT = 1883
MQTT_USER = "jmm"
MQTT_PASSWORD = "jmmsqn"

messages_received = []

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")
    client.subscribe("m360/#")
    print("Subscribed to m360/#")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode('utf-8')
    except Exception:
        payload = str(msg.payload)
    print(f"Received Topic: {msg.topic} | Payload: {payload}")
    messages_received.append((msg.topic, payload))

client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

print(f"Connecting to {MQTT_SERVER}:{MQTT_PORT}...")
client.connect(MQTT_SERVER, MQTT_PORT, 60)

client.loop_start()
time.sleep(2)  # Wait to establish connection and subscription

# Publish FORCE_UPDATE to Node 99 on both prefixes
# Format: {prefix}/in/{nodeId}/{sensorId}/{command}/{ack}/{type}
# cmd=1 (SET), ack=0, type=48 (V_CUSTOM), payload="FORCE_UPDATE"
print("Publishing FORCE_UPDATE to Node 99...")
client.publish("m360/DF/0000/in/99/0/1/0/48", "FORCE_UPDATE")
client.publish("m360/DF/0001/in/99/0/1/0/48", "FORCE_UPDATE")

# Also publish the JSON version just in case the gateway expects JSON input
# Gateway fromJSON parses topic buildTopicIn(cfg) -> m360/UF/CAR/in
client.publish("m360/DF/0000/in", json.dumps({"nodeId": 99, "sensorId": 0, "command": 1, "type": 48, "payload": "FORCE_UPDATE"}))
client.publish("m360/DF/0001/in", json.dumps({"nodeId": 99, "sensorId": 0, "command": 1, "type": 48, "payload": "FORCE_UPDATE"}))

print("Listening for 30 seconds for any responses...")
time.sleep(30)

client.loop_stop()
client.disconnect()
print(f"Finished. Captured {len(messages_received)} messages.")
