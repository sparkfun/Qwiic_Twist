/*
  An I2C based Rotary Encoder
  By: Nathan Seidle
  SparkFun Electronics
  Date: October 27th, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Qwiic Twist is an I2C based rotary encoder that tracks how much the dial has been turned.
  It also offers RGB control of the illumination of the encoder (on applicable models).

  For example, if you Wire.request(0x3F, 2) you'll get two bytes from Qwiic Twist and they might read:
  byte 0: 0x01 - The MSB of a 16-bit signed integer
  byte 1: 0x5A - The LSB of a 16-bit signed integer

  0x015A = 346 so the rotary dial has been turned 346 'ticks' since power on

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15083

  To install support for ATtiny84 in Arduino IDE: https://github.com/SpenceKonde/ATTinyCore/blob/master/Installation.md
  This core is installed from the Board Manager menu
  This core has built in support for I2C S/M and serial
  If you have Dave Mellis' ATtiny installed you may need to remove it from \Users\xx\AppData\Local\Arduino15\Packages

  To support 400kHz I2C communication reliably ATtiny84 needs to run at 8MHz. This requires user to
  click on 'Burn Bootloader' before code is loaded.

  TODO -
  We don't track which reason the interrupt pin was set and which feature the user has read
*/

#include <Wire.h>
#include <EEPROM.h>
#include "nvm.h"

#include "PinChangeInterrupt.h" //Nico Hood's library: https://github.com/NicoHood/PinChangeInterrupt/
//Used for pin change interrupts on ATtinys (encoder button causes interrupt)
//Note: To make this code work with Nico's library you have to comment out https://github.com/NicoHood/PinChangeInterrupt/blob/master/src/PinChangeInterruptSettings.h#L228

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#if defined(__AVR_ATmega328P__)
//Hardware connections while developing with an Uno
const byte addressPin = 6;
const byte ledRedPin = 9; //PWM
const byte ledGreenPin = 10; //PWM
const byte ledBluePin = 5; //PWM
const byte switchPin = 4;
const byte encoderBPin = 3; //Encoder must be on 2/3 on Uno
const byte encoderAPin = 2;
const byte interruptPin = 7; //Pin goes low when a button event is available

#elif defined(__AVR_ATtiny84__)
//Hardware connections for the final design
const byte addressPin = 9;
const byte ledRedPin = 8; //PWM
const byte ledGreenPin = 7; //PWM
const byte ledBluePin = 5; //PWM
const byte switchPin = 3;
const byte encoderBPin = 2;
const byte encoderAPin = 10;
const byte interruptPin = 0;
#endif

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//Variables used in the I2C interrupt so we use volatile

//This is the pseudo register map of the product. If user asks for 0x02 then get the 3rd
//byte inside the register map.
//5602/118 on ATtiny84 prior to conversion
//Approximately 4276/156 on ATtiny84 after conversion
struct memoryMap {
  byte id;
  byte status;
  byte firmwareMajor;
  byte firmwareMinor;
  byte interruptEnable;
  int16_t encoderCount;
  int16_t encoderDifference;
  uint16_t timeSinceLastMovement;
  uint16_t timeSinceLastButton;
  byte ledBrightnessRed;
  byte ledBrightnessGreen;
  byte ledBrightnessBlue;
  int16_t ledConnectRed;
  int16_t ledConnectGreen;
  int16_t ledConnectBlue;
  uint16_t turnInterruptTimeout;
  byte i2cAddress;
};

const byte statusButtonClickedBit = 2;
const byte statusButtonPressedBit = 1;
const byte statusEncoderMovedBit = 0;

const byte enableInterruptButtonBit = 1;
const byte enableInterruptEncoderBit = 0;

//These are the defaults for all settings
volatile memoryMap registerMap = {
  .id = 0x5C,
  .status = 0x00, //2 - button clicked, 1 - button pressed, 0 - encoder moved
  .firmwareMajor = 0x01, //Firmware version. Helpful for tech support.
  .firmwareMinor = 0x00,
  .interruptEnable = 0x03, //1 - button interrupt, 0 - encoder interrupt
  .encoderCount = 0x0000,
  .encoderDifference = 0x0000,
  .timeSinceLastMovement = 0x0000,
  .timeSinceLastButton = 0x0000,
  .ledBrightnessRed = 0xFF,
  .ledBrightnessGreen = 0xFF,
  .ledBrightnessBlue = 0xFF,
  .ledConnectRed = 0x0000,
  .ledConnectGreen = 0x0000,
  .ledConnectBlue = 0x0000,
  .turnInterruptTimeout = 250,
  .i2cAddress = I2C_ADDRESS_DEFAULT,
};

