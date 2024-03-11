//USB C Power tester
//Patrick Leiser, March 2024


#include <AP33772.h>


//if U1 and U2 are unpopulated, green-wire U1 pin 14 to U2 pin 9 and define USB_SHIFT_REG_BYPASS
#define USB_SHIFT_REG_BYPASS




////Shift register pins
#define shiftSRCLK 18
#define shiftRCLK 15
#define shiftSer 19
#define shiftOELEDs 20
#define shiftOEUSB 21

////USB C testing pins
#define USBinVBUS 2
#define USBinGND 6
#define USBinDP 9
#define USBinDN 10

#define USBinCC1 32  //ADC CAPABLE
#define USBinCC2 31  //ADC CAPABLE
#define USBinSBU1 11
#define USBinSBU2 3

#define USBinSSTXP1 7
#define USBinSSTXN1 8
#define USBinSSRXP1 5
#define USBinSSRXN1 4
#define USBinSSTXP2 14
#define USBinSSTXN2 17
#define USBinSSRXP2 13
#define USBinSSRXN2 12

#define USBinSHIELD 22


////misc I/O pins
#define pushButton 22
#define ADCPot 28

////picoPD pin definitions
#define internalLED 25
#define VBUSSwitch 23

AP33772 usbpd; // Automatically wire to Wire0

//global to persist voltages detected
long ledState = 0x00000F00;


void setup() {
  // put your setup code here, to run once:
  pinMode(internalLED, OUTPUT);
  pinMode(shiftSRCLK, OUTPUT);
  pinMode(shiftRCLK, OUTPUT);
  pinMode(shiftSer, OUTPUT);
  pinMode(shiftOELEDs, OUTPUT);
  pinMode(VBUSSwitch, OUTPUT);
  digitalWrite(shiftOELEDs, HIGH);
  digitalWrite(shiftOEUSB, HIGH);
  digitalWrite(VBUSSwitch, LOW);  //disable high-voltage VBUS output
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Serial.begin(115200);
  scanLeds(50); //test all LEDs and give AP33772 time to initialize
  usbpd.begin();
  usbpd.printPDO();
  ledState = ledState | scanVoltages();
}

void loop() {
  // put your main code here, to run repeatedly:
  ledState = ledState & 0x000000FF;  //persist power state, rescan other inputs

  sendBits(ledState,0,true,false);
  delay(5); //give the LEDs time to shine before being toggled off again to update
}

