//This is the pseudo register map of the product. If user asks for 0x02 then get the 3rd
//byte inside the register map.
//5602/118 on ATtiny84 prior to conversion
struct memoryMap{
  byte firmwareMajor;
  byte firmwareMinor;
  byte interruptEnable;
  byte status;
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

//This defines which of the registers are read-only (0) vs read-write (1)
memoryMap protectionMap = {
  .firmwareMajor = 0x00,
};
volatile memoryMap registerMap;
volatile memoryMap tempMap;

void setup() {
  Serial.begin(9600);
  Serial.println("Memory Mapping Test");

  registerMap.encoderCount = 0x1BCD;
  registerMap.encoderDifference = 0x123F;
  registerMap.i2cAddress = 0x55;
  
  uint8_t *bytePointer = (uint8_t *)&registerMap; 
  //Cast 32bit address of the object configurationSpace with uint8_t so we can increment the pointer

  byte test = *(bytePointer + 1); //Should be 0x1B

  Serial.print("registerMap Located at: 0x");
  Serial.println((uint32_t)&registerMap, HEX);
  
  Serial.print("registerMap.encoderCount: 0x");
  Serial.println((int16_t)registerMap.encoderCount, HEX);

  Serial.print("registerMap.i2cAddress: 0x");
  Serial.println(registerMap.i2cAddress, HEX);

  Serial.print("test: 0x");
  Serial.println(test, HEX);
}

void loop() {

}
