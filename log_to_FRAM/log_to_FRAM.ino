#include <Wire.h>
#include "Adafruit_FRAM_I2C.h"
#include "SparkFunBME280.h"
/* Example code for the Adafruit I2C FRAM breakout */

/* Connect SCL    to analog 5
   Connect SDA    to analog 4
   Connect VDD    to 5.0V DC
   Connect GROUND to common ground */
   
Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();
uint16_t          framAddr = 0;

//Global sensor object
BME280 mySensor;

struct {
  uint16_t last_pos;
  uint16_t start_pos;
  } FRAM_storage;

#define WAIT_MINUTES 10
#define FRAM_SIZE 32768
  
void setup(void) {
  Serial.begin(9600);
  //For I2C, enable the following and disable the SPI section
  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = 0x77;
  mySensor.settings.runMode = 3; //Normal mode
  mySensor.settings.tStandby = 0;
  mySensor.settings.filter = 0;
  mySensor.settings.tempOverSample = 1;
  mySensor.settings.pressOverSample = 1;
  mySensor.settings.humidOverSample = 1;
  Serial.print("Starting BME280... result of .begin(): 0x");
  
  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  Serial.println(mySensor.begin(), HEX);
  
  if (fram.begin()) {  // you can stick the new i2c addr in here, e.g. begin(0x51);
    Serial.println("Found I2C FRAM");
  } else {
    Serial.println("No I2C FRAM found ... check your connections\r\n");
    while (1);
  }

// FRAM_reset();
  
  // Read the first byte
  FRAM_storage.last_pos = (fram.read8(0x00)<<8)| fram.read8(0x01);
  FRAM_storage.start_pos  = (fram.read8(0x02)<<8)| fram.read8(0x03);

  // format an "empty disk"
  if (FRAM_storage.last_pos<4) {
      FRAM_reset();
  }
  
   Serial.print("last_pos: 0x"); Serial.println(FRAM_storage.last_pos, HEX);
   Serial.print("start_pos:  0x"); Serial.println(FRAM_storage.start_pos, HEX);
   Serial.print("will store values any ");
   Serial.print(WAIT_MINUTES);
   Serial.println(" Minutes.");
};
/** resets FRAM structure
 *  put pointer in FRAM to start position 
 */
void FRAM_reset()  {
      FRAM_storage.last_pos = 4;
      FRAM_storage.start_pos  = 4;
      fram.write8(0x00, (FRAM_storage.last_pos&0xff00)>>8);
      fram.write8(0x01, (FRAM_storage.last_pos&0x00ff));
      fram.write8(0x02, (FRAM_storage.start_pos&0xff00)>>8);
      fram.write8(0x03, (FRAM_storage.start_pos&0x00ff));
      Serial.println("\nFRAM Disk initialised!");
}
void FRAM_check(int n, int m) {
  if(FRAM_storage.last_pos+(n*m)>FRAM_SIZE) {
    FRAM_storage.start_pos = 0;    
    fram.write8(0x02, (FRAM_storage.start_pos&0xff00)>>8);
    fram.write8(0x03, (FRAM_storage.start_pos&0x00ff));
    FRAM_storage.last_pos = 4;
    fram.write8(0x00, (FRAM_storage.last_pos&0xff00)>>8);
    fram.write8(0x01, (FRAM_storage.last_pos&0x00ff));
  }
}
/** appends one byte to FRAM Memory structure
 *  
 */
int FRAM_append(int a) {
  uint16_t pos;
  if (FRAM_storage.last_pos+2<FRAM_SIZE) {
    fram.write8(FRAM_storage.last_pos,(a&0xff00)>>8);
    fram.write8(FRAM_storage.last_pos+1,(a&0x00ff));
    FRAM_storage.last_pos += 2;
    fram.write8(0x00, (FRAM_storage.last_pos&0xff00)>>8);
    fram.write8(0x01, (FRAM_storage.last_pos&0x00ff));
    return(2);
  } else {
    FRAM_storage.start_pos = 0;    
    fram.write8(0x02, (FRAM_storage.start_pos&0xff00)>>8);
    fram.write8(0x03, (FRAM_storage.start_pos&0x00ff));
    FRAM_storage.last_pos = 4;
    fram.write8(0x00, (FRAM_storage.last_pos&0xff00)>>8);
    fram.write8(0x01, (FRAM_storage.last_pos&0x00ff));
    return(0);
  };
};
/** appends a long integer (4 bytes) to the FRAM memeory
 *  put 4 bytes, high bytes first into FRAM
 */