//This defines which of the registers are read-only (0) vs read-write (1)
memoryMap protectionMap = {
  .id = 0x00,
  .status = 0xFF,
  .firmwareMajor = 0x00,
  .firmwareMinor = 0x00,
  .interruptEnable = 0xFF,
  .encoderCount = 0xFFFF,
  .encoderDifference = 0xFFFF,
  .timeSinceLastMovement = 0xFFFF,
  .timeSinceLastButton = 0xFFFF,
  .ledBrightnessRed = 0xFF,
  .ledBrightnessGreen = 0xFF,
  .ledBrightnessBlue = 0xFF,
  .ledConnectRed = 0xFFFF,
  .ledConnectGreen = 0xFFFF,
  .ledConnectBlue = 0xFFFF,
  .turnInterruptTimeout = 0xFFFF,
  .i2cAddress = 0xFF,
};

//Cast 32bit address of the object registerMap with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerMap;
uint8_t *protectionPointer = (uint8_t *)&protectionMap;

volatile byte registerNumber; //Gets set when user writes an address. We then serve the spot the user requested.

volatile boolean updateOutputs = false; //Goes true when we receive new bytes from user. Causes LEDs and things to update in main loop.

volatile byte lastEncoded = 0; //Used to compare encoder readings between interrupts. Helps detect turn direction.

volatile byte interruptIndicated = false; //Tracks the state of the int output pin - helps prevent the need of constantly writing to the pin

volatile unsigned long lastButtonTime; //Time stamp of last button event

volatile unsigned long lastEncoderTwistTime; //Time stamp of last twist.

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void setup(void)
{
  pinMode(addressPin, INPUT_PULLUP);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);
  pinMode(switchPin, INPUT); //No pull-up. It's pulled low with 10k
  pinMode(encoderBPin, INPUT); //No pull-up. External 10k
  pinMode(encoderAPin, INPUT); //No pull-up. External 10k
  pinMode(interruptPin, INPUT); //Interrupt is high-impedance until we have int (and then go low). Optional external pull up.

#if defined(__AVR_ATmega328P__)
  pinMode(addressPin, INPUT_PULLUP);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);
  pinMode(switchPin, INPUT); //No pull-up. It's pulled low with 10k
  pinMode(encoderBPin, INPUT_PULLUP);
  pinMode(encoderAPin, INPUT_PULLUP);
  pinMode(interruptPin, INPUT); //Interrupt is high-impedance until we have int (and then go low). Optional external pull up.
#endif

  turnOffExtraBits(); //Turn off all unused peripherals

  readSystemSettings(); //Load all system settings from EEPROM

  setupInterrupts(); //Enable pin change interrupts for I2C, encoder, switch, etc

  lastEncoderTwistTime = 0; //User has not yet twisted the encoder. Used for firing int pin.
  lastButtonTime = 0; //User has not yet pressed the encoder button.

  //Get the initial state of encoder pins
  //byte MSB = digitalRead(encoderAPin);
  //byte LSB = digitalRead(encoderBPin);
  //lastEncoded = (MSB << 1) | LSB; //Convert the 2 pin value to single number

  startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus

#if defined(__AVR_ATmega328P__)
  Serial.begin(9600);
  Serial.println("Qwiic Twist");
  Serial.print("Address: 0x");

  if (digitalRead(addressPin) == HIGH) //Default is HIGH, the address jumper is open
    Serial.print(registerMap.i2cAddress, HEX);
  else
    Serial.print(I2C_FORCED_ADDRESS, HEX);
  Serial.println();

#endif
}

