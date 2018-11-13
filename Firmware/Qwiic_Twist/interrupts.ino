
//Called every time the encoder is twisted
//Based on http://bildr.org/2012/08/rotary-encoder-arduino/
//Adam - I owe you many beers for bildr
void updateEncoder() {
  byte MSB = digitalRead(encoderAPin); //MSB = most significant bit
  byte LSB = digitalRead(encoderBPin); //LSB = least significant bit

  byte encoded = (MSB << 1) | LSB; //Convert the 2 pin value to single number
  lastEncoded = (lastEncoded << 2) | encoded; //Add this to the previous readings

  //A complete indent occurs across four interrupts and looks like:
  //ABAB.ABAB = 01001011 for CW
  //ABAB.ABAB = 10000111 for CCW

  //lastEncoded could be many things but by looking for two unique values
  //we filter out corrupted and partially dropped encoder readings
  //Gaurantees we will not get partial indent readings

  if (lastEncoded == 0b01001011) //One indent clockwise
  {
    encoderValue++;
    encoderDifference++;

    //If the user has enabled connection between a color and the knob, change LED brightness here
    if (settingRedConnectAmount != 0)
    {
      int newRed = settingRedLED + settingRedConnectAmount; //May go negative if setting is <0
      if (newRed > 255) newRed = 255; //Cap at max
      if (newRed < 0) newRed = 0; //Cap at min
      settingRedLED = newRed;

      analogWrite(ledRedPin, 255 - settingRedLED);
    }
    if (settingGreenConnectAmount != 0)
    {
      int newGreen = settingGreenLED + settingGreenConnectAmount; //May go negative if setting is <0
      if (newGreen > 255) newGreen = 255; //Cap at max
      if (newGreen < 0) newGreen = 0; //Cap at min
      settingGreenLED = newGreen;

      analogWrite(ledGreenPin, 255 - settingGreenLED);
    }
    if (settingBlueConnectAmount != 0)
    {
      int newBlue = settingBlueLED + settingBlueConnectAmount; //May go negative if setting is <0
      if (newBlue > 255) newBlue = 255; //Cap at max
      if (newBlue < 0) newBlue = 0; //Cap at min
      settingBlueLED = newBlue;

      analogWrite(ledBluePin, 255 - settingBlueLED);
    }
  }
  else if (lastEncoded == 0b10000111) //One indent counter clockwise
  {
    encoderValue--;
    encoderDifference--;

    //If the user has enabled connection between a color and the knob, change LED brightness here
    if (settingRedConnectAmount != 0)
    {
      int newRed = settingRedLED - settingRedConnectAmount; //May increase if setting is <0
      if (newRed > 255) newRed = 255; //Cap at max
      if (newRed < 0) newRed = 0; //Cap at min
      settingRedLED = newRed;

      analogWrite(ledRedPin, 255 - settingRedLED);
    }
    if (settingGreenConnectAmount != 0)
    {
      int newGreen = settingGreenLED - settingGreenConnectAmount; //May increase if setting is <0
      if (newGreen > 255) newGreen = 255; //Cap at max
      if (newGreen < 0) newGreen = 0; //Cap at min
      settingGreenLED = newGreen;

      analogWrite(ledGreenPin, 255 - settingGreenLED);
    }
    if (settingBlueConnectAmount != 0)
    {
      int newBlue = settingBlueLED - settingBlueConnectAmount; //May increase if setting is <0
      if (newBlue > 255) newBlue = 255; //Cap at max
      if (newBlue < 0) newBlue = 0; //Cap at min
      settingBlueLED = newBlue;

      analogWrite(ledBluePin, 255 - settingBlueLED);
    }
  }

  lastEncoderTwistTime = millis(); //Timestamp this event
  //We will set the general int pin when the user has stopped turning for more than settingTurnInterruptTimeout
  twistInterrupt = true; //We have a new event to cause the interrupt to fire
}

