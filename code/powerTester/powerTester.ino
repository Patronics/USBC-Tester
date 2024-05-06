//USB C Power tester
//Patrick Leiser, March 2024


#include <AP33772.h>


#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define VERSION_PATCH 3

//generate a pseudo-random unique hash from semver to display installed software version on leds
#define VERSION_HASH ((VERSION_MAJOR * 5823 + (VERSION_MINOR)%4096)*1409%4096+VERSION_PATCH%4096)



//if U1 and U2 are unpopulated, green-wire U1 pin 14 to U2 pin 9 and define USB_SHIFT_REG_BYPASS
//#define USB_SHIFT_REG_BYPASS


/*
Hardware version Errata:
ERRATABOARD = 0: perfect fully functional board, revision 1.1
ERRATABOARD = 1: test ports and U1, U2 unpopulated, red leds 15x brightness
ERRATABOARD = 2 DAMAGED, test port detached: Inputs SBU2 and DP1 shorted together; outputs SSTXP2 and SSTXPN2 shorted together; outputs VBUS and CC1 shorted together
ERRATABOARD = 3: Outputs DP2 and CC2 and SSTXN2 and SBU1 and SBY2 and SSRXN1 and SSRXP1 shorted together; Inputs DP and DN and SBU2 shorted together
ERRATABOARD = 4: Assembled with the open-collector shift registers
ERRATABOARD = 5: Fully functional Rev 1.0 board
*/

#define ERRATABOARD 0


#if ERRATABOARD == 0  
  #define ERRATA_SET
  #define BOARD_REVISION 1.1
#endif

#if ERRATABOARD == 1
  #define ERRATA_SET
  #define USB_SHIFT_REG_BYPASS
  #define BRIGHTLEDDIMFACTOR 15
  #define NOPINSTEST
  #define BOARD_REVISION 1.0
#endif


#if ERRATABOARD == 2
  #define ERRATA_SET
  #define NOPINSTEST
  #define BOARD_REVISION 1.0
#endif


#if ERRATABOARD == 3
  #define ERRATA_SET
  //optional TODO HANDLE testing some pins
  #define NOPINSTEST
  #define BOARD_REVISION 1.0
#endif

#if ERRATABOARD == 4
  #define ERRATA_SET
  //optional TODO HANDLE testing some pins
  //#define NOPINSTEST
  #define PULL_UP_INPUTS
  #define BOARD_REVISION 1.0
#endif

#if ERRATABOARD == 5
  #define ERRATA_SET
  #define BOARD_REVISION 1.0
#endif

#ifndef ERRATABOARD
  #warning "Warning, no board errata defined"
#endif

#ifdef PULL_UP_INPUTS
  #define USB_TEST_INPUT_TYPE INPUT_PULLUP
#else
  #define USB_TEST_INPUT_TYPE INPUT
#endif

////Shift register pins
#define shiftSRCLK 18
#define shiftRCLK 15
#define shiftSer 19
//OE pins are active low
#define shiftOELEDs 20
#define shiftOEUSB 21

////USB C testing pins - GPIO inputs
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

#define USBinSHIELD 16

////USB C testing pins - Shift register outputs

#define USBoutVBUS   0x100000
//use USBoutGND with caution, as it's tied to the USBoutSHIELD pin by most cables
#define USBoutGND    0x080000
#define USBoutDP1    0x000800
#define USBoutDP2    0x000008
#define USBoutDN1    0x000400
#define USBoutDN2    0x040000

#define USBoutSSTXP1 0x004000
#define USBoutSSTXN1 0x002000
#define USBoutSSRXP1 0x008000
#define USBoutSSRXN1 0x010000
#define USBoutSSTXP2 0x000040
#define USBoutSSTXN2 0x000020
#define USBoutSSRXP2 0x000080
#define USBoutSSRXN2 0x000100


#define USBoutSBU1   0x000200
#define USBoutSBU2   0x020000

//this group pulled up to VCC by 36k by default when USB outputs disabled
#define USBoutCC1    0x001000
#define USBoutCC2    0x000010

//this group tied to the LED output enable line, NOT USB output enable
#define USBoutSHIELD 0x000004
#define USBpullupCC1 0x000002
#define USBpullupCC2 0x000001


/*LED Outputs:
software mapping:
0x000000FF > power (voltages and PPS, 3A, 5A)
0x00000F00 > usb 2
0x000FF000 > usb 3
0x00F00000 > other
0x1F000000 > status
0x60000000 > power (1A, 2A)
0x80000000 > unused
*/