void loop(void)
{
  //Check if interrupts are enabled
  if ( (registerMap.interruptEnable & (1 << enableInterruptEncoderBit) ) )
  {
    //See if user has turned the encoder at all
    if ( registerMap.status & (1 << statusEncoderMovedBit) )
    {
      //See if the user has stopped turning the encoder
      //If they have, see if this is a newTwist or if we've already asserted int
      if ( (millis() - lastEncoderTwistTime) > registerMap.turnInterruptTimeout)
      {
        indicateInterrupt(); //Drive INT pin low
      }
    }
  }

  if (updateOutputs == true)
  {
    updateOutputs = false;

    //Record anything new to EEPROM like connection values and LED values
    //It can take ~3.4ms to write EEPROM byte so we do that here instead of in interrupt
    recordSystemSettings();
  }

#if defined(__AVR_ATmega328P__)
  Serial.print("Encoder: ");
  Serial.print(registerMap.encoderCount);

  Serial.print(" Red: ");
  Serial.print(registerMap.ledBrightnessRed);

  Serial.print(" Blue: ");
  Serial.print(registerMap.ledBrightnessBlue);

  Serial.print(" Diff: ");
  Serial.print(registerMap.encoderDifference);

  Serial.print(" Reg: ");
  Serial.print(registerNumber);

  Serial.print(" RegValue: 0x");
  Serial.print(*(registerPointer + registerNumber), HEX);

  if (registerMap.status & (1 << statusButtonClickedBit) )
    Serial.print(" Click");

  Serial.println();
#endif

  //sleep_mode(); //Stop everything and go to sleep. Wake up from Encoder, Button, or I2C interrupts.
}

