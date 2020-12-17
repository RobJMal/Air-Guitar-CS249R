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
#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


#include <TensorFlowLite.h>

#include "main_functions.h"

#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "motion_model.h"
#include "chord_model.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Interfacing with SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD

// Globals, used for compatibility with Arduino-style sketches.
namespace {
Adafruit_BNO055 IMU = Adafruit_BNO055(55);
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* motion_model = nullptr;
tflite::MicroInterpreter* motion_interpreter = nullptr;
TfLiteTensor* motion_input_tensor = nullptr;
TfLiteTensor* motion_output_tensor = nullptr;
int input_length_motion;

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playSdWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playSdWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

const float ACCELERATION_RMS_THRESHOLD = 5.0;  // RMS (root mean square) threshold of significant motion in G's
const int SENSOR_SAMPLING_RATE = 100;
const float GESTURE_DURATION = 0.5;
const int NUM_CAPTURED_SAMPLES_PER_GESTURE = GESTURE_DURATION * SENSOR_SAMPLING_RATE;
const int NUM_FEATURES_PER_SAMPLE = 3;
const int TOTAL_SAMPLES = NUM_CAPTURED_SAMPLES_PER_GESTURE * NUM_FEATURES_PER_SAMPLE;
const int SENSOR_SAMPLING_SEPARATION = 1000 / SENSOR_SAMPLING_RATE ;
const int THRESHOLD_SAMPLE_INDEX =  ((NUM_CAPTURED_SAMPLES_PER_GESTURE / 3) * NUM_FEATURES_PER_SAMPLE); // one-third of data comes before threshold
int capturedSamplesIMU = 0;

// Create an area of memory to use for input, output, and intermediate arrays.
// Minimum arena size, at the time of writing. After allocating tensors
// you can retrieve this value by invoking interpreter.arena_used_bytes().
const int kModelArenaSize_motion = 16*1024;
// Extra headroom for model + alignment + future interpreter changes.
const int kExtraArenaSize_motion = 560 + 16 + 100;
const int kTensorArenaSize_motion = kModelArenaSize_motion + kExtraArenaSize_motion;
uint8_t tensor_arena_motion[kTensorArenaSize_motion];

const float IMU_DATA_MIN_VALUE = -48.89;
const float IMU_DATA_MAX_VALUE = 41.48;
const float IMU_DATA_DELTA = IMU_DATA_MAX_VALUE - IMU_DATA_MIN_VALUE;

const int FLEX_DATA_MIN_VALUE = 308;
const int FLEX_DATA_MAX_VALUE = 904;
const int FLEX_DATA_DELTA = FLEX_DATA_MAX_VALUE - FLEX_DATA_MIN_VALUE;

const int PIN_INDEX = 17;
const int PIN_MIDDLE = 16;
const int PIN_RING = 15;
const int PIN_PINKY = 14; 

const tflite::Model* chord_model = nullptr;
tflite::MicroInterpreter* chord_interpreter = nullptr;
TfLiteTensor* chord_input_tensor = nullptr;
TfLiteTensor* chord_output_tensor = nullptr;
int input_length_chord; 

const int NUM_CAPTURED_SAMPLES_PER_GESTURE_FLEX = 200; // Sampling rate is 1000 Hz, but chord gesture lasts for 0.2 seconds
const int NUM_FEATURES_PER_SAMPLE_FLEX = 4; // four fingers
const int TOTAL_DATA_POINTS_FLEX = NUM_CAPTURED_SAMPLES_PER_GESTURE_FLEX * NUM_FEATURES_PER_SAMPLE_FLEX;

// Memory area 
const int kModelArenaSize_chord = 30*1024;
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
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  Serial.begin(9600);


  if (!IMU.begin()) {
    error_reporter->Report("Failed to initialize IMU!");
    while (1) {
      error_reporter->Report("Failed to intialize IMU :(");
      delay(500);
    };
  }

  motion_model = tflite::GetModel(g_motion_model);
  if (motion_model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Motion model provided is schema version %d not equal to supported version %d.",
                         motion_model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  chord_model = tflite::GetModel(c_motion_model);
  if (chord_model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Chord model provided is a schema version %d not equal to supported version %d.",
                          chord_model->version(), TFLITE_SCHEMA_VERSION);
  }

  static tflite::ops::micro::AllOpsResolver resolver_motion;
  static tflite::ops::micro::AllOpsResolver resolver_chord; 


  static tflite::MicroInterpreter static_interpreter_motion(
      motion_model, resolver_motion, tensor_arena_motion, kTensorArenaSize_motion, error_reporter);
  motion_interpreter = &static_interpreter_motion;

  static tflite::MicroInterpreter static_interpreter_chord(
      chord_model, resolver_chord, tensor_arena_chord, kTensorArenaSize_chord, error_reporter);
  chord_interpreter = &static_interpreter_chord; 


  TfLiteStatus allocate_status_motion = motion_interpreter->AllocateTensors();
  if (allocate_status_motion != kTfLiteOk) {
    error_reporter->Report("interpreter: AllocateTensors() failed (strum)");
    return;
  }

