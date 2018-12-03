
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

#include <Wire.h>

byte deviceAddress = 0x3F;

const byte interruptPin = 7;

const byte statusButtonClickedBit = 2;
const byte statusButtonPressedBit = 1;
const byte statusEncoderMovedBit = 0;

const byte enableInterruptButtonBit = 1;
const byte enableInterruptEncoderBit = 0;

enum encoderRegisters {
  TWIST_ID = 0x00,
  TWIST_STATUS = 0x01, //2 - button clicked, 1 - button pressed, 0 - encoder moved
  TWIST_VERSION = 0x02,
  TWIST_ENABLE_INTS = 0x04, //1 - button interrupt, 0 - encoder interrupt
  TWIST_COUNT = 0x05,
  TWIST_DIFFERENCE = 0x07,
  TWIST_LAST_ENCODER_EVENT = 0x09, //Millis since last movement of knob
  TWIST_LAST_BUTTON_EVENT = 0x0A, //Millis since last press/release

  TWIST_RED = 0x0D,
  TWIST_GREEN = 0x0E,
  TWIST_BLUE = 0x0F,

  TWIST_CONNECT_RED = 0x10, //Amount to change red LED for each encoder tick
  TWIST_CONNECT_GREEN = 0x12,
  TWIST_CONNECT_BLUE = 0x14,

  TWIST_TURN_INT_TIMEOUT = 0x16,
  TWIST_CHANGE_ADDRESS = 0x18,
};

void setup()
{
  pinMode(interruptPin, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Qwiic Twist Example");

  Wire.begin();

  //int result = (readRegister(0));
  //Serial.print("result: 0x");
  //Serial.println(result, HEX);
  //while(1);
  //setEncoderColor(255, 255, 255); //R, G, B - max of 255
  //connectBlue(1);
  //connectRed(-1);
}

void loop()
{
  if (digitalRead(interruptPin) == LOW)
  {
    Serial.print("Int! ");
  }

  int counts = getCounts();
  Serial.print("Encoder: ");
  Serial.print(counts);

  int diff = getDiff(); //Get number of ticks different from last reading
  Serial.print(" Diff: ");
  Serial.print(diff);
  
  if (buttonPressed())
  {
    Serial.print(" Pressed!");
  }

  Serial.println();

  delay(1000);
}

//Returns true if a click event has occured
boolean buttonPressed()
{
  return (readRegister(TWIST_STATUS) & (1 << statusButtonPressedBit));
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectGreen(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(TWIST_CONNECT_GREEN);
  Wire.write(amount & 0xFF); //LSB
  Wire.write(amount >> 8); //MSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectBlue(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(TWIST_CONNECT_BLUE);
  Wire.write(amount & 0xFF); //LSB
  Wire.write(amount >> 8); //MSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectRed(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(TWIST_CONNECT_RED);
  Wire.write(amount & 0xFF); //LSB
  Wire.write(amount >> 8); //MSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Sets the color of the encoder LEDs
boolean setEncoderColor(uint8_t red, uint8_t green, uint8_t blue)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(TWIST_RED); //Start at red memory location
  Wire.write(red);
  Wire.write(green);
  Wire.write(blue);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Returns the number of accrued encoder counts
int getCounts()
{
  return (readRegister16(TWIST_COUNT));
}

//Returns the number of ticks since last check
int getDiff()
{
  return (readRegister16(TWIST_DIFFERENCE));
}
