// Simple test to see if we can interface with SD card. 
// Obtained from: https://forum.pjrc.com/threads/61088-Teensy-4-1-Cannot-access-Internal-SD-card

#include <SD.h>
#include <SPI.h>

void setup()
{
  if (!SD.begin(BUILTIN_SDCARD))
    Serial.println ("NO LUCK");
  else
    Serial.println ("SD WORKS!");
}

void loop()
{
  // nothing
}