//If the current setting is different from that in EEPROM, update EEPROM
void recordSystemSettings(void)
{
  //I2C address is byte
  byte i2cAddr;

  EEPROM.get(LOCATION_I2C_ADDRESS, i2cAddr);
  if (i2cAddr != registerMap.i2cAddress)
  {
    EEPROM.put(LOCATION_I2C_ADDRESS, registerMap.i2cAddress);
    startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus
  }

  //LED values are bytes
  byte ledBrightness;

  EEPROM.get(LOCATION_RED_BRIGHTNESS, ledBrightness);
  if (ledBrightness != registerMap.ledBrightnessRed)
    EEPROM.put(LOCATION_RED_BRIGHTNESS, registerMap.ledBrightnessRed);
  analogWrite(ledRedPin, 255 - registerMap.ledBrightnessRed); //Change LED brightness

  EEPROM.get(LOCATION_GREEN_BRIGHTNESS, ledBrightness);
  if (ledBrightness != registerMap.ledBrightnessGreen)
    EEPROM.put(LOCATION_GREEN_BRIGHTNESS, registerMap.ledBrightnessGreen);
  analogWrite(ledGreenPin, 255 - registerMap.ledBrightnessGreen); //Change LED brightness

  EEPROM.get(LOCATION_BLUE_BRIGHTNESS, ledBrightness);
  if (ledBrightness != registerMap.ledBrightnessBlue)
    EEPROM.put(LOCATION_BLUE_BRIGHTNESS, registerMap.ledBrightnessBlue);
  analogWrite(ledBluePin, 255 - registerMap.ledBrightnessBlue); //Change LED brightness

  //Connect amounts are ints
  int16_t setting;

  EEPROM.get(LOCATION_RED_CONNECT_AMOUNT, setting);
  if (setting != registerMap.ledConnectRed)
    EEPROM.put(LOCATION_RED_CONNECT_AMOUNT, registerMap.ledConnectRed);

  EEPROM.get(LOCATION_GREEN_CONNECT_AMOUNT, setting);
  if (setting != registerMap.ledConnectGreen)
    EEPROM.put(LOCATION_GREEN_CONNECT_AMOUNT, registerMap.ledConnectGreen);

  EEPROM.get(LOCATION_BLUE_CONNECT_AMOUNT, setting);
  if (setting != registerMap.ledConnectBlue)
    EEPROM.put(LOCATION_BLUE_CONNECT_AMOUNT, registerMap.ledConnectBlue);

  //Turn Timeout is uint16_t
  uint16_t timeout;

  EEPROM.get(LOCATION_TURN_INTERRUPT_TIMEOUT_AMOUNT, timeout);
  if (timeout != registerMap.turnInterruptTimeout)
    EEPROM.put(LOCATION_TURN_INTERRUPT_TIMEOUT_AMOUNT, registerMap.turnInterruptTimeout);
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  //Read what I2C address we should use
  registerMap.i2cAddress = EEPROM.read(LOCATION_I2C_ADDRESS);
  if (registerMap.i2cAddress == 255)
  {
    registerMap.i2cAddress = I2C_ADDRESS_DEFAULT; //By default, we listen for I2C_ADDRESS_DEFAULT
    EEPROM.write(LOCATION_I2C_ADDRESS, registerMap.i2cAddress);
  }

  //Read the starting value for the red LED
  registerMap.ledBrightnessRed = EEPROM.read(LOCATION_RED_BRIGHTNESS);
  analogWrite(ledRedPin, 255 - registerMap.ledBrightnessRed);

  //Read the starting value for the green LED
  registerMap.ledBrightnessGreen = EEPROM.read(LOCATION_GREEN_BRIGHTNESS);
  analogWrite(ledGreenPin, 255 - registerMap.ledBrightnessGreen);

  //Read the starting value for the blue LED
  registerMap.ledBrightnessBlue = EEPROM.read(LOCATION_BLUE_BRIGHTNESS);
  analogWrite(ledBluePin, 255 - registerMap.ledBrightnessBlue);

  //Read the connection value for red color
  //There are 24 pulses per rotation on the encoder
  //For each pulse, how much does the user want red to go up (or down)
  EEPROM.get(LOCATION_RED_CONNECT_AMOUNT, registerMap.ledConnectRed); //16-bit
  if (registerMap.ledConnectRed == 0xFFFF)
  {
    registerMap.ledConnectRed = 0; //Default to no connection
    EEPROM.put(LOCATION_RED_CONNECT_AMOUNT, registerMap.ledConnectRed);
  }

  EEPROM.get(LOCATION_GREEN_CONNECT_AMOUNT, registerMap.ledConnectGreen);
  if (registerMap.ledConnectGreen == 0xFFFF)
  {
    registerMap.ledConnectGreen = 0; //Default to no connection
    EEPROM.put(LOCATION_GREEN_CONNECT_AMOUNT, registerMap.ledConnectGreen);
  }

  EEPROM.get(LOCATION_BLUE_CONNECT_AMOUNT, registerMap.ledConnectBlue);
  if (registerMap.ledConnectBlue == 0xFFFF)
  {
    registerMap.ledConnectBlue = 0; //Default to no connection
    EEPROM.put(LOCATION_BLUE_CONNECT_AMOUNT, registerMap.ledConnectBlue);
  }

  EEPROM.get(LOCATION_TURN_INTERRUPT_TIMEOUT_AMOUNT, registerMap.turnInterruptTimeout);
  if (registerMap.turnInterruptTimeout == 0xFFFF)
  {
    registerMap.turnInterruptTimeout = 250; //Default to 250ms
    EEPROM.put(LOCATION_TURN_INTERRUPT_TIMEOUT_AMOUNT, registerMap.turnInterruptTimeout);
  }
}

//Turn off anything we aren't going to use
void turnOffExtraBits()
{
  //Disable ADC
  ADCSRA = 0;

  //Disble Brown-Out Detect
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  //Power down various bits of hardware to lower power usage
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
}

//Begin listening on I2C bus as I2C slave using the global variable setting_i2c_address
void startI2C()
{
  Wire.end(); //Before we can change addresses we need to stop

  if (digitalRead(addressPin) == HIGH) //Default is HIGH, the address jumper is open
    Wire.begin(registerMap.i2cAddress); //Start I2C and answer calls using address from EEPROM
  else
    Wire.begin(I2C_FORCED_ADDRESS); //Force address to I2C_FORCED_ADDRESS if user has closed the solder jumper

  //The connections to the interrupts are severed when a Wire.begin occurs. So re-declare them.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void indicateInterrupt()
{
  if (interruptIndicated == false) //Is the interrupt pin currently being driven low?
  {
    interruptIndicated = true;
    pinMode(interruptPin, OUTPUT); //Go to output to indicate interrupt
    digitalWrite(interruptPin, LOW); //Pull pin low
  }
}
