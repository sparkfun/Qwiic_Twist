struct space {
  int settingRed;
  int settingBlue;
  byte encoder;
} configurationSpace;

void setup() {
  Serial.begin(9600);
  Serial.println("Union Test");

  configurationSpace.settingRed = 0x1BCD;
  configurationSpace.settingBlue = 0x123F;
  configurationSpace.encoder = 0x55;
  
  uint8_t *bytePointer = (uint8_t *)&configurationSpace; 
  //Cast 32bit address of the object configurationSpace with uint8_t so we can increment the pointer

  byte test = *(bytePointer + 1); //Should be 0x1B

  Serial.print("configurationSpace: 0x");
  Serial.println((uint32_t)&configurationSpace, HEX);
  Serial.print("configurationSpace.settingRed: 0x");
  Serial.println((uint32_t)&configurationSpace.settingRed, HEX);
  Serial.print("configurationSpace.settingBlue: 0x");
  Serial.println((uint32_t)&configurationSpace.settingBlue, HEX);

  Serial.print("test: 0x");
  Serial.println(test, HEX);
}

void loop() {

}