int FRAM_append4(uint32_t a) {
  int flag=0;
  uint16_t pos;
  if (FRAM_storage.last_pos+4>FRAM_SIZE) {
    FRAM_storage.last_pos = 4;
    FRAM_storage.start_pos = 0;
    flag=1;
  };
  fram.write8(FRAM_storage.last_pos,  (a&0xff000000)>>24);
  fram.write8(FRAM_storage.last_pos+1,(a&0x00ff0000)>>16);
  fram.write8(FRAM_storage.last_pos+2,(a&0x0000ff00)>>8);
  fram.write8(FRAM_storage.last_pos+3,(a&0x000000ff));
  FRAM_storage.last_pos += 4;    
  fram.write8(0x00, (FRAM_storage.last_pos&0xff00)>>8);
  fram.write8(0x01, (FRAM_storage.last_pos&0x00ff));
  fram.write8(0x02, (FRAM_storage.start_pos&0xff00)>>8);
  fram.write8(0x03, (FRAM_storage.start_pos&0x00ff));
  return(flag);
};

void FRAM_dump_memory(void) {
  uint8_t value;
  for (uint16_t a=4; a<FRAM_storage.last_pos; a++) {
    value = fram.read8(a);
    if ((a % 32) == 0) {
      Serial.print("\n 0x"); Serial.print(a, HEX); Serial.print(": ");
    }
    Serial.print("0x"); 
    if (value < 0x10) 
      Serial.print('0');
    Serial.print(value, HEX); Serial.print(" ");
  }
  Serial.print("\n");
}
/**
 * dump values as integers, 
 * readyto plot with the ARDUINO serial plotter
 */
void FRAM_dump_integer(int n) {
  union { 
    uint32_t value;
    long int in;
  } bytes;
  
  for (uint16_t a=4; a<FRAM_storage.last_pos; a+=4*n) {
    for (int i=0; i<n; i++) {
      bytes.value = fram.read8(4*i+a)<<24;
      bytes.value |= fram.read8(4*i+a+1)<<16;
      bytes.value |= fram.read8(4*i+a+2)<<8;
      bytes.value |= fram.read8(4*i+a+3);
      if(i==0) {
        Serial.print(bytes.in); Serial.print(" ");
      }
    };
    Serial.println(" ");
  };
}

void loop(void) {
  int i;
  float v;

//  
//  FRAM_dump();
  //Each loop, take a reading.
  //Start with temperature, as that data is needed for accurate compensation.
  //Reading the temperature updates the compensators of the other functions
  //in the background.
  FRAM_check(3,4);
  v=mySensor.readTempC();
  Serial.print("\nTemperature: ");
  Serial.print(v, 2);
  Serial.print(" degrees C; ");
  FRAM_append4(v*100); //proper scaling 2 decimals in C
  
  v=mySensor.readFloatPressure();
  Serial.print("Pressure: ");
  Serial.print(v,2);
  Serial.print(" Pa; ");
  FRAM_append4(v/10); //proper scaling 2 decimals in mBar

  v=mySensor.readFloatHumidity();
  Serial.print("%RH: ");
  Serial.print(v, 2);
  Serial.println(" %");
  FRAM_append4(v*100); //proper scaling 2 decimals in % RH
  
//  FRAM_dump_integer(3);

// store a value set, every x Minutes
  for (i=0; i<WAIT_MINUTES; i++) {
    delay(30000); delay(30000); // one Minute delay
  }
}
