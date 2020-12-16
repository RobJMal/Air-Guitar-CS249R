/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>
#include <Wire.h>

#include <TensorFlowLite.h>

#include "main_functions.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"

#include "motion_model.h"
#include "chord_model.h"

#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Libraries for playing back from SD card 
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playSdWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playSdWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Interfacing with SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD

// Globals, used for compatibility with Arduino-style sketches.
namespace {
Adafruit_BNO055 IMU = Adafruit_BNO055(55);

// STRUM DETECTION GLOBALS
tflite::ErrorReporter* error_reporter_motion = nullptr;
const tflite::Model* motion_model = nullptr;
tflite::MicroInterpreter* motion_interpreter = nullptr;
TfLiteTensor* motion_input_tensor = nullptr;
TfLiteTensor* motion_output_tensor = nullptr;
int input_length_motion;

const float ACCELERATION_RMS_THRESHOLD = 5.0;  // RMS (root mean square) threshold of significant motion in G's
const int SENSOR_SAMPLING_RATE_IMU = 100;
const float GESTURE_DURATION_IMU = 0.5;
const int NUM_CAPTURED_SAMPLES_PER_GESTURE_IMU = GESTURE_DURATION_IMU * SENSOR_SAMPLING_RATE_IMU;
const int NUM_FEATURES_PER_SAMPLE_IMU = 3;
const int TOTAL_SAMPLES_IMU = NUM_CAPTURED_SAMPLES_PER_GESTURE_IMU * NUM_FEATURES_PER_SAMPLE_IMU;
const int SENSOR_SAMPLING_SEPARATION_IMU = 1000 / SENSOR_SAMPLING_RATE_IMU ;
const int THRESHOLD_SAMPLE_INDEX_IMU =  ((NUM_CAPTURED_SAMPLES_PER_GESTURE_IMU / 3) * NUM_FEATURES_PER_SAMPLE_IMU); // one-third of data comes before threshold
int capturedSamplesIMU = 0;

// Create an area of memory to use for input, output, and intermediate arrays.
// Minimum arena size, at the time of writing. After allocating tensors
// you can retrieve this value by invoking interpreter.arena_used_bytes().
const int kModelArenaSize_motion = 16*1024;
// Extra headroom for model + alignment + future interpreter changes.
const int kExtraArenaSize_motion = 560 + 16 + 100;
const int kTensorArenaSize_motion = kModelArenaSize_motion + kExtraArenaSize_motion;
uint8_t tensor_arena_motion[kTensorArenaSize_motion];


// CHORD DETECTION GLOBALS 
// Define the analog input pin to measure flex sensor position:
const int PIN_INDEX = 17;
const int PIN_MIDDLE = 16;
const int PIN_RING = 15;
const int PIN_PINKY = 14; 

tflite::ErrorReporter* error_reporter_chord = nullptr;
const tflite::Model* chord_model = nullptr;
tflite::MicroInterpreter* chord_interpreter = nullptr;
TfLiteTensor* chord_input_tensor = nullptr;
TfLiteTensor* chord_output_tensor = nullptr;
int input_length_chord; 

const int NUM_CAPTURED_SAMPLES_PER_GESTURE_FLEX = 200; // Sampling rate is 1000 Hz, but chord gesture lasts for 0.2 seconds
const int NUM_FEATURES_PER_SAMPLE_FLEX = 4; // four fingers
const int TOTAL_DATA_POINTS_FLEX = NUM_CAPTURED_SAMPLES_PER_GESTURE_FLEX * NUM_FEATURES_PER_SAMPLE_FLEX;
int data_flex[TOTAL_DATA_POINTS_FLEX];

// Memory area 
const int kModelArenaSize_chord = 16*1024;
const int kExtraArenaSize_chord = 560 + 16 + 100;
const int kTensorArenaSize_chord = kModelArenaSize_chord + kExtraArenaSize_chord;
uint8_t tensor_arena_chord[kTensorArenaSize_chord];


unsigned long time_before; //the Arduino function millis(), that we use to time out model, returns a unsigned long.
unsigned long time_after;
unsigned long data_time_before;
unsigned long data_time_after;
unsigned long data_duration;
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter_motion;
  error_reporter_motion = &micro_error_reporter_motion;

