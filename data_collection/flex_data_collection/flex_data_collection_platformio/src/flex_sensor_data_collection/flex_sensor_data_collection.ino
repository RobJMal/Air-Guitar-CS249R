/*
SparkFun Inventor's Kit for Arduino
Example sketch 09

FLEX SENSOR

  Use the "flex sensor" to change the position of a servo

  In the previous sketch, we learned how to command a servo to
  mode to different positions. In this sketch, we'll introduce
  a new sensor, and use it to control the servo.

  A flex sensor is a plastic strip with a conductive coating.
  When the strip is straight, the coating will be a certain
  resistance. When the strip is bent, the particles in the coating
  get further apart, increasing the resistance. You can use this
  sensor to sense finger movement in gloves, door hinges, stuffed
  animals, etc. See http://www.sparkfun.com/tutorials/270 for
  more information.

Hardware connections:

  Flex sensor:

    The flex sensor is the plastic strip with black stripes.
    It senses bending away from the striped side.

    The flex sensor has two pins, and since it's a resistor,
    the pins are interchangable.

    Connect one of the pins to ANALOG IN pin 0 on the Arduino.
    Connect the same pin, through a 10K Ohm resistor (brown
    black orange) to GND.
    Connect the other pin to 5V.

  Servo:

    The servo has a cable attached to it with three wires.
    Because the cable ends in a socket, you can use jumper wires
    to connect between the Arduino and the servo. Just plug the
    jumper wires directly into the socket.

    Connect the RED wire (power) to 5 Volts (5V)
    Connect the WHITE wire (signal) to digital pin 9
    Connect the BLACK wire (ground) to ground (GND)

    Note that servos can use a lot of power, which can cause your
    Arduino to reset or behave erratically. If you're using large
    servos or many of them, it's best to provide them with their
    own separate 5V supply. See this Arduino Forum thread for info:
    http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1239464763

This sketch was written by SparkFun Electronics,
with lots of help from the Arduino community.
This code is completely free for any use.
Visit http://learn.sparkfun.com/products/2 for SIK information.
Visit http://www.arduino.cc to learn about the Arduino.

Version 2.0 6/2012 MDG
*/

// Define the analog input pin to measure flex sensor position:
const int PIN_INDEX = 17;
const int PIN_MIDDLE = 16;
const int PIN_RING = 15;
const int PIN_PINKY = 14; 

const int NUM_CAPTURED_SAMPLES_PER_GESTURE = 200; // Sampling rate is 1000 Hz, but chord gesture lasts for 0.2 seconds
const int NUM_FEATURES_PER_SAMPLE = 4; // four fingers
const int TOTAL_DATA_POINTS = NUM_CAPTURED_SAMPLES_PER_GESTURE * NUM_FEATURES_PER_SAMPLE;

int data[TOTAL_DATA_POINTS];

void setup() 
{ 
  // Use the serial monitor window to help debug our sketch:
  Serial.begin(9600);
  Serial.println("Starting...");
  delay(1000);
} 


void loop() 
{ 

  Serial.println("Capturing gesture in 3...");
  delay(1000);
  Serial.println("2...");
  delay(1000);
  Serial.println("1...");
  delay(1000);

  Serial.println("Capturing gesture...");

  int index = 0;
  while (index < TOTAL_DATA_POINTS) {
    data[index++] = analogRead(PIN_INDEX);
    data[index++] = analogRead(PIN_MIDDLE);
    data[index++] = analogRead(PIN_RING);
    data[index++] = analogRead(PIN_PINKY);
    delay(1); /* actual sample rate is 9600 Hz, artficially make it 1000 Hz */
  }

  Serial.println("Printing data...");
  Serial.println("START DATA");
  index = 0;
  for (int base = 0; base < TOTAL_DATA_POINTS; base += NUM_FEATURES_PER_SAMPLE) {
    for (int i = 0; i < NUM_FEATURES_PER_SAMPLE; i++) {
      Serial.print(data[base+i]);
      if (i != NUM_FEATURES_PER_SAMPLE - 1) {
        Serial.print(','); 
      }
    }
    Serial.println();
  }
  Serial.println(",,,,");
  Serial.println("END DATA");

  Serial.println("New loop...");
} 
