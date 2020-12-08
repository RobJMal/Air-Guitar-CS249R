// Source https://learn.adafruit.com/adafruit-bno055-absolute-orientation-sensor/arduino-code

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
  
Adafruit_BNO055 bno = Adafruit_BNO055(55);

void setup(void) 
{
  Serial.begin(9600);
  Serial.println("Orientation Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
 
  delay(1000);
   
  bno.setExtCrystalUse(true);
}
 
void loop(void) 
{
  imu::Vector<3> linearaccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL );
  
  /* Display the floating point data */
  Serial.print("X: ");
  Serial.print(linearaccel.x());
  Serial.print(" Y: ");
  Serial.print(linearaccel.y());
  Serial.print(" Z: ");
  Serial.print(linearaccel.z());
  Serial.println("");
  
  delay(100);
}

//const float ACCELERATION_RMS_THRESHOLD = 2.0;  // RMS (root mean square) threshold of significant motion in G's
//const int NUM_CAPTURED_SAMPLES_PER_GESTURE = 119;
//const int NUM_FEATURES_PER_SAMPLE = 6;
//const int TOTAL_SAMPLES = NUM_CAPTURED_SAMPLES_PER_GESTURE * NUM_FEATURES_PER_SAMPLE;
//const int THRESHOLD_SAMPLE_INDEX =  ((NUM_CAPTURED_SAMPLES_PER_GESTURE / 3) * NUM_FEATURES_PER_SAMPLE); // one-third of data comes before threshold
//
//float samples[TOTAL_SAMPLES];
//
//int capturedSamples = 0;
//
//Adafruit_BNO055 IMU = Adafruit_BNO055(55);
//
//void setup(void) 
//{
//  Serial.begin(9600);
//  Serial.println("Orientation Sensor Test"); Serial.println("");
//  
//  /* Initialise the sensor */
//  if(!bno.begin())
//  {
//    /* There was a problem detecting the BNO055 ... check your connections */
//    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
//    while(1);
//  }
//  
//  delay(1000);
//    
//  IMU.setExtCrystalUse(true);
//
//  // print the header
//  Serial.println("aX,aY,aZ");
//}
//
//
//void loop() {
//  float aX, aY, aZ;
//
//  sensors_event_t event; 
//  bno.getEvent(&event);
//
//  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
//
//  // wait for threshold trigger, but keep N samples before threshold occurs
//  while (1) {
//    // reading the acceleration data 
//    aX = euler.x();
//    aY = euler.y();
//    aZ = euler.z();
//
//    Serial.print(aX);
//    Serial.print(',');
//    Serial.print(aY);
//    Serial.print(',');
//    Serial.println(aZ);
//
//    // shift values over one position (TODO: replace memmove with for loop?)
//    memmove(samples, samples + NUM_FEATURES_PER_SAMPLE, sizeof(float) * NUM_FEATURES_PER_SAMPLE * 39);
//
//    // insert the new data at the threshold index
//    samples[THRESHOLD_SAMPLE_INDEX + 0] = aX;
//    samples[THRESHOLD_SAMPLE_INDEX + 1] = aY;
//    samples[THRESHOLD_SAMPLE_INDEX + 2] = aZ;
//
//    // calculate the RMS of the acceleration
//    float accelerationRMS =  sqrt(fabs(aX) + fabs(aY) + fabs(aZ));
//
//    if (accelerationRMS > ACCELERATION_RMS_THRESHOLD) {
//      // threshold reached, break the loop
//      break;
//    }  
//
//    Serial.println("Loop 1");
//  }
//
//  // use the threshold index as the starting point for the remainder of the data
//  capturedSamples = THRESHOLD_SAMPLE_INDEX + NUM_FEATURES_PER_SAMPLE;
//
//  // collect the remaining samples
//  while (capturedSamples < TOTAL_SAMPLES) {    
//    // read the acceleration and gyroscope data
//    aX = euler.x();
//    aY = euler.y();
//    aZ = euler.z();
//
//    // insert the new data
//    samples[capturedSamples + 0] = aX;
//    samples[capturedSamples + 1] = aY;
//    samples[capturedSamples + 2] = aZ;
//
//    capturedSamples += NUM_FEATURES_PER_SAMPLE;
//    
//  }
//
//  // print the samples
//  for (int i = 0; i < TOTAL_SAMPLES; i += NUM_FEATURES_PER_SAMPLE) {
//    // print the data in CSV format
//    Serial.print(samples[i + 0], 3);
//    Serial.print(',');
//    Serial.print(samples[i + 1], 3);
//    Serial.print(',');
//    Serial.print(samples[i + 2], 3);
//
//    delayMicroseconds(8403); // delay between each line for Serial Plotter, this matches the 119 Hz data rate of IMU
//  }
//
//  Serial.println(); // empty line
//}

//void setup(void) 
//{
//  Serial.begin(9600);
//  Serial.println("Orientation Sensor Test"); Serial.println("");
//  
//  /* Initialise the sensor */
//  if(!bno.begin())
//  {
//    /* There was a problem detecting the BNO055 ... check your connections */
//    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
//    while(1);
//  }
//  
//  delay(1000);
//    
//  bno.setExtCrystalUse(true);
//}
// 
//void loop(void) 
//{
//  imu::Vector<3> linearaccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL );
//  
//  /* Display the floating point data */
//  Serial.print("X: ");
//  Serial.print(linearaccel.x());
//  Serial.print(" Y: ");
//  Serial.print(linearaccel.y());
//  Serial.print(" Z: ");
//  Serial.print(linearaccel.z());
//  Serial.println("");
//  
//  delay(100);
//}