  static tflite::MicroErrorReporter micro_error_reporter_chord;
  error_reporter_chord = &micro_error_reporter_chord;

  // Flex sensors automatically initialize, check output values on serial monitor however
  if (!IMU.begin()) {
    TF_LITE_REPORT_ERROR(error_reporter_motion,"Failed to initialize IMU!");
    while (1);
  }

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  motion_model = tflite::GetModel(g_motion_model);
  if (motion_model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter_motion,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         motion_model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  chord_model = tflite::GetModel(c_motion_model);
  if (chord_model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter_chord,
                          "Model provided is a schema version %d not equal"
                          "to supported version %d.",
                          chord_model->version(), TFLITE_SCHEMA_VERSION);
  }


  static tflite::AllOpsResolver resolver_motion;
  static tflite::AllOpsResolver resolver_chord; 

  static tflite::MicroInterpreter static_interpreter_motion(
      motion_model, resolver_motion, tensor_arena_motion, kTensorArenaSize_motion, error_reporter_motion);
  motion_interpreter = &static_interpreter_motion;

  static tflite::MicroInterpreter static_interpreter_chord(
      chord_model, resolver_chord, tensor_arena_chord, kTensorArenaSize_chord, error_reporter_chord);
  chord_interpreter = &static_interpreter_chord; 


  TfLiteStatus allocate_status_motion = motion_interpreter->AllocateTensors();
  if (allocate_status_motion != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_motion, "interpreter: AllocateTensors() failed (strum)");
    return;
  }

  TfLiteStatus allocate_status_chord = chord_interpreter-> AllocateTensors();
  if (allocate_status_chord != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_chord, "interpreter: AllocateTensors() failed (chord)");
  }

  // I/O tensors for strum detect 
  motion_input_tensor = motion_interpreter->input(0);
  input_length_motion = motion_input_tensor->bytes / sizeof(float);
  motion_output_tensor = motion_interpreter->output(0);

  // I/O tensors for chord detect
  chord_input_tensor = chord_interpreter->input(0);
  input_length_chord = chord_input_tensor->bytes / sizeof(float);
  chord_output_tensor = chord_interpreter->output(0); 

  // Setting up SD card 
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.45);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      TF_LITE_REPORT_ERROR(error_reporter_chord, "Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  delay(1000);
}

