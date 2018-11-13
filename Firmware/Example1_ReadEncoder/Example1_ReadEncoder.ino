
/*
  Reading the encoder value from the Qwiic Twist
  By: Nathan Seidle
  SparkFun Electronics
  Date: November 6th, 2018
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15083

  Hardware Connections:
  Attach the Twist to the Qwiic port on your development board
  Don't have a Qwiic board? A Qwiic breadboard cable will help:
  Serial.print it out at 9600 baud to serial monitor.

  Available:


*/
#include "SparkFun_Qwiic_Twist_Arduino_Library.h"
#include <Wire.h>

byte deviceAddress = 0x3F;

const byte interruptPin = 7;

TWIST knob; //Create instance of Qwiic Twist encoder

void setup()
{
  pinMode(interruptPin, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Qwiic Twist Example");

  Wire.begin();

  if (knob.begin() == false)
  {
    Serial.println("Qwiic twist not detected. Check wiring. Freezing...");
    while (1); //Freeze!
  }
  Serial.println("Qwiic Twist Detected!");

  //You can set the individual LED values. 0 to 255 (100%)
  //Set encoder to aqua
  knob.setRed(20);
  knob.setGreen(50);
  knob.setBlue(150);

  //Or you can set all three colors one on command - this is faster
  //knob.setColor(255, 255, 255); //R, G, B - max of 255

  knob.setColor(255, 0, 0); //Turn only red on, 100%
  knob.connectBlue(10); //Make blue go up 10 when user turns knob CW and down 10 when user turns CCW
  knob.connectRed(-10); //Make red go down 10 when user turns know CW and up 10 when user turns CCW

  unsigned int version = knob.getVersion();
  Serial.print("Qwiic Twist Version - Major: 0x");
  Serial.print(version >> 8, HEX);
  Serial.print(" Minor: 0x");
  Serial.println(version & 0xFF, HEX);

  knob.setCount(4000); //Set the encoder amount to 4000
}

void loop()
{
  if (digitalRead(interruptPin) == LOW)
  {
    Serial.print("Int! ");
  }

  int counts = knob.getCount();
  Serial.print("Encoder: ");
  Serial.print(counts);

  int diff = knob.getDiff(); //Get number of ticks different from last reading
  Serial.print(" Diff: ");
  Serial.print(diff);

  unsigned int millisSinceKnobMovement = knob.getLastKnobEvent(); //Get the number of milliseconds since the last encoder movement
  Serial.print(" Time Since Knob Moved:");
  Serial.print(millisSinceKnobMovement);

  unsigned int millisSinceButtonEvent = knob.getLastButtonEvent(); //Get the number of milliseconds since the last button press
  Serial.print(" Time Since Button Pressed:");
  Serial.print(millisSinceButtonEvent);

  byte redValue = knob.getRed(); //Get current value of the LED
  Serial.print(" Red:");
  Serial.print(redValue);

  int blueConnectValue = knob.getBlueConnect(); //Get the amount connected to blue LED
  Serial.print(" BlueConnect:");
  Serial.print(blueConnectValue);

  int timeout = knob.getIntTimeout(); //Get number of milliseconds that must elapse between end of knob turning and interrupt firing
  Serial.print(" timeout:");
  Serial.print(timeout);

  if(knob.getButtonState() == false)
    Serial.print(" Not pressed");
  else
    Serial.print(" Pressed");
    
  if(knob.buttonPressed()) Serial.print(" Button event!");

  Serial.println();

  delay(100); //Don't poll too quickly, this just causes unneeded I2C traffic
}
