import sys
import time
import subprocess

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Installing paho-mqtt...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "paho-mqtt"])
    import paho.mqtt.client as mqtt

# Config
MQTT_SERVER = "72.62.142.165"
MQTT_PORT = 1883
MQTT_USER = "jmm"
MQTT_PASSWORD = "jmmsqn"
TOPIC = "m360/#"

messages_received = []

def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")
    client.subscribe(TOPIC)
    print(f"Subscribed to {TOPIC}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode('utf-8')
    except Exception:
        payload = str(msg.payload)
    print(f"Topic: {msg.topic} | Payload: {payload}")
    messages_received.append((msg.topic, payload))

client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

print(f"Connecting to {MQTT_SERVER}:{MQTT_PORT}...")
client.connect(MQTT_SERVER, MQTT_PORT, 60)

# Run loop in background
client.loop_start()

# Let's listen for 15 seconds
print("Listening for 15 seconds...")
time.sleep(15)

client.loop_stop()
client.disconnect()
print(f"Done. Captured {len(messages_received)} messages.")