// The name of this function is important for Arduino compatibility.
void loop() {

  /*========================================================================*/
  const char* chord_to_play; 

  // Collecting data from flex sensors 
  int index = 0;
  while (index < TOTAL_DATA_POINTS_FLEX) {
    data_flex[index++] = analogRead(PIN_INDEX);
    data_flex[index++] = analogRead(PIN_MIDDLE);
    data_flex[index++] = analogRead(PIN_RING);
    data_flex[index++] = analogRead(PIN_PINKY);
    delay(1); /* actual sample rate is 9600 Hz, artficially make it 1000 Hz */
  }

  // Run chord model inferencing 
  time_before = millis();
  TfLiteStatus invoke_status = chord_interpreter->Invoke();
  time_after = millis();

  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_chord,"Invoke failed!");
    while (1);
    return;
  }

  // Loop through the output tensor values from the model
  float max_val = 0.0;
  int max_index = 0;
  for (int i = 0; i < num_chords; i++) {
    if (chord_output_tensor->data.f[i] > max_val){
      max_val = chord_output_tensor->data.f[i];
      max_index = i;
    }
  }

  chord_to_play = CHORDS[max_index];
  TF_LITE_REPORT_ERROR(error_reporter_chord, "Chord: %s", CHORDS[max_index]);

  /*=========================================================================*/
  float aX, aY, aZ;

  // wait for threshold trigger, but keep N samples before threshold occurs
  while (1) {
    // Accesses the IMU
    data_time_before = millis();
    sensors_event_t event;
    IMU.getEvent(&event);
    imu::Vector<3> linearaccel = IMU.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    
    // reading accleration data 
    aX = linearaccel.x();
    aY = linearaccel.y();
    aZ = linearaccel.z();
    data_time_after = millis();

    data_duration = data_time_after - data_time_before;
    if (data_duration < SENSOR_SAMPLING_SEPARATION_IMU) {
      delay(SENSOR_SAMPLING_SEPARATION_IMU - data_duration);
    }

    // shift values over one position (TODO: replace memmove with for loop?)
    memmove(motion_input_tensor->data.f, motion_input_tensor->data.f + NUM_FEATURES_PER_SAMPLE_IMU, sizeof(float) * NUM_FEATURES_PER_SAMPLE_IMU * 39);

    // insert the new data at the threshold index
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 0] = aX;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 1] = aY;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 2] = aZ;

    // calculate the RMS of the acceleration
    float accelerationRMS =  sqrt(fabs(aX) + fabs(aY) + fabs(aZ));

    if (accelerationRMS > ACCELERATION_RMS_THRESHOLD) {
      // threshold reached, break the loop
      break;
    }    
  }

  // use the threshold index as the starting point for the remainder of the data
  capturedSamplesIMU = THRESHOLD_SAMPLE_INDEX_IMU + NUM_FEATURES_PER_SAMPLE_IMU;

  // collect the remaining samples
  while (capturedSamplesIMU < TOTAL_SAMPLES_IMU) {
    // Accesses the IMU
    data_time_before = millis();
    sensors_event_t event;
    IMU.getEvent(&event);
    imu::Vector<3> linearaccel = IMU.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    // reading accleration data 
    aX = linearaccel.x();
    aY = linearaccel.y();
    aZ = linearaccel.z();
    data_time_after = millis();

    data_duration = data_time_after - data_time_before;
    if (data_duration < SENSOR_SAMPLING_SEPARATION_IMU) {
      delay(SENSOR_SAMPLING_SEPARATION_IMU - data_duration);
    }

    // insert the new data at the threshold index
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 0] = aX;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 1] = aY;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX_IMU + 2] = aZ;

    capturedSamplesIMU += NUM_FEATURES_PER_SAMPLE_IMU;    
  }


  // Run inferencing
  time_before = millis();
  TfLiteStatus invoke_status = motion_interpreter->Invoke();
  time_after = millis();

  if (invoke_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_motion,"Invoke failed!");
    while (1);
    return;
  }

  // Loop through the output tensor values from the model
  float max_val = 0.0;
  int max_index = 0;
  for (int i = 0; i < num_gestures; i++) {
    if (motion_output_tensor->data.f[i] > max_val){
      max_val = motion_output_tensor->data.f[i];
      max_index = i;
    }
  }

  TF_LITE_REPORT_ERROR(error_reporter_motion, "Time Stamp: %d", time_after);
  TF_LITE_REPORT_ERROR(error_reporter_motion, "Gesture: %s", GESTURES[max_index]);
  TF_LITE_REPORT_ERROR(error_reporter_motion, "Invoke time (mS): %d", time_after-time_before);
  TF_LITE_REPORT_ERROR(error_reporter_motion, "Memory Consumption (bytes): %d", motion_interpreter->arena_used_bytes());


  /*=======================================================================================*/
  // Playing the air guitar if strum is detected based on previous chord classified
  char chord_filename[8];

  if (GESTURES[max_index] == "strum") {
    if (playSdWav1.isPlaying() == false) {
      TF_LITE_REPORT_ERROR(error_reporter_chord, "Start playing");

      strcat(chord_filename, chord_to_play);
      strcat(chord_filename, ".WAV");
      playSdWav1.play(chord_filename);
      
      delay(10);
    }

    // print the play time offset
    TF_LITE_REPORT_ERROR(error_reporter_chord, "Playing, now at ");
    TF_LITE_REPORT_ERROR(error_reporter_chord, playSdWav1.positionMillis()+" ms");

    // blink LED and print info while playing
    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
    delay(250);
  }
}