byte scanVoltages(){
  
  byte voltageLEDs = 0x00;
  int maxCurrent = usbpd.getMaxCurrent();
  if (maxCurrent >= 2850){
    voltageLEDs = voltageLEDs | 0x40;
  }
  if (maxCurrent >= 4750){
    voltageLEDs = voltageLEDs | 0x80;
  }
  if (checkForSupportedVoltageInToleranceRange(5000,0x01,4750,5500)){  //max voltage is 5.5 to comply with USB 2.0 spec
    voltageLEDs = voltageLEDs | 0x01;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  if(checkForSupportedVoltage(9000,0x02)){
    voltageLEDs = voltageLEDs | 0x02;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  if(checkForSupportedVoltage(12000, 0x04)){
    voltageLEDs = voltageLEDs | 0x04;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  if(checkForSupportedVoltage(15000,0x08)){
    voltageLEDs = voltageLEDs | 0x08;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  if(checkForSupportedVoltage(20000,0x10)){
    voltageLEDs = voltageLEDs | 0x10;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }

  usbpd.setVoltage(5000); //restore default 5v voltage

  return voltageLEDs;
}

//check if voltage is supported within +/- 5% tolerance
bool checkForSupportedVoltage(int targetmV,long ledsToFlash){
  return checkForSupportedVoltageInToleranceRange(targetmV,ledsToFlash,targetmV*0.95,targetmV*1.05);
}

bool checkForSupportedVoltageInToleranceRange(int targetmV,long ledsToFlash, int minRange, int maxRange){
  digitalWrite(VBUSSwitch, LOW); //make sure voltage is not output to VBUS
  usbpd.setVoltage(targetmV);
  for(int i=0; i<5; i++){
    sendBits(ledState | ledsToFlash,0,true, false);
    delay(25);
    sendBits(ledState,0,true,false);
    delay(25);
  }
  delay(275);   //maximum potential delay
  int measuredVoltage = usbpd.readVoltage();
  if (measuredVoltage>minRange && measuredVoltage<maxRange){  //comply with PD spec tolerance
    return true;
  }
  return false;

}

//do a scan to demonstrate that all LEDs are functional
void scanLeds(int loopDelay){
  digitalWrite(internalLED, !digitalRead(internalLED));
  for(int i=1; i<pow(2,29); i=i*2){
    sendBits(i, 0, true, false);
    delay(loopDelay);
  }

}

/*ledBits - software mapping:
0x000000FF > power
0x00000F00 > usb 2
0x000FF000 > usb 3
0x00F00000 > other
0x1F000000 > status
0xE0000000 > unused (ignored)

usbBits = remaining 19 values to shift out (important to note that the 3 LSB WILL be set if ledEnable is true even if usbEnable is false)
*/
void sendBits(long ledBits, long usbBits, bool ledEnable, bool usbEnable){
  digitalWrite(shiftOEUSB,HIGH);
  digitalWrite(shiftOELEDs,HIGH);
  ledBits = remapLeds(ledBits);

  for(int i=24; i<29; i++){   //shift out last few LEDs
    shiftOutBit(ledBits, i);
  }
#ifdef USB_SHIFT_REG_BYPASS
    for(int i=0; i<3; i++){   //shift out USB's 3 LSB only
    shiftOutBit(usbBits, i);
  }
#else
  for(int i=0; i<19; i++){   //shift out USB
    shiftOutBit(usbBits, i);
  }
#endif
  for(int i=0; i<24; i++){  //shift out most of LEDs
    shiftOutBit(ledBits, i);
  }


  digitalWrite(shiftOEUSB,!usbEnable); //OE is active low, so invert
  digitalWrite(shiftOELEDs,!ledEnable);
}

/*
software mapping:
0x000000FF > power
0x00000F00 > usb 2
0x000FF000 > usb 3
0x00F00000 > other
0x1F000000 > status

hardware mapping:
0x00FF0000 > power
0x0000F000 > usb 2
0x00000F87 > usb 3
0x00000F00 > usb 3 - bit 0-3
0x00000080 > usb 3 - bit 4
0x00000007 > usb 3 - bit 5-7
0x00000078 > other
0x1F000000 > status
*/

//remap bit order of LEDs to simplify understanding
long remapLeds(long cleanMapping){
  
  long remapped = ((cleanMapping &   0xFF) << 16); //Power
  remapped += ((cleanMapping &     0x0F00) << 4);  //usb 2
  remapped += ((cleanMapping &    0x0F000) >> 4);  // usb 3 bit 0-3
  remapped += ((cleanMapping &    0x10000) >> 9);  // usb 3 bit 4
  remapped += ((cleanMapping &    0xFE0000) >> 17); // other and usb 3 bit 5-7
  remapped += (cleanMapping & 0x1F000000);         //status
  return remapped;
}

void shiftOutBit(long word, int chosenBit){
  digitalWrite(shiftSRCLK, LOW);
  digitalWrite(shiftRCLK,LOW);
  digitalWrite(shiftSer, bitRead(word, chosenBit));
  //delayMicroseconds(1); //technically only 0.2uS delay needed
  digitalWrite(shiftSRCLK, HIGH);
  //delayMicroseconds(1);  //unnecessary except for final shift, but harmless.
  digitalWrite(shiftRCLK, HIGH);
  //delayMicroseconds(1);
}