#define LEDNONE 0x00
#define BRIGHTLEDMASK 0x1A000000   //to software-pwm the red LEDs to a more subtle level
//#define BRIGHTLEDMASK 0x1FFFFFFF   //testing pwm dimming of green leds too

//#define BRIGHTLEDDIMFACTOR 15 //how much to reduce LED intensity, or comment out to disable software dimming

#define LED5V 0x01
#define LED9V 0x02
#define LED12V 0x04
#define LED15V 0x08
#define LED20V 0x10
#define LEDPPS 0x20
#define LED1A 0x20000000
#define LED2A 0x40000000
#define LED3A 0x40
#define LED5A 0x80

#define AllVoltageLEDs (LED5V | LED9V | LED12V | LED15V | LED20V)
#define AllCurrentLEDs (LED5A | LED3A | LED2A | LED1A)
#define AllPowerLEDs (AllVoltageLEDs | AllCurrentLEDs | LEDPPS)

#define LEDGND     0x100
#define LEDVBUS    0x200
#define LEDDN  0x400
#define LEDDP   0x800

#define LEDSSTXP1  0x01000
#define LEDSSTXN1  0x02000
#define LEDSSRXP1  0x04000
#define LEDSSRXN1  0x08000
#define LEDSSTXP2  0x10000
#define LEDSSTXN2  0x20000
#define LEDSSRXP2  0x40000
#define LEDSSRXN2  0x80000

#define LEDCC1     0x00100000
#define LEDCC2     0x00200000
#define LEDSBU1    0x00400000
#define LEDSBU2    0x00800000

#define LEDSHIELD  0x01000000
#define LEDFLIPPED 0x02000000
#define LEDEMARK   0x04000000
#define LEDABSENT  0x08000000
#define LEDCROSSED 0x10000000

////misc I/O pins
#define pushButton 22
#define ADCPot 28

////picoPD pin definitions
#define internalLED 25
#define VBUSSwitch 23

AP33772 usbpd; // Automatically wire to Wire0

//global to persist voltages detected
unsigned long ledState = 0x00000F00;

//count when to enable pulses of bright leds for PWM
int brightLedPulseCounter = 0;


void setup() {
  // put your setup code here, to run once:
  pinMode(internalLED, OUTPUT);
  pinMode(shiftSRCLK, OUTPUT);
  pinMode(shiftRCLK, OUTPUT);
  pinMode(shiftSer, OUTPUT);
  pinMode(shiftOELEDs, OUTPUT);
  pinMode(VBUSSwitch, OUTPUT);



  pinMode(USBinSHIELD, USB_TEST_INPUT_TYPE);
  pinMode(USBinSBU1, USB_TEST_INPUT_TYPE);
  pinMode(USBinSBU2, USB_TEST_INPUT_TYPE);


  digitalWrite(shiftOELEDs, HIGH);
  digitalWrite(shiftOEUSB, HIGH);
  digitalWrite(VBUSSwitch, LOW);  //disable high-voltage VBUS output
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Serial.begin(115200);
  scanLeds(50); //test all LEDs and give AP33772 time to initialize
  usbpd.begin();
  displayVersionAndHash();
  delay(500);
  usbpd.printPDO();
  //scanLeds(150);
  ledState = ledState | scanVoltages();
}

void loop() {
  //scanLeds(150);
  //ledState = 0x1FFFFFFF;
  //sendBits(ledState,0,true,false);
  // put your main code here, to run repeatedly:
  ledState = ledState & AllPowerLEDs;  //persist power state, rescan other inputs
  if(!checkAllPins()){
    ledState = ledState | LEDABSENT;
  }

  #ifdef BRIGHTLEDDIMFACTOR //only run this logic if dimming enabled
  brightLedPulseCounter++;
  if (brightLedPulseCounter > BRIGHTLEDDIMFACTOR){
    brightLedPulseCounter = 0;
  } else {
    ledState = ledState & ~BRIGHTLEDMASK;
  }
  #endif
  sendBits(ledState,0,true,false);
  delay(1); //give the LEDs time to shine before being toggled off again to update

}