//Turn on interrupts for the various pins
void setupInterrupts()
{
  //Call updateEncoder() when any high/low changed seen on interrupt 0 (pin 2), or interrupt 1 (pin 3)
#if defined(__AVR_ATmega328P__)
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
#endif

#if defined(__AVR_ATtiny84__)
  //To make this line work you have to comment out https://github.com/NicoHood/PinChangeInterrupt/blob/master/src/PinChangeInterruptSettings.h#L228
  attachPCINT(digitalPinToPCINT(encoderAPin), updateEncoder, CHANGE); //Doesn't work

  attachPCINT(digitalPinToPCINT(encoderBPin), updateEncoder, CHANGE);
#endif

  //Attach interrupt to switch
  attachPCINT(digitalPinToPCINT(switchPin), buttonInterrupt, CHANGE);
}

//When KeyPad receives data bytes, this function is called as an interrupt
//The only valid command we can receive from the master is the change I2C adddress command
void receiveEvent(int numberOfBytesReceived)
{
  while (Wire.available())
  {
    //Record bytes to local array
    byte incoming = Wire.read();

    if (incoming == COMMAND_GET_VERSION)
    {
      responseType = RESPONSE_GET_VERSION;
    }

    else if (incoming == COMMAND_GET_ENCODER_COUNT)
    {
      responseType = RESPONSE_GET_ENCODER_COUNT;
    }

    //Tell the encoder to go to a value - often used to 'zero' or reset the encoder
    else if (incoming == COMMAND_SET_ENCODER_COUNT)
    {
      if (Wire.available() > 1)
      {
        encoderValue = Wire.read() << 8;
        encoderValue |= Wire.read();
      }
    }

    else if (incoming == COMMAND_GET_DIFFERENCE)
    {
      responseType = RESPONSE_GET_DIFFERENCE;
    }

    else if (incoming == COMMAND_GET_BUTTON_STATE)
    {
      responseType = RESPONSE_GET_BUTTON_STATE;
    }

    else if (incoming == COMMAND_GET_LAST_BUTTON_EVENT)
    {
      responseType = RESPONSE_GET_LAST_BUTTON_EVENT;
    }

    else if (incoming == COMMAND_GET_LAST_ENCODER_EVENT)
    {
      responseType = RESPONSE_GET_LAST_ENCODER_EVENT;
    }

    else if (incoming == COMMAND_IS_CLICKED)
    {
      responseType = RESPONSE_IS_CLICKED;
    }

    //Get each of the LED colors, or all three
    else if (incoming == COMMAND_GET_RED)
    {
      responseType = RESPONSE_GET_RED;
    }
    else if (incoming == COMMAND_GET_GREEN)
    {
      responseType = RESPONSE_GET_GREEN;
    }
    else if (incoming == COMMAND_GET_BLUE)
    {
      responseType = RESPONSE_GET_BLUE;
    }
    else if (incoming == COMMAND_GET_COLOR)
    {
      responseType = RESPONSE_GET_COLOR;
    }

    //Set each LED color, or all three
    else if (incoming == COMMAND_SET_RED)
    {
      if (Wire.available())
      {
        settingRedLED = Wire.read();
        EEPROM.write(LOCATION_RED_BRIGHTNESS, settingRedLED);
        analogWrite(ledRedPin, 255 - settingRedLED); //Change LED brightness
      }
    }

    else if (incoming == COMMAND_SET_GREEN)
    {
      if (Wire.available())
      {
        settingGreenLED = Wire.read();
        EEPROM.write(LOCATION_GREEN_BRIGHTNESS, settingGreenLED);
        analogWrite(ledGreenPin, 255 - settingGreenLED); //Change LED brightness
      }
    }

    else if (incoming == COMMAND_SET_BLUE)
    {
      if (Wire.available())
      {
        settingBlueLED = Wire.read();
        EEPROM.write(LOCATION_BLUE_BRIGHTNESS, settingBlueLED);
        analogWrite(ledBluePin, 255 - settingBlueLED); //Change LED brightness
      }
    }

    else if (incoming == COMMAND_SET_COLOR) //Read all three values at once
    {
      if (Wire.available() > 2)
      {
        settingRedLED = Wire.read();
        settingGreenLED = Wire.read();
        settingBlueLED = Wire.read();

        EEPROM.write(LOCATION_RED_BRIGHTNESS, settingRedLED);
        EEPROM.write(LOCATION_GREEN_BRIGHTNESS, settingGreenLED);
        EEPROM.write(LOCATION_BLUE_BRIGHTNESS, settingBlueLED);

        analogWrite(ledRedPin, 255 - settingRedLED); //Change LED brightness
        analogWrite(ledGreenPin, 255 - settingGreenLED); //Change LED brightness
        analogWrite(ledBluePin, 255 - settingBlueLED); //Change LED brightness
      }
    }

    //Get the connection values
    else if (incoming == COMMAND_GET_CONNECT_RED)
    {
      responseType = RESPONSE_GET_CONNECT_RED;
    }
    else if (incoming == COMMAND_GET_CONNECT_GREEN)
    {
      responseType = RESPONSE_GET_CONNECT_GREEN;
    }
    else if (incoming == COMMAND_GET_CONNECT_BLUE)
    {
      responseType = RESPONSE_GET_CONNECT_BLUE;
    }
    else if (incoming == COMMAND_GET_CONNECT_COLOR)
    {
      responseType = RESPONSE_GET_CONNECT_COLOR;
    }

    //Set each LED connection value, or all three
    else if (incoming == COMMAND_SET_CONNECT_RED)
    {
      if (Wire.available())
      {
        settingRedConnectAmount = Wire.read() << 8;
        settingRedConnectAmount |= Wire.read();
        EEPROM.put(LOCATION_RED_CONNECT_AMOUNT, settingRedConnectAmount);
      }
    }
    else if (incoming == COMMAND_SET_CONNECT_GREEN)
    {
      if (Wire.available() > 1)
      {
        settingGreenConnectAmount = Wire.read() << 8;
        settingGreenConnectAmount |= Wire.read();
        EEPROM.put(LOCATION_GREEN_CONNECT_AMOUNT, settingGreenConnectAmount);
      }
    }
    else if (incoming == COMMAND_SET_CONNECT_BLUE)
    {
      if (Wire.available())
      {
        settingBlueConnectAmount = Wire.read() << 8;
        settingBlueConnectAmount |= Wire.read();
        EEPROM.put(LOCATION_BLUE_CONNECT_AMOUNT, settingBlueConnectAmount);
      }
    }
    else if (incoming == COMMAND_SET_CONNECT_COLOR)
    {
      if (Wire.available() > 5)
      {
        settingRedConnectAmount = Wire.read() << 8;
        settingRedConnectAmount |= Wire.read();
        settingGreenConnectAmount = Wire.read() << 8;
        settingGreenConnectAmount |= Wire.read();
        settingBlueConnectAmount = Wire.read() << 8;
        settingBlueConnectAmount |= Wire.read();
        EEPROM.put(LOCATION_RED_CONNECT_AMOUNT, settingRedConnectAmount);
        EEPROM.put(LOCATION_GREEN_CONNECT_AMOUNT, settingGreenConnectAmount);
        EEPROM.put(LOCATION_BLUE_CONNECT_AMOUNT, settingBlueConnectAmount);
      }
    }

    else if (incoming == COMMAND_GET_TURN_INT_TIMEOUT) //Respond with the timeout amount
    {
      responseType = RESPONSE_GET_TURN_INT_TIMEOUT;
    }
    else if (incoming == COMMAND_SET_TURN_INT_TIMEOUT) //Get two bytes to form interrupt timeout
    {
      if (Wire.available() > 1)
      {
        settingTurnInterruptTimeout = Wire.read() << 8;
        settingTurnInterruptTimeout |= Wire.read();
      }
    }

    else if (incoming == COMMAND_CHANGE_ADDRESS) //Set new I2C address
    {
      if (Wire.available())
      {
        settingI2CAddress = Wire.read();

        //Error check
        if (settingI2CAddress < 0x08 || settingI2CAddress > 0x77)
          continue; //Command failed. This address is out of bounds.

        EEPROM.write(LOCATION_I2C_ADDRESS, settingI2CAddress);

        //Our I2C address may have changed because of user's command
        startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus
      }
    }
  }
}

