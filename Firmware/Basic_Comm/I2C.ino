
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

  Wire.requestFrom((int)deviceAddress, (int)3);
  if (Wire.available())
  {
    return (Wire.read());
  }

  Serial.println("Error: Sensor did not respond");
  return (0);
}

//Reads two consecutive bytes from a given location
uint16_t readRegister16(uint8_t addr)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr); //MSB
  if (Wire.endTransmission() != 0)
  {
    Serial.println("No ack!");
    return (0); //Sensor did not ACK
  }

  Wire.requestFrom((uint8_t)deviceAddress, (uint8_t)2);
  if (Wire.available())
  {
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    return ((uint16_t)msb << 8 | lsb);
  }
  Serial.println("No data!");
  return (0); //Sensor did not respond
}


//Reads 5 consecutive bytes from a given location
uint16_t readBlock(uint8_t addr)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr);
  if (Wire.endTransmission() != 0)
    return (0); //Sensor did not ACK

  Wire.requestFrom((uint8_t)deviceAddress, (uint8_t)5);
  if (Wire.available())
  {
    for(int x = 0 ; x < 5 ; x++)
    {
      Serial.print(x);
      Serial.print(": 0x");
      Serial.print(Wire.read(), HEX);
      Serial.println();
    }
  }
  return (0); //Sensor did not respond
}

//Write 5 bytes to a spot
boolean writeBlock(uint8_t addr)
{
  Wire.beginTransmission(deviceAddress);
  Wire.write(addr);
  Wire.write(0x1A);
  Wire.write(0x2B);
  Wire.write(0x3C);
  Wire.write(0x4D);
  Wire.write(0x5E);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  return (true);
}
