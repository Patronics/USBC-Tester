


////Shift register pins
#define shiftSRCLK 18
#define shiftRCLK 15
#define shiftSer 19
#define shiftOE 20


//misc pin definitions
#define internalLED 25

void setup() {
  // put your setup code here, to run once:
  pinMode(internalLED, OUTPUT);
  pinMode(shiftSRCLK, OUTPUT);
  pinMode(shiftRCLK, OUTPUT);
  pinMode(shiftSer, OUTPUT);
  pinMode(shiftOE, OUTPUT);
  digitalWrite(shiftOE, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(internalLED, !digitalRead(internalLED));
  digitalWrite(shiftSer,!digitalRead(shiftSer));
  delay(50);
  digitalWrite(shiftSRCLK,HIGH);
  delay(50);
  digitalWrite(shiftRCLK,HIGH);
  delay(900);
  digitalWrite(shiftSRCLK,LOW);
  digitalWrite(shiftRCLK,LOW);

  
}