//show the version currently installed on the LEDs
void displayVersionAndHash(){
  long versionLedSequence = 0;
  versionLedSequence = 0;
  versionLedSequence = versionLedSequence | VERSION_MINOR%16 * 0x100; //set usb2 leds to minor version
  versionLedSequence = versionLedSequence | VERSION_PATCH%32 * 0x1000000;  //set status leds to patch version
  //set current leds for major version
  versionLedSequence = versionLedSequence | VERSION_MAJOR%32/16 * LEDPPS;
  versionLedSequence = versionLedSequence | VERSION_MAJOR%16/4 * LED1A;
  versionLedSequence = versionLedSequence | VERSION_MAJOR%4 * LED3A;
  versionLedSequence = versionLedSequence | VERSION_HASH * 0x1000;    //set USB3 and other lights to version hash
  sendBits(versionLedSequence, 0, true, false);
}

//check all pins, update LEDs, and return true if any pins are found connected
bool checkAllPins(){
  bool cf = false;   //Connection Found flag
  cf = cf | checkPinConnectionFast(USBoutVBUS, USBinVBUS, LEDVBUS);
  cf = cf | checkPinConnectionFast(USBoutGND|USBoutSHIELD, USBinGND, LEDGND);
  cf = cf | checkPinConnectionFast(USBoutGND|USBoutSHIELD, USBinSHIELD, LEDSHIELD);
  cf = cf | checkPinConnectionFast(USBoutDP1|USBoutDP2, USBinDP, LEDDP);
  cf = cf | checkPinConnectionFast(USBoutDN1|USBoutDN2, USBinDN, LEDDN);

  cf = cf | checkPinConnectionFast(USBoutDP1, USBinDP, LEDFLIPPED);
  
  cf = cf | checkPinConnectionFast(USBoutSSTXP1|USBoutSSTXP2, USBinSSRXP1, LEDSSRXP1);
  cf = cf | checkPinConnectionFast(USBoutSSTXN1|USBoutSSTXN2, USBinSSRXN1, LEDSSRXN1);
  cf = cf | checkPinConnectionFast(USBoutSSRXP1|USBoutSSRXP2, USBinSSTXP1, LEDSSTXP1);
  cf = cf | checkPinConnectionFast(USBoutSSRXN1|USBoutSSRXN2, USBinSSTXN1, LEDSSTXN1);
  cf = cf | checkPinConnectionFast(USBoutSSTXP2|USBoutSSTXP1, USBinSSRXP2, LEDSSRXP2);
  cf = cf | checkPinConnectionFast(USBoutSSTXN2|USBoutSSTXN1, USBinSSRXN2, LEDSSRXN2);
  cf = cf | checkPinConnectionFast(USBoutSSRXP2|USBoutSSRXP1, USBinSSTXP2, LEDSSTXP2);
  cf = cf | checkPinConnectionFast(USBoutSSRXN2|USBoutSSRXN1, USBinSSTXN2, LEDSSTXN2);

//CC1/CC2 handling more compelex than this, not passed through
//  cf = cf | checkPinConnectionFast(USBoutCC1|USBoutCC2, USBinCC1, LEDCC1);
//  cf = cf | checkPinConnectionFast(USBoutCC1|USBoutCC2, USBinCC2, LEDCC2);

  cf = cf | checkPinConnectionFast(USBoutSBU1|USBoutSBU2, USBinSBU1, LEDSBU1);
  cf = cf | checkPinConnectionFast(USBoutSBU2|USBoutSBU1, USBinSBU2, LEDSBU2);
  return cf;

}

//check continuity of a specified pin
bool checkPinConnectionFast(unsigned long outPin,int inPin,unsigned long ledPin){
  return checkPinConnectionFastWithUSBEnablePinSetAs(outPin, inPin, ledPin, true);
}

//helper function for checkPinConnectionFast, use with usbEnable true except when testing shield pin
bool checkPinConnectionFastWithUSBEnablePinSetAs(unsigned long outPin,int inPin,unsigned long ledPin, bool usbEnable){
  sendBits(ledState, 0, true, usbEnable); //test low first, and flash the led corresponding to pin under test
  delayMicroseconds(20);   //give time for bits to settle and LED to flash
  if (digitalRead(inPin)){
    Serial.println("test outPin "+String(outPin)+" connection to inPin "+String(inPin)+" failed, expected LOW, got HIGH");
    sendBits(ledState, 0, true, false); //restore LEDs and disable output
    return false;
  }
  sendBits(ledState, outPin, true, usbEnable); //test low first, and flash the led corresponding to pin under test
  delayMicroseconds(20);   //give time for bits to settle and LED to flash
  if (!digitalRead(inPin)){
    Serial.println("test outPin "+String(outPin)+"connection to inPin"+String(inPin)+"failed, expected HIGH, got LOW");
    sendBits(ledState, 0, true, false); //restore LEDs and disable output
    return false;
  }
  ledState = ledState | ledPin;
  sendBits(ledState, 0, true, false);
  return true;
}

bool checkShieldConnection(){
  if (checkPinConnectionFastWithUSBEnablePinSetAs(USBoutSHIELD, USBinSHIELD, LEDNONE, false)){
  //shield should be tied to ground, so only test driving it low
  //sendBits(ledState, 0, true, true); //test low first, and flash the led corresponding to pin under test
  //delayMicroseconds(20);   //give time for bits to settle and LED to flash
  //if (digitalRead(USBinSHIELD)){
    Serial.println("test outPin "+String(USBoutSHIELD)+" connection to inPin "+String(USBinSHIELD)+" failed, expected LOW, got HIGH");
    sendBits(ledState, 0, true, false); //restore LEDs and disable output
    return false;
  }

  ledState = ledState | LEDSHIELD;
  return true;
}

//check continuity of a specified pin, AND that no other pins are shorted to it
bool checkPinConnectionFull(unsigned long outPin, int inPin, unsigned long ledPin){
  //stub function, unimplemented
  return checkPinConnectionFast(outPin, inPin, ledPin);
}

bool checkPPS(){
  if(usbpd.getExistPPS()){
    return true;
  }
  return false;
}

//specify target voltage in millivolts, or -1 for overall maximum
unsigned int getCurrentLEDsForVoltage(int targetVoltage){
  unsigned int currentLEDs = 0;
  int maxCurrent = 0;
  if (targetVoltage == -1){ //get overall MaxCurrent
    maxCurrent = usbpd.getMaxCurrent();
  } else {
    maxCurrent = usbpd.getMaxCurrentForVoltage(targetVoltage);
  }
  Serial.println("got current "+String(maxCurrent)+" for voltage "+String(targetVoltage));
  if (maxCurrent >= 950){
    currentLEDs = currentLEDs | LED1A;
  }
  if (maxCurrent >= 1850){
    currentLEDs = currentLEDs | LED2A;
  }
  if (maxCurrent >= 2850){
    currentLEDs = currentLEDs | LED3A;
  }
  if (maxCurrent >= 4750){
    currentLEDs = currentLEDs | LED5A;
  }
  return currentLEDs;
}


unsigned long scanVoltages(){
  
  unsigned long voltageLEDs = 0x00;
  voltageLEDs = voltageLEDs | (LEDPPS * checkPPS());
  
  ledState = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(5000);
  sendBits(ledState,0,true,false);
  if (checkForSupportedVoltageInToleranceRange(5000,LED5V,4750,5500)){  //max voltage is 5.5 to comply with USB 2.0 spec
    voltageLEDs = voltageLEDs | LED5V;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  ledState = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(9000);
  sendBits(ledState,0,true,false);
  if(checkForSupportedVoltage(9000,LED9V)){
    voltageLEDs = voltageLEDs | LED9V;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  ledState = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(12000);
  sendBits(ledState,0,true,false);
  if(checkForSupportedVoltage(12000, LED12V)){
    voltageLEDs = voltageLEDs | LED12V;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  ledState = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(15000);
  sendBits(ledState,0,true,false);
  if(checkForSupportedVoltage(15000,LED15V)){
    voltageLEDs = voltageLEDs | LED15V;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }
  ledState = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(20000);
  sendBits(ledState,0,true,false);
  if(checkForSupportedVoltage(20000,LED20V)){
    voltageLEDs = voltageLEDs | LED20V;
    ledState = ledState | voltageLEDs;
    sendBits(ledState,0,true,false);
  }

  usbpd.setVoltage(5000); //restore default 5v voltage
  voltageLEDs = (voltageLEDs & ~AllCurrentLEDs) | getCurrentLEDsForVoltage(-1);
  delay(500);
  return voltageLEDs;
}

//check if voltage is supported within +/- 5% tolerance
bool checkForSupportedVoltage(int targetmV,unsigned long ledsToFlash){
  return checkForSupportedVoltageInToleranceRange(targetmV,ledsToFlash,targetmV*0.95,targetmV*1.05);
}

bool checkForSupportedVoltageInToleranceRange(int targetmV,unsigned long ledsToFlash, int minRange, int maxRange){
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
  for(unsigned long i=1; i<pow(2,31); i=i*2){
    sendBits(i, 0, true, false);
    delay(loopDelay);
  }

}

//backwards compatible function call, leaving the 4 general purpose outputs unused
void sendBits(unsigned long ledBits, unsigned long usbBits, bool ledEnable, bool usbEnable){
  sendBits(ledBits, usbBits, 0, ledEnable, usbEnable);
}

/*ledBits - software mapping:
0x000000FF > power (original set)
0x00000F00 > usb 2
0x000FF000 > usb 3
0x00F00000 > other
0x1F000000 > status
0x60000000 > power (1A and 2A)
0x80000000 > unused (ignored)

usbBits = remaining 19 values to shift out (important to note that the 3 LSB WILL be set if ledEnable is true even if usbEnable is false)
*/
void sendBits(unsigned long ledBits, unsigned long usbBits, unsigned long GPOBits, bool ledEnable, bool usbEnable){
  digitalWrite(shiftOEUSB,HIGH);
  digitalWrite(shiftOELEDs,HIGH);
  //digitalWrite(shiftOEUSB,LOW); //for testing, simulate no OE pin
  //digitalWrite(shiftOELEDs, LOW);

  ledBits = remapLeds(ledBits);

  for(int i=0; i<4; i++){   //general purpose output bits E,F,G,H
    shiftOutBit(GPOBits, i);
  }
  
  for(int i=29; i<31; i++){  //shift out 1A and 2A power leds
    shiftOutBit(ledBits, i);
  }

  for(int i=19; i<21; i++){  //shift out USB vbus and gnd
    shiftOutBit(usbBits, i);
  }

  for(int i=24; i<29; i++){   //shift out last few LEDs
    shiftOutBit(ledBits, i);
  }
#ifdef USB_SHIFT_REG_BYPASS
    for(int i=0; i<3; i++){   //shift out USB's 3 LSB only (shield pin, and the CC pulldowns/pullups)
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
  digitalWrite(shiftRCLK, HIGH);

  digitalWrite(shiftOEUSB,!usbEnable); //OE is active low, so invert
  digitalWrite(shiftOELEDs,!ledEnable);
}

/*
LED
software mapping:
0x000000FF > power (voltages and PPS, 3A, 5A)
0x00000F00 > usb 2
0x000FF000 > usb 3
0x00F00000 > other
0x1F000000 > status
0x60000000 > power (1A, 2A)

0x80000000 > unused

hardware mapping:
0x00FF0000 > power (voltages and PPS, 3A, 5A)
0x0000F000 > usb 2
0x00000F87 > usb 3
0x00000F00 > usb 3 - bit 0-3
0x00000080 > usb 3 - bit 4
0x00000007 > usb 3 - bit 5-7
0x00000078 > other
0x1F000000 > status
0x60000000 > Power (1A, 2A) //Note that these are on a distinct shift register, not physically adjacent to the others (handled in sendbits)
*/

//remap bit order of LEDs to simplify understanding
unsigned long remapLeds(unsigned long cleanMapping){
  
  unsigned long remapped = ((cleanMapping &   0xFF) << 16); //Power (original set)
  remapped += ((cleanMapping &     0x0F00) << 4);  //usb 2
  remapped += ((cleanMapping &    0x0F000) >> 4);  // usb 3 bit 0-3
  remapped += ((cleanMapping &    0x10000) >> 9);  // usb 3 bit 4
  remapped += ((cleanMapping &    0xFE0000) >> 17); // other and usb 3 bit 5-7
  remapped += (cleanMapping & 0x7F000000);         //status and Power (1A and 2A)
  return remapped;
}

void shiftOutBit(unsigned long word, int chosenBit){
  digitalWrite(shiftSRCLK, LOW);
  digitalWrite(shiftRCLK,LOW);
  digitalWrite(shiftSer, bitRead(word, chosenBit));
  //delayMicroseconds(1); //technically only 0.2uS delay needed
  digitalWrite(shiftSRCLK, HIGH);
  //delayMicroseconds(1);  //unnecessary except for final shift, but harmless.
  //digitalWrite(shiftRCLK, HIGH);
  //delayMicroseconds(1);
}

