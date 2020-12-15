import serial

ARDUINO_COM = 'COM7'
ARDUINO_PORT = 9600
LABEL = "strum"
DATA_DIR = "../../data/imu/"

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
