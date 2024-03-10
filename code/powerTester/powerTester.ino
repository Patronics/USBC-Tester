


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
  for(int i=1; i<pow(2,28); i=i*2){
    sendBits(i, 0, true, false);
    delay(loopDelay);
  }

}

//LEDBits = first 29 bits to shift out (MSB 3 bits unused)
//usbBits = remaining 19 values to shift out (important to note that the 3 LSB WILL be set if ledEnable is true even if usbEnable is false)
void sendBits(long ledBits, long usbBits, bool ledEnable, bool usbEnable){
  digitalWrite(shiftOEUSB,HIGH);
  digitalWrite(shiftOELEDs,HIGH);
  for(int i=18; i>=3; i--){   //shift out most of USB
    shiftOut(usbBits, i);
  }
  for(int i=28; i>=24; i--){   //shift out last few LEDs
    shiftOut(ledBits, i);
  }
  for(int i=2; i>=0; i--){   //shift out last few USB
    shiftOut(usbBits, i);
  }
  for(int i=23; i>=0; i--){  //shift out most of LEDs
    shiftOut(ledBits, i);
  }


  digitalWrite(shiftOEUSB,!usbEnable); //OE is active low, so invert
  digitalWrite(shiftOELEDs,!ledEnable);
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

