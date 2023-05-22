################################################################################
# This file requires https://github.com/orionrobots/Raspi_MotorHAT to execute. #
# please refer this github to install Raspi_MotorHAT.                          #
################################################################################

import os
import socket
import threading
import signal
from Raspi_MotorHAT import Raspi_MotorHAT, Raspi_DCMotor
from Raspi_PWM_Servo_Driver import PWM
import paho.mqtt.client as mqtt
import dotenv
from time import sleep


mh = Raspi_MotorHAT(addr=0x6f) 
myMotor = mh.getMotor(2) #핀번호
servo = PWM(0x6F)
servo.setPWMFreq(60)  # Set frequency to 60 Hz
MIN = 230
MAX = 370
POS = 300
lrspeed = 0
lrthread = None

def terminate(signum, frame):
    if lrthread:
        lrthread.cancel()
    exit(0)

def GO():
    myMotor.setSpeed(200)
    myMotor.run(Raspi_MotorHAT.FORWARD)
    
def LEFT():
    global lrspeed
    lrspeed = -10

def RIGHT():
    global lrspeed
    lrspeed = 10

def UPDATELR():
    global MIN, MAX, POS, lrspeed, lrthread
    POS += lrspeed
    if POS < MIN:
        POS = MIN
    elif POS > MAX:
        POS = MAX
    servo.setPWM(0, 0, POS)
    lrthread = threading.Timer(0.05, UPDATELR)
    lrthread.start()


def BACK():
    myMotor.setSpeed(200)
    myMotor.run(Raspi_MotorHAT.BACKWARD)

def STOP():
    global lrspeed
    myMotor.run(Raspi_MotorHAT.RELEASE)
    lrspeed = 0
    #servo.setPWM(0, 0, 345)

def MIDDLE():
    servo.setPWM(0, 0, 300)

def on_message(client, userdata, message):
    print(f"[MQTT {message.topic}]: {str(message.payload.decode('utf-8'))}")
    if str(message.payload.decode('utf-8')) == '0':
        STOP();
    elif message.topic == 'rc/top':
        GO();
    elif message.topic == 'rc/bottom':
        BACK();
    elif message.topic == 'rc/left':
        LEFT();
    elif message.topic == 'rc/right':
        RIGHT();

if __name__ == '__main__':
    dotenv.load_dotenv(dotenv.find_dotenv())
    modelServer = os.getenv('modelServer')
    broker_address = modelServer
    port = 1883
    client = mqtt.Client()
    print(f"{broker_address}:{port}")
    client.connect(host = broker_address, port=port);
    client.subscribe("rc/+")
    client.on_message = on_message
    UPDATELR()
    signal.signal(signal.SIGINT, terminate)
    client.loop_forever()
