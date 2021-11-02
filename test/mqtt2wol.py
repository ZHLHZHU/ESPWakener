import paho.mqtt.client as mqtt
import time
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.publish('SwingFrogWakener', payload="wake#D4-5D-64-D1-4A-CC", qos=1, retain=True)
        client.publish('SwingFrogWakener', payload="wake#D4-5D-64-D1-4A-CC", qos=1, retain=True)


client = mqtt.Client()
client.on_connect = on_connect
client.connect("broker.emqx.io", 1883, 60)
    

client.loop_forever()