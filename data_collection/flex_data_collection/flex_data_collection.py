import serial


#  Script to collect data from the IMU and store it in the Data Directory!

## Steps to collect data
### 1. Compile and run the flex_data_collection_platformio project on the Arduino board
### 3. Set the ARDUINO_COM, ARDUINO_PORT to the appropriate values to communicate with the Arduino board
### 4. Set the DATA_DIR to the location to store the csv files
### 5. Set the LABEL to the label of the data being collected
### 6. Make sure the Arduino board is running the data collection software
### 7. Run python ./flex_data_collection.py
### 8. Collect data! 
### 9. ???
### 10. Profit

ARDUINO_COM = 'COM7'
ARDUINO_PORT = 9600
DATA_DIR = "../../data/flex/"
LABEL = "a"


arduino = serial.Serial('COM7', 9600, timeout=.1)
print_to_file = False

with open(DATA_DIR+LABEL+".csv", "ab") as f:
    while True:
        # the last bit gets rid of the new-line chars
        data = arduino.readline()[:-2]
        if data:
            if (data == b"END DATA"):
                print_to_file = False
            print(data)
            if (print_to_file):
                f.write(data)
                f.write(b"\r\n")
            if (data == b"START DATA"):
                print_to_file = True
