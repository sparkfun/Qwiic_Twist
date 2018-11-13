//Define the size of the I2C buffer based on the platform the user has
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)

//I2C_BUFFER_LENGTH is defined in Wire.H
#define I2C_BUFFER_LENGTH BUFFER_LENGTH

#elif defined(__SAMD21G18A__)

//SAMD21 uses RingBuffer.h
#define I2C_BUFFER_LENGTH SERIAL_BUFFER_SIZE

#elif __MK20DX256__
//Teensy

#elif ARDUINO_ARCH_ESP32
//ESP32 based platforms

#else

//The catch-all default is 32
#define I2C_BUFFER_LENGTH 32

#endif
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



//Reads from a given location
//Stores the result at the provided outputPointer
uint8_t readRegister(uint8_t addr)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr);
  if (Wire.endTransmission(false) != 0) //Send a restart command. Do not release bus.
  {
    //Sensor did not ACK
    Serial.println("Error: Sensor did not ack");
  }

  Wire.requestFrom((uint8_t)deviceAddress, (uint8_t)1);
  if (Wire.available())
  {
    return (Wire.read());
  }

  Serial.println("Error: Sensor did not respond");
  return (0);
}

//Reads two consecutive bytes from a given location
uint16_t readRegister16(uint16_t addr)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr >> 8); //MSB
  Wire.write(addr & 0xFF); //LSB
  if (Wire.endTransmission() != 0)
    return (0); //Sensor did not ACK

  Wire.requestFrom((uint8_t)deviceAddress, (uint8_t)2);
  if (Wire.available())
  {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    return ((uint16_t)msb << 8 | lsb);
  }
  return (0); //Sensor did not respond
}

//Read a lot of bytes into an array of uints
//Call ex: readRegisterMany(0x2716, 73, &myData);
boolean readRegisterMany(uint16_t addr, uint8_t bytesRemaining, uint8_t *data)
{
  uint16_t dataSpot = 0; //Start at beginning of array

  //Setup a series of chunked 32 byte reads
  while (bytesRemaining > 0)
  {
    //If the device doesn't have auto-incrementing pointer then we have the re-send the address for each read
    Wire.beginTransmission(deviceAddress);
    Wire.write(addr >> 8); //MSB
    Wire.write(addr & 0xFF); //LSB
    if (Wire.endTransmission() != 0)
      return (0); //Sensor did not ACK


    uint16_t numberOfBytesToRead = bytesRemaining;
    if (numberOfBytesToRead > I2C_BUFFER_LENGTH) numberOfBytesToRead = I2C_BUFFER_LENGTH;

    Wire.requestFrom((uint8_t)deviceAddress, numberOfBytesToRead);
    if (Wire.available())
    {
      for (uint16_t x = 0 ; x < numberOfBytesToRead ; x++)
      {
        data[dataSpot++] = Wire.read();
      }
    }
    else
    {
      return (false); //Sensor did not respond
    }

    bytesRemaining -= numberOfBytesToRead;

    addr += (numberOfBytesToRead / 2); //Address advances by 1 spot, every two bytes read
  }
  return (true); //Success
}

//Read a lot of 2bytes uints into an array of uints
//Call ex: readRegisterMany(0x2716, 73, &myData);
boolean readRegisterMany16(uint16_t addr, uint16_t intsRemaining, uint16_t *data)
{
  uint16_t dataSpot = 0; //Start at beginning of array

  uint16_t bytesRemaining = intsRemaining * 2;

  //Setup a series of chunked 32 byte reads
  while (bytesRemaining > 0)
  {
    //If the device doesn't have auto-incrementing pointer then we have the re-send the address for each read
    Wire.beginTransmission(deviceAddress);
    Wire.write(addr >> 8); //MSB
    Wire.write(addr & 0xFF); //LSB
    if (Wire.endTransmission() != 0)
      return (0); //Sensor did not ACK


    uint16_t numberOfBytesToRead = bytesRemaining;
    if (numberOfBytesToRead > I2C_BUFFER_LENGTH) numberOfBytesToRead = I2C_BUFFER_LENGTH;

    Wire.requestFrom((uint8_t)deviceAddress, numberOfBytesToRead);
    if (Wire.available())
    {
      for (uint16_t x = 0 ; x < numberOfBytesToRead / 2 ; x++)
      {
        data[dataSpot] = Wire.read() << 8; //MSB
        data[dataSpot] |= Wire.read(); //LSB

        dataSpot++;
      }
    }
    else
    {
      return (false); //Sensor did not respond
    }

    bytesRemaining -= numberOfBytesToRead;

    addr += (numberOfBytesToRead / 2); //Address advances by 1 spot, every two bytes read
  }
  return (true); //Success

}

//Write a byte to a spot
boolean writeRegister(uint8_t addr, uint8_t val)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr);
  Wire.write(val);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}

//Write two bytes to a spot
boolean writeRegister16(uint16_t addr, uint16_t val)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr >> 8); //MSB
  Wire.write(addr & 0xFF); //LSB
  Wire.write(val >> 8); //MSB
  Wire.write(val & 0xFF); //LSB
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}
