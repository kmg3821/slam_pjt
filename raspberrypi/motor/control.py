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
from config import config
from time import sleep

env = config('../../config/config.txt')

mh = Raspi_MotorHAT(addr=0x6f) 
myMotor = mh.getMotor(int(env.get('MOTOR_PIN'))) #핀번호
servo = PWM(0x6F)
servo.setPWMFreq(60)  # Set frequency to 60 Hz
MIN = int(env.get('MOTOR_LEFT_LIMIT'))
MAX = int(env.get('MOTOR_RIGHT_LIMIT'))
CENTER = int(env.get('MOTOR_CENTER'))
SPEED = int(env.get('MOTOR_SPEED'))

POS = CENTER
lrspeed = 0
lrthread = None

def terminate(signum, frame):
    if lrthread:
        lrthread.cancel()
    exit(0)

def GO():
    myMotor.setSpeed(SPEED)
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
    myMotor.setSpeed(SPEED)
    myMotor.run(Raspi_MotorHAT.BACKWARD)

def STOP():
    myMotor.run(Raspi_MotorHAT.RELEASE)

def LRSTOP():
    global lrspeed
    lrspeed = 0

def MIDDLE():
    global lrspeed, POS, CENTER
    POS = CENTER
    lrspeed = 0
    servo.setPWM(0, 0, CENTER)

def on_message(client, userdata, message):
    print(f"[MQTT {message.topic}]: {str(message.payload.decode('utf-8'))}")
    if str(message.payload.decode('utf-8')) == '0':
        if message.topic == 'rc/top' or message.topic == 'rc/bottom':
            STOP()
        elif message.topic == 'rc/left' or message.topic == 'rc/right':
            LRSTOP()
    elif message.topic == 'rc/top':
        GO()
    elif message.topic == 'rc/bottom':
        BACK()
    elif message.topic == 'rc/left':
        LEFT()
    elif message.topic == 'rc/right':
        RIGHT()
    elif message.topic == 'rc/align':
        MIDDLE()

if __name__ == '__main__':
    broker_address = env.get('MQTT_BROKER_IP')
    port = int(env.get('MQTT_BROKER_PORT'))
    client = mqtt.Client()
    print(f"{broker_address}:{port}")
    client.connect(host = broker_address, port=port);
    client.subscribe("rc/+")
    client.on_message = on_message
    UPDATELR()
    signal.signal(signal.SIGINT, terminate)
    client.loop_forever()
