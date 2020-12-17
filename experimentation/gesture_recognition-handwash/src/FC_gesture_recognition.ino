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
#include <Arduino_LSM9DS1.h>
#include <Adafruit_BNO055.h>

#include <TensorFlowLite.h>

#include "main_functions.h"

#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "motion_model.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* motion_model = nullptr;
tflite::MicroInterpreter* motion_interpreter = nullptr;
TfLiteTensor* motion_input_tensor = nullptr;
TfLiteTensor* motion_output_tensor = nullptr;
int input_length;

const float ACCELERATION_RMS_THRESHOLD = 1.5;  // RMS (root mean square) threshold of significant motion in G's
const int NUM_CAPTURED_SAMPLES_PER_GESTURE = 59;
const int NUM_FEATURES_PER_SAMPLE = 6;
const int TOTAL_SAMPLES = NUM_CAPTURED_SAMPLES_PER_GESTURE * NUM_FEATURES_PER_SAMPLE;
const int THRESHOLD_SAMPLE_INDEX =  ((NUM_CAPTURED_SAMPLES_PER_GESTURE / 3) * NUM_FEATURES_PER_SAMPLE); // one-third of data comes before threshold
int capturedSamples = 0;

// Create an area of memory to use for input, output, and intermediate arrays.
// Minimum arena size, at the time of writing. After allocating tensors
// you can retrieve this value by invoking interpreter.arena_used_bytes().
const int kModelArenaSize = 4*1024;
// Extra headroom for model + alignment + future interpreter changes.
const int kExtraArenaSize = 560 + 16 + 100;
const int kTensorArenaSize = kModelArenaSize + kExtraArenaSize;
uint8_t tensor_arena[kTensorArenaSize];

unsigned long time_before; //the Arduino function millis(), that we use to time out model, returns a unsigned long.
unsigned long time_after;
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() {
  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  if (!IMU.begin()) {
    error_reporter->Report("Failed to initialize IMU!");
    while (1);
  }
  error_reporter->Report(
                      "Accelerometer sample rate:%f Hz, Gyroscope sample rate:%f Hz", 
                      IMU.accelerationSampleRate(),
                      IMU.gyroscopeSampleRate());


  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  motion_model = tflite::GetModel(g_motion_model);
  if (motion_model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         motion_model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::ops::micro::AllOpsResolver resolver;


  static tflite::MicroInterpreter static_interpreter(
      motion_model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  motion_interpreter = &static_interpreter;


  TfLiteStatus allocate_status = motion_interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    error_reporter->Report("interpreter: AllocateTensors() failed");
    return;
  }

  motion_input_tensor = motion_interpreter->input(0);

  input_length = motion_input_tensor->bytes / sizeof(float);

  motion_output_tensor = motion_interpreter->output(0);
}

// The name of this function is important for Arduino compatibility.
void loop() {
float aX, aY, aZ, gX, gY, gZ;

  // wait for threshold trigger, but keep N samples before threshold occurs
  while (1) {
    // wait for both acceleration and gyroscope data to be available
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      // shift values over one position (TODO: replace memmove with for loop?)
      memmove(motion_input_tensor->data.f, motion_input_tensor->data.f + NUM_FEATURES_PER_SAMPLE, sizeof(float) * NUM_FEATURES_PER_SAMPLE * 39);

      // insert the new data at the threshold index
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 0] = (aX + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 1] = (aY + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 2] = (aZ + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 3] = (gX + 2000.0) / 4000.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 4] = (gY + 2000.0) / 4000.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 5] = (gZ + 2000.0) / 4000.0;

      // calculate the RMS of the acceleration
      float accelerationRMS =  sqrt(fabs(aX) + fabs(aY) + fabs(aZ));

      if (accelerationRMS > ACCELERATION_RMS_THRESHOLD) {
        // threshold reached, break the loop
        break;
      }
    }
  }

  // use the threshold index as the starting point for the remainder of the data
  capturedSamples = THRESHOLD_SAMPLE_INDEX + NUM_FEATURES_PER_SAMPLE;

  // collect the remaining samples
  while (capturedSamples < TOTAL_SAMPLES) {
    // wait for both acceleration and gyroscope data to be available
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      // insert the new data
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 0] = (aX + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 1] = (aY + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 2] = (aZ + 4.0) / 8.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 3] = (gX + 2000.0) / 4000.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 4] = (gY + 2000.0) / 4000.0;
      motion_input_tensor->data.f[THRESHOLD_SAMPLE_INDEX + 5] = (gZ + 2000.0) / 4000.0;

      capturedSamples += NUM_FEATURES_PER_SAMPLE;
    }
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
  // error_reporter->Report("Memory Consumption (bytes): %d", motion_interpreter->arena_used_bytes());
}
