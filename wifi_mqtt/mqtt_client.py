import paho.mqtt.client as paho

import time
import serial

serdev = '/dev/ttyACM0'                # use the device name you get from `ls /dev/ttyACM*`

s = serial.Serial(serdev, 9600)
# https://os.mbed.com/teams/mqtt/wiki/Using-MQTT#python-client


# MQTT broker hosted on local machine

mqttc = paho.Client()


# Settings for connection

# TODO: revise host to your IP

host = "192.168.31.176"

topic = "Mbed"


# Callbacks

def on_connect(self, mosq, obj, rc):

    print("Connected rc: " + str(rc))


def on_message(mosq, obj, msg):

    print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n");
    # threshold angle
    if chr(msg.payload[0]) == 't' :
        print("stop guest UI mode");
        s.write("/mode/run 1 0\r".encode())
        
    # detect mode : overtilt
    if chr(msg.payload[0]) == 'o' :
        if chr(msg.payload[13]) >= '5' :
            print("stop tilt detection mode");
            s.write("/mode/run 2 0\r".encode())
            




def on_subscribe(mosq, obj, mid, granted_qos):

    print("Subscribed OK")


def on_unsubscribe(mosq, obj, mid, granted_qos):

    print("Unsubscribed OK")


# Set callbacks

mqttc.on_message = on_message

mqttc.on_connect = on_connect

mqttc.on_subscribe = on_subscribe

mqttc.on_unsubscribe = on_unsubscribe


# Connect and subscribe

print("Connecting to " + host + "/" + topic)

mqttc.connect(host, port=1883, keepalive=60)

mqttc.subscribe(topic, 0)


# Publish messages from Python

num = 0

while num != 1:

    ret = mqttc.publish(topic, "Message from Python!\n", qos=0)

    if (ret[0] != 0):

            print("Publish failed")

    mqttc.loop()

    time.sleep(1.5)

    num += 1


# Loop forever, receiving messages

mqttc.loop_forever()