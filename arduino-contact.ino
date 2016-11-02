/** 
1. Handshake PC
2. Wait for a card to block the light source
3. Tell the computer to perform OCR
4. Wait for the PC to reply
5. Activate motor (LED in initial implementation)
6. Wait for lighting to be restored
7. Deactivate motor (LED in initial implementation)
8. Go back to #2
**/

int exceptionPin = 13;
int motorPin     = 3;
int dividerPin   = 14;

float arduinoVoltage = 5.0;

float baseResistance = 2.35e3;
float transistorBeta = 3.6e2;
float vbe            = 0.68;

float currentMotorOn             = (arduinoVoltage-vbe) / baseResistance * transistorBeta;
float desiredAverageMotorCurrent = 0.15;

float dividerSupplyVoltage = 5.0;
float Rd = 4.6e4; //fixed resistance in the photocell voltage divider

float photocellVoltageFromResistance(float photocellResistance) {
  return photocellResistance / (photocellResistance + Rd) * dividerSupplyVoltage;
}

float photocellResistanceLightThreshold = 5.6e3 * 2.0; //typical resistance measured when tray is exposed to light (times 2)
float photocellResistanceDarkThreshold  = 4.0e4 / 2.0; //typical resistance measured when tray is covered (divided by 2)

float dividerVoltageLightThreshold = photocellVoltageFromResistance(photocellResistanceLightThreshold);
float dividerVoltageDarkThreshold  = photocellVoltageFromResistance(photocellResistanceDarkThreshold);

int timeInLightThreshold = 1000; //time in millis
int timeInDarkThreshold  = 1000; //time in millis

int sampleDelay = 5;

void setup() {
  Serial.begin(9600);
  //comment out the next two lines to test motor control and sensing only
  //startNotification();
  //waitForStartReply();
  
  pinMode(exceptionPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
}

void loop() {
  waitUntilConsistentlyLight(); //this is to prevent the camera from triggering in an initially-unlit room

  while(true) {
    waitUntilConsistentlyDark();
    //comment out the next two lines to test motor control and sensing only
    //requestPhoto();
    //waitForPhotoResponse();
    activateMotor(); //for now, just activate unidirectional motor
    waitUntilConsistentlyLight();
    deactivateMotor(); //for now, just deactivate unidirectional motor
  }
}

bool isLight() {return fromAdcToVoltage(analogRead(dividerPin)) < dividerVoltageLightThreshold;}
bool isDark()  {return fromAdcToVoltage(analogRead(dividerPin)) > dividerVoltageDarkThreshold;}

float fromAdcToVoltage(int pinReading) { return (float)pinReading * arduinoVoltage / 1023.0; }
float dutyCycleToPwm(float dutyCycle)  { return (int)(dutyCycle * 255); }

void activateMotor()   { analogWrite(motorPin,dutyCycleToPwm(desiredAverageMotorCurrent/currentMotorOn)); }
void deactivateMotor() { analogWrite(motorPin,0); }

void waitUntilConsistentlyDark(void) {
  int timeInDark = 0;
  int initialTimeInDark;

  while(timeInDark<timeInDarkThreshold) {
    delay(sampleDelay);
    
    if(isDark()) {
      if(timeInDark==0) {
        initialTimeInDark = millis();
      }
      timeInDark = millis()-initialTimeInDark;
    } else {
      timeInDark = 0;
    }
  }
}

void waitUntilConsistentlyLight(void) {
  int timeInLight = 0;
  int initialTimeInLight;
  
  while(timeInLight<timeInLightThreshold) {
    delay(sampleDelay);
    
    if(isLight()) {
      if(timeInLight==0) {
        initialTimeInLight = millis();
      }
      timeInLight = millis()-initialTimeInLight;
    } else {
      timeInLight = 0;
    }
  }
}

void startNotification(void) {
  String startNotification = "Arduino ready";
  
  Serial.println(startNotification);
  Serial.flush();
}

void waitForStartReply(void) {
  String replyString;

  while(1) {  
    replyString = "";
    
    while(!Serial.available()) {}
    while(Serial.available()) {
      delay(30);
      if(Serial.available()>0) {
        char c = Serial.read();
        replyString += c;
      }
    }
    
    if(replyString.equals("PC ready")) {
      return;
    }
  }
}

void requestPhoto() { //serial write
  String photoRequest = "Say cheese";
  
  Serial.println(photoRequest);
  Serial.flush();
}

void waitForPhotoResponse() { //serial read
  String photoReplyString;

  while(1) {
    photoReplyString = "";
    
    while(!Serial.available()) {}
    while(Serial.available()) {
      delay(30);
      if(Serial.available()>0) {
        char c = Serial.read();
        photoReplyString += c;
      }
    }
  }
  
  if(photoReplyString.equals("Cheese")) {
    return;
  }
}
