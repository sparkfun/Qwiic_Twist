
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

enum encoderCommands {
  COMMAND_GET_VERSION = 0x00,
  COMMAND_GET_ENCODER_COUNT,
  COMMAND_SET_ENCODER_COUNT,
  COMMAND_GET_DIFFERENCE,
  COMMAND_GET_BUTTON_STATE, //Current state of button
  COMMAND_GET_LAST_ENCODER_EVENT, //Millis since last movement of knob
  COMMAND_GET_LAST_BUTTON_EVENT, //Millis since last press/release
  COMMAND_IS_CLICKED, //True if user has pressed and released button since last isClicked() command

  COMMAND_GET_RED,
  COMMAND_GET_GREEN,
  COMMAND_GET_BLUE,
  COMMAND_GET_COLOR, //Get all three colors
  COMMAND_SET_RED,
  COMMAND_SET_GREEN,
  COMMAND_SET_BLUE,
  COMMAND_SET_COLOR,

  COMMAND_GET_CONNECT_RED,
  COMMAND_GET_CONNECT_GREEN,
  COMMAND_GET_CONNECT_BLUE,
  COMMAND_GET_CONNECT_COLOR,
  COMMAND_SET_CONNECT_RED, //Amount to change red LED for each encoder tick
  COMMAND_SET_CONNECT_GREEN,
  COMMAND_SET_CONNECT_BLUE,
  COMMAND_SET_CONNECT_COLOR,

  COMMAND_GET_TURN_INT_TIMEOUT,
  COMMAND_SET_TURN_INT_TIMEOUT,
  COMMAND_CHANGE_ADDRESS = 0xC7,
};

void setup()
{
  pinMode(interruptPin, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("Qwiic Twist Example");

  Wire.begin();

  setEncoderColor(255, 255, 255); //R, G, B - max of 255
  connectGreen(1);
  connectRed(-1);
}

void loop()
{
  if(digitalRead(interruptPin) == LOW)
  {
    Serial.print("Int! ");
  }

  int counts = getCounts();
  Serial.print("Encoder: ");
  Serial.print(counts);

  //int diff = getDiff(); //Get number of ticks different from last reading
  //Serial.print("Diff: ");
  //Serial.println(diff);

  if (buttonPressed())
  {
    Serial.print(" Pressed!");
  }

  Serial.println();


  delay(100);
}

//Returns true if a click event has occured
boolean buttonPressed()
{
  if (readRegister(COMMAND_IS_CLICKED)) return(true);
  return(false);
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectGreen(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(COMMAND_SET_CONNECT_GREEN);
  Wire.write(amount >> 8); //MSB
  Wire.write(amount & 0xFF); //LSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectBlue(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(COMMAND_SET_CONNECT_BLUE);
  Wire.write(amount >> 8); //MSB
  Wire.write(amount & 0xFF); //LSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Connect the LED so it changes [amount] with each encoder tick
//Negative numbers are allowed (so LED gets brighter the more you turn the encoder down)
boolean connectRed(int amount)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(COMMAND_SET_CONNECT_RED);
  Wire.write(amount >> 8); //MSB
  Wire.write(amount & 0xFF); //LSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}

//Sets the color of the encoder LEDs
boolean setEncoderColor(uint8_t red, uint8_t green, uint8_t blue)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(COMMAND_SET_COLOR); //Command
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
  return (readRegister16(COMMAND_GET_ENCODER_COUNT));
}

//Returns the number of ticks since last check
int getDiff()
{
  return (readRegister16(COMMAND_GET_DIFFERENCE));

  Wire.beginTransmission(deviceAddress);
  Wire.write(COMMAND_GET_DIFFERENCE);
  if (Wire.endTransmission() != 0)
  {
    Serial.println("No ack!");
    return (0); //Sensor did not ACK
  }

  Wire.requestFrom((uint8_t)deviceAddress, (uint8_t)2);
  if (Wire.available())
  {
    return ((int16_t)Wire.read() << 8 | Wire.read());
  }
  return (0); //Sensor did not respond
}
