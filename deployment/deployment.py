import serial
from playsound import playsound
import os

ARDUINO_COM = 'COM7'
ARDUINO_PORT = 9600
SOUND_DIR = "../audio_files/chord_recordings/"
LABEL = "g"


arduino = serial.Serial(ARDUINO_COM, ARDUINO_PORT, timeout=.1)

while True:
    # the last bit gets rid of the new-line chars
    data = arduino.readline()[:-2]
    if data:
        print(data.decode("utf-8"))
    if data and data == b"START AUDIO":
        chord = arduino.readline()[:-2].decode("utf-8")
        print("Now playing chord: ", chord)
        playsound(SOUND_DIR + chord + ".wav", False)

