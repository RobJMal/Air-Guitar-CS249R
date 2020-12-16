// Source https://learn.adafruit.com/adafruit-bno055-absolute-orientation-sensor/arduino-code

/*
  Script to collect data from Adafruit BNO055 IMU

  Wiring:
  3.3V (Red)  --> 3.3V rail/power supply
  GND (Black) --> Board GND
  SDA (Gray)  --> 18 (Teensy 4.1)
  SCL (White) --> 19 (Teensy 4.1)

*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
  
Adafruit_BNO055 IMU = Adafruit_BNO055(55);

const int PORT = 9600;
const float ACCELERATION_RMS_THRESHOLD = 3.0;  // RMS (root mean square) threshold of significant motion in G's
const int SENSOR_SAMPLING_RATE = 100;
const float GESTURE_DURATION = 0.5;
const int NUM_FEATURES_PER_SAMPLE = 3;

const int NUM_CAPTURED_SAMPLES_PER_GESTURE = GESTURE_DURATION * SENSOR_SAMPLING_RATE; // Sampling rate is 100 Hz, but strum lasts for 0.5 seconds
const int SENSOR_SAMPLING_SEPARATION = 1000 / SENSOR_SAMPLING_RATE ;
const int TOTAL_SAMPLES = NUM_CAPTURED_SAMPLES_PER_GESTURE * NUM_FEATURES_PER_SAMPLE;
const int THRESHOLD_SAMPLE_INDEX =  ((NUM_CAPTURED_SAMPLES_PER_GESTURE / 3) * NUM_FEATURES_PER_SAMPLE); // one-third of data comes before threshold

float samples[TOTAL_SAMPLES];

int capturedSamples = 0;

void setup(void) {
  Serial.begin(PORT);
  Serial.println("Orientation Sensor Test"); 
  Serial.println("");
  
  /* Initialise the sensor */
  if(!IMU.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
 
  delay(1000);
}
 
void loop(void) {
  float aX, aY, aZ;
  unsigned long time_before, time_after, duration;

  // wait for threshold trigger, but keep N samples before threshold occurs
  while (1) {

    // Accesses the IMU
    time_before = millis();
    sensors_event_t event;
    IMU.getEvent(&event);
    imu::Vector<3> linearaccel = IMU.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL );
    // reading accleration data 
    aX = linearaccel.x();
    aY = linearaccel.y();
    aZ = linearaccel.z();
    time_after = millis();

    duration = time_after - time_before;
    if (duration < SENSOR_SAMPLING_SEPARATION) {
      delay(SENSOR_SAMPLING_SEPARATION - duration);
    }
    
    // shift values over one position (TODO: replace memmove with for loop?)
    memmove(samples, samples + NUM_FEATURES_PER_SAMPLE, sizeof(float) * NUM_FEATURES_PER_SAMPLE * 39);

    // insert the new data at the threshold index
    samples[THRESHOLD_SAMPLE_INDEX + 0] = aX;
    samples[THRESHOLD_SAMPLE_INDEX + 1] = aY;
    samples[THRESHOLD_SAMPLE_INDEX + 2] = aZ;

    // calculate the RMS of the acceleration
    float accelerationRMS =  sqrt(fabs(aX) + fabs(aY) + fabs(aZ));

     if (accelerationRMS > ACCELERATION_RMS_THRESHOLD) {
      // threshold reached, break the loop
      break;
     }  
  }

  // use the threshold index as the starting point for the remainder of the data
  capturedSamples = THRESHOLD_SAMPLE_INDEX + NUM_FEATURES_PER_SAMPLE;

  // collect the remaining samples
  while (capturedSamples < TOTAL_SAMPLES) {    
    time_before = millis();
    // Accesses the IMU
    sensors_event_t event;
    IMU.getEvent(&event);
    imu::Vector<3> linearaccel = IMU.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL );
    // reading accleration data 
    aX = linearaccel.x();
    aY = linearaccel.y();
    aZ = linearaccel.z();
    time_after = millis();

    duration = time_after - time_before;
    
    if (duration < SENSOR_SAMPLING_SEPARATION) {
      delay(SENSOR_SAMPLING_SEPARATION - duration);
    }

    // insert the new data
    samples[capturedSamples + 0] = aX;
    samples[capturedSamples + 1] = aY;
    samples[capturedSamples + 2] = aZ;

    capturedSamples += NUM_FEATURES_PER_SAMPLE;
 }

  // Printing out the samples
  Serial.println("START DATA");

  for (int i = 0; i < TOTAL_SAMPLES; i += NUM_FEATURES_PER_SAMPLE) {
    // print the data in CSV format
    Serial.print(samples[i + 0], 3);
    Serial.print(',');
    Serial.print(samples[i + 1], 3);
    Serial.print(',');
    Serial.print(samples[i + 2], 3);
    Serial.println(); // empty line

  }

  Serial.println(",,,"); // empty row
  Serial.println("END DATA");
  delay(1000); // How long to wait before restarting the data collection process 
}
