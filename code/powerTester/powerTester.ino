//USB C Power tester
//Patrick Leiser, March 2024

//if U1 and U2 are unpopulated, green-wire U1 pin 14 to U2 pin 9 and define USB_SHIFT_REG_BYPASS
#define USB_SHIFT_REG_BYPASS



////Shift register pins
#define shiftSRCLK 18
#define shiftRCLK 15
#define shiftSer 19
#define shiftOELEDs 20
#define shiftOEUSB 21

//misc pin definitions
#define internalLED 25

void setup() {
  // put your setup code here, to run once:
  pinMode(internalLED, OUTPUT);
  pinMode(shiftSRCLK, OUTPUT);
  pinMode(shiftRCLK, OUTPUT);
  pinMode(shiftSer, OUTPUT);
  pinMode(shiftOELEDs, OUTPUT);
  digitalWrite(shiftOELEDs, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  scanLeds(100);
  
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
    shiftOut(ledBits, i);
  }
#ifdef USB_SHIFT_REG_BYPASS
    for(int i=0; i<3; i++){   //shift out USB's 3 LSB only
    shiftOut(usbBits, i);
  }
#else
  for(int i=0; i<19; i++){   //shift out USB
    shiftOut(usbBits, i);
  }
#endif
  for(int i=0; i<24; i++){  //shift out most of LEDs
    shiftOut(ledBits, i);
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

void shiftOut(long word, int chosenBit){
  digitalWrite(shiftSRCLK, LOW);
  digitalWrite(shiftRCLK,LOW);
  digitalWrite(shiftSer, bitRead(word, chosenBit));
  delayMicroseconds(3); //technically only 0.2uS delay needed
  digitalWrite(shiftSRCLK, HIGH);
  delayMicroseconds(3);  //unnecessary except for final shift, but harmless.
  digitalWrite(shiftRCLK, HIGH);
  delayMicroseconds(3);
}