  TfLiteStatus allocate_status_chord = chord_interpreter-> AllocateTensors();
  if (allocate_status_chord != kTfLiteOk) {
    error_reporter->Report("interpreter: AllocateTensors() failed (chord)");
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
    // while (1) {
    //   error_reporter_chord->Report("Unable to access the SD card");
    //   delay(500);
    // }
    error_reporter->Report("Unable to access SD Card :(");
  }
  pinMode(13, OUTPUT); // LED on pin 13
  delay(1000);
}

// The name of this function is important for Arduino compatibility.
void loop() {
  float aX, aY, aZ;

  error_reporter->Report("In loop");
  delay(500);

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
    if (data_duration < SENSOR_SAMPLING_SEPARATION) {
      delay(SENSOR_SAMPLING_SEPARATION - data_duration);
    }

    // shift values over one position (TODO: replace memmove with for loop?)
    memmove(motion_input_tensor->data.f, motion_input_tensor->data.f + NUM_FEATURES_PER_SAMPLE, sizeof(float) * NUM_FEATURES_PER_SAMPLE * 39);

    // insert the new data at the threshold index
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 0] = (aX - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 1] = (aY - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;
    motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 2] = (aZ - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;

    // calculate the RMS of the acceleration
    float accelerationRMS =  sqrt(fabs(aX) + fabs(aY) + fabs(aZ));

    if (accelerationRMS > ACCELERATION_RMS_THRESHOLD) {
      // threshold reached, break the loop
      break;
    }
  }

  // use the threshold index as the starting point for the remainder of the data
  capturedSamplesIMU = THRESHOLD_SAMPLE_INDEX + NUM_FEATURES_PER_SAMPLE;

  // collect the remaining samples
  while (capturedSamplesIMU < TOTAL_SAMPLES) {
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
    if (data_duration < SENSOR_SAMPLING_SEPARATION) {
      delay(SENSOR_SAMPLING_SEPARATION - data_duration);
    }

    // insert the new data at the threshold index
    motion_input_tensor->data.f[capturedSamplesIMU + 0] = (aX - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;
    motion_input_tensor->data.f[capturedSamplesIMU + 1] = (aY - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;
    motion_input_tensor->data.f[capturedSamplesIMU + 2] = (aZ - IMU_DATA_MIN_VALUE) / IMU_DATA_DELTA;

    capturedSamplesIMU += NUM_FEATURES_PER_SAMPLE;
    
  }

  // Run inferencing
  time_before = millis();
  TfLiteStatus invoke_status = motion_interpreter->Invoke();
  time_after = millis();

  if (invoke_status != kTfLiteOk) {
    error_reporter->Report("Invoke failed!");
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

  error_reporter->Report("Time Stamp: %d", time_after);
  error_reporter->Report("Gesture: %s", GESTURES[max_index]);
  error_reporter->Report("Invoke time (mS): %d", time_after-time_before);

  if (GESTURES[max_index][0] == 's') {
    // Collecting data from flex sensors 
    int index = 0;
    while (index < TOTAL_DATA_POINTS_FLEX) {
      chord_input_tensor->data.f[index++] = (analogRead(PIN_INDEX) - FLEX_DATA_MIN_VALUE) / FLEX_DATA_DELTA;
      chord_input_tensor->data.f[index++] = (analogRead(PIN_MIDDLE) - FLEX_DATA_MIN_VALUE) / FLEX_DATA_DELTA;
      chord_input_tensor->data.f[index++] = (analogRead(PIN_RING) - FLEX_DATA_MIN_VALUE) / FLEX_DATA_DELTA;
      chord_input_tensor->data.f[index++] = (analogRead(PIN_PINKY) - FLEX_DATA_MIN_VALUE) / FLEX_DATA_DELTA;
      delay(1); /* actual sample rate is 9600 Hz, artficially make it 1000 Hz */
    }

    // Run chord model inferencing 
    time_before = millis();
    TfLiteStatus invoke_status_chord = chord_interpreter->Invoke();
    time_after = millis();

    if (invoke_status_chord != kTfLiteOk) {
      error_reporter->Report("Invoke failed!");
      while (1);
      return;
    }

    // Loop through the output tensor values from the model
    max_val = 0.0;
    max_index = 0;
    for (int i = 0; i < num_chords; i++) {
      if (chord_output_tensor->data.f[i] > max_val){
        max_val = chord_output_tensor->data.f[i];
        max_index = i;
      }
    }

    const char* chord_to_play = CHORDS[max_index];
    error_reporter->Report("Chord: %s", chord_to_play);
    error_reporter->Report("Invoke time: %d", time_after - time_before);

    // // Playing the air guitar if strum is detected based on previous chord classified
    // char chord_filename[8];

    // if (playSdWav1.isPlaying() == false) {
    //   error_reporter->Report("Start playing");

    //   strcat(chord_filename, chord_to_play);
    //   strcat(chord_filename, ".WAV");
    //   playSdWav1.play(chord_filename);

    //   delay(10);
    // }

    Serial.println("START AUDIO");
    Serial.println(chord_to_play);

    // blink LED and print info while playing
    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
    delay(250);
  }
  // error_reporter->Report("Memory Consumption (bytes): %d", motion_interpreter->arena_used_bytes());
}
