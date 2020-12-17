# Script to play the glorious sounds from your Air Guitar!

## Steps to hear the fruits of your labor
### 1. Follow wiring instructions to connect IMU and flex sensors
### 2. Compile and run the deployment Platform.IO project on the Teensy board
### 3. Set the ARDUINO_COM, ARDUINO_PORT to the appropriate values to communicate with the Teensy board
### 4. Set the SOUND_DIR to the location of the chord sound files
### 5. Run python ./deployment.py
### 6. Look at the output from the Teensy on the terminal
### 7. The script will play a chord's corresponding sound file when the board instructs it to do
### 8. Enjoy the music!
### 9. ???
### 10. Profit

import serial
from playsound import playsound
import os

ARDUINO_COM = 'COM7'
ARDUINO_PORT = 9600
SOUND_DIR = "../audio_files/chord_recordings/"


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

