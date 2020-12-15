/*
  Demo of the audio sweep function.
  The user specifies the amplitude,
  start and end frequencies (which can sweep up or down)
  and the length of time of the sweep.

  Modified to eliminate the audio shield, and use Max98357A mono I2S chip.
  https://smile.amazon.com/gp/product/B07PS653CD/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1

  Pins:    Teensy 4.x  Teensy 3.x

  LRCLK:  Pin 20/A6 Pin 23/A9
  BCLK:   Pin 21/A7 Pin 9
  DIN:    Pin 7   Pin 22/A8
  Gain:   see below see below
  Shutdown: N/C   N/C
  Ground: Ground    Ground
  VIN:    5v    5v

  Other I2S2 pins not used by the Max98357A device:

  MCLK    Pin 23    Pin 11
  DOUT    Pin 8   Pin 13

  Gain setting:

  15dB  if a 100K resistor is connected between GAIN and GND
  12dB  if GAIN is connected directly to GND
   9dB  if GAIN is not connected to anything (this is the default)
   6dB  if GAIN is conneted directly to Vin
   3dB  if a 100K resistor is connected between GAIN and Vin.

  SD setting (documentation from the Adafruit board)

  This pin is used for shutdown mode but is also used for setting which channel
  is output. It's a little confusing but essentially:

  * If SD is connected to ground directly (voltage is under 0.16V) then the amp
    is shut down

  * If the voltage on SD is between 0.16V and 0.77V then the output is (Left +
    Right)/2, that is the stereo average.

  * If the voltage on SD is between 0.77V and 1.4V then the output is just the
    Right channel

  * If the voltage on SD is higher than 1.4V then the output is the Left
    channel.

    This is compounded by an internal 100K pulldown resistor on SD so you need
    to use a pullup resistor on SD to balance out the 100K internal pulldown.

  Or alternatively, use the HiLetgo PCM5102 I2S IIS Lossless Digital Audio DAC
  Decoder which provides stereo output:
  https://smile.amazon.com/gp/product/B07Q9K5MT8/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1  */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code (edited by meissner afterwards).
AudioSynthToneSweep tonesweep;    //xy=99,198
AudioMixer4   mixer2;     //xy=280,253
AudioMixer4   mixer1;     //xy=280,175
AudioOutputI2S    i2s;      //xy=452,189

AudioConnection   patchCord1(tonesweep, 0, mixer1, 0);
AudioConnection   patchCord2(tonesweep, 0, mixer2, 0);
AudioConnection   patchCord3(mixer2,    0, i2s,    1);
AudioConnection   patchCord4(mixer1,    0, i2s,    0);
// GUItool: end automatically generated code

const float t_ampx  = 0.8;
const int t_lox = 10;
const int t_hix = 22000;
const float t_timex = 10;   // Length of time for the sweep in seconds

// Do a sweep in both directions, enabling or disabling the left/right speakers
void do_sweep (int i)
{
  int do_left  = (i & 1) != 0;
  int do_right = (i & 2) != 0;
  float gain   = (do_left && do_right) ? 0.5f : 1.0f;

  Serial.printf ("Sweep up,   left = %c, right = %c\n",
     (do_left)  ? 'Y' : 'N',
     (do_right) ? 'Y' : 'N');

  mixer1.gain (0, do_left  ? gain : 0.0f);
  mixer2.gain (0, do_right ? gain : 0.0f);

  if (!tonesweep.play (t_ampx, t_lox, t_hix, t_timex)) {
    Serial.println ("ToneSweep - play failed");
    while (1)
      ;
  }

  // wait for the sweep to end
  while (tonesweep.isPlaying ())
    ;

  // and now reverse the sweep
  Serial.printf ("Sweep down, left = %c, right = %c\n",
     (do_left)  ? 'Y' : 'N',
     (do_right) ? 'Y' : 'N');

  if (!tonesweep.play (t_ampx, t_hix, t_lox, t_timex)) {
    Serial.println("ToneSweep - play failed");
    while (1)
      ;
  }

  // wait for the sweep to end
  while (tonesweep.isPlaying ())
    ;

  Serial.println ("Sweep done");
}

void setup(void)
{
  // Wait for at least 3 seconds for the USB serial connection
  Serial.begin (9600);
  while (!Serial && millis () < 3000)
    ;

  AudioMemory (8);
  Serial.println ("setup done");

  for (int i = 1; i <= 3; i++)
    do_sweep (i);

  Serial.println ("Done");
}

void loop (void)
{
}