//Respond to GET commands
//When Twist gets a request for data from the user, this function is called as an interrupt
//The interrupt will respond with different types of data depending on what response state we are in
void requestEvent()
{
  if (responseType == RESPONSE_GET_VERSION)
  {
    Wire.write(firmwareVersionMajor);
    Wire.write(firmwareVersionMinor);
  }
  else if (responseType == RESPONSE_GET_ENCODER_COUNT)
  {
    Wire.write(encoderValue >> 8); //MSB
    Wire.write(encoderValue & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_DIFFERENCE)
  {
    Wire.write(encoderDifference >> 8); //MSB
    Wire.write(encoderDifference & 0xFF); //LSB

    //Once written to caller, reset the variable
    encoderDifference = 0;
  }
  else if (responseType == RESPONSE_GET_BUTTON_STATE)
  {
    Wire.write(buttonPressed);
  }
  else if (responseType == RESPONSE_GET_LAST_ENCODER_EVENT)
  {
    //Report when the encoder was last turned
    unsigned long timeSinceLastKnob = millis() - lastEncoderTwistTime;
    Wire.write(timeSinceLastKnob >> 8); //MSB 
    Wire.write(timeSinceLastKnob & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_LAST_BUTTON_EVENT)
  {
    //Report the time when the last button press occured
    unsigned long timeSinceLastButton = millis() - lastButtonTime;
    Wire.write(timeSinceLastButton >> 8); //MSB 
    Wire.write(timeSinceLastButton & 0xFF); //LSB
  }

  //Respond with each of 3 LED color values or all 3 at once
  else if (responseType == RESPONSE_GET_RED)
  {
    Wire.write(settingRedLED); //Report this LED amount
  }
  else if (responseType == RESPONSE_GET_GREEN)
  {
    Wire.write(settingGreenLED); //Report this LED amount
  }
  else if (responseType == RESPONSE_GET_BLUE)
  {
    Wire.write(settingBlueLED); //Report this LED amount
  }
  else if (responseType == RESPONSE_GET_COLOR)
  {
    //Report all three values
    Wire.write(settingRedLED);
    Wire.write(settingGreenLED);
    Wire.write(settingBlueLED);
  }

  //Respond with each of 3 connection values or all 3 at once
  else if (responseType == RESPONSE_GET_CONNECT_RED)
  {
    Wire.write(settingRedConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingRedConnectAmount & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_CONNECT_GREEN)
  {
    Wire.write(settingGreenConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingGreenConnectAmount & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_CONNECT_BLUE)
  {
    Wire.write(settingBlueConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingBlueConnectAmount & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_CONNECT_COLOR)
  {
    //Report all three values
    Wire.write(settingRedConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingRedConnectAmount & 0xFF); //LSB
    Wire.write(settingGreenConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingGreenConnectAmount & 0xFF); //LSB
    Wire.write(settingBlueConnectAmount >> 8); //MSB - Report connection amount
    Wire.write(settingBlueConnectAmount & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_GET_TURN_INT_TIMEOUT)
  {
    Wire.write(settingTurnInterruptTimeout >> 8); //MSB
    Wire.write(settingTurnInterruptTimeout & 0xFF); //LSB
  }
  else if (responseType == RESPONSE_IS_CLICKED)
  {
    //Report true if the button has been toggled
    //This is cleared each time the user queries this setting
    Wire.write(isClicked);

    isClicked = false; //Reset the variable after each read
  }

  //Clear the interrupt pin once user has requested something
  interruptOn = false;
  pinMode(interruptPin, INPUT); //Go to high impedance
}


//Called any time the pin changes state
void buttonInterrupt()
{
  buttonPressed = !buttonPressed;

  if (buttonPressed == false) //User has released the button, we have completed a click cycle
  {
    isClicked = true; //This is used to respond to the IS_CLICKED command query
    lastButtonTime = millis();
  }

  //Set the interrupt pin to indicate something new has happened
  if (interruptOn == false)
  {
    interruptOn = true;
    pinMode(interruptPin, OUTPUT); //Go to output to indicate interrupt
    digitalWrite(interruptPin, LOW); //Pull pin low
  }
}
