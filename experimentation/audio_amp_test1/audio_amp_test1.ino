/* This is example code from Arduino's Audio library. It 
 * Is labelled as Part_1_04_Blink_While_Playing. It has 
 * been adapted to read a .WAV file from an SD Card.
 * 
 * Pin connection for Teensy 4.1 for Adafruit I2S 3W Amplifier (MAX 98357A)
 * Obtained from https://github.com/TeensyUser/doc/wiki/Audio-Example-I2S-without-AudioShield
    LRC:       Pin 20/A6
    BCLK:      Pin 21/A7
    Gain:      see below
    Shutdown:  N/C
    GND:       Ground
    VIN:       5V
 
 * Gain setting:

    15dB if a 100K resistor is connected between GAIN and GND
    12dB  if GAIN is connected directly to GND
     9dB  if GAIN is not connected to anything (this is the default)
     6dB  if GAIN is conneted directly to Vin
     3dB  if a 100K resistor is connected between GAIN and Vin.

 * Example Teensy .WAV files obtained from https://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
 * 
 * Audio Tutorial kit found at https://www.pjrc.com/store/audio_tutorial_kit.html
 *
*/

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


void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.45);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  pinMode(13, OUTPUT); // LED on pin 13
  delay(1000);
}

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing");
    playSdWav1.play("Am_chord.WAV");
    delay(10); // wait for library to parse WAV info
  }

  // print the play time offset
  Serial.print("Playing, now at ");
  Serial.print(playSdWav1.positionMillis());
  Serial.println(" ms");

  // blink LED and print info while playing
  digitalWrite(13, HIGH);
  delay(250);
  digitalWrite(13, LOW);
  delay(250);

  // read the knob position (analog input A2)
  /*
  int knob = analogRead(A2);
  float vol = (float)knob / 1280.0;
  sgtl5000_1.volume(vol);
  Serial.print("volume = ");
  Serial.println(vol);
  */
}
