/** 
 * 1. Handshake PC
 * 2. Wait for a card to block the light source
 * 3. Tell the computer to perform OCR
 * 4. Wait for the PC to reply
 * 5. Activate motor for a short pulse
 * 6. Wait for lighting to be restored
 * 7. Go back to #2
 **/

int motorEnablePin   = 2;
int motorForwardPin  = 3;
int motorBackwardPin = 4;

int exceptionPin = 13;
int dividerPin   = A0;

float arduinoVoltage = 5.0;

//float baseResistance = 2.35e3;
//float transistorBeta = 3.6e2;
//float vbe            = 0.68;

//float currentMotorOn             = (arduinoVoltage-vbe) / baseResistance * transistorBeta;
//float desiredAverageMotorCurrent = 0.15;

float dividerSupplyVoltage = 5.0;
float Rd = 1.0e5; //fixed resistance in the photocell voltage divider

float photocellVoltageFromResistance(float photocellResistance) {
  return photocellResistance / (photocellResistance + Rd) * dividerSupplyVoltage;
}

float photocellResistanceLightThreshold = 5.0e4; //above the typical "light" resistance of 1.3k
float photocellResistanceDarkThreshold  = 5.0e5; //below the typical "dark" resistance of 1.3M

float dividerVoltageLightThreshold = photocellVoltageFromResistance(photocellResistanceLightThreshold);
float dividerVoltageDarkThreshold  = photocellVoltageFromResistance(photocellResistanceDarkThreshold);

int timeInLightThreshold = 1000; //time in millis
int timeInDarkThreshold  = 1000; //time in millis

int sampleDelay = 50;

void setup() {
  pinMode(exceptionPin, OUTPUT);
  digitalWrite(exceptionPin,LOW);
  
  Serial.begin(9600);
  //comment out the next two lines to test motor control and sensing only
  startNotification();
  waitForStartReply();

  pinMode(motorEnablePin,   OUTPUT);
  pinMode(motorForwardPin,  OUTPUT);
  pinMode(motorBackwardPin, OUTPUT);
  
  digitalWrite(motorEnablePin,   LOW);
  digitalWrite(motorForwardPin,  LOW);
  digitalWrite(motorBackwardPin, LOW);
}

void loop() {
  String response;
  
  waitUntilConsistentlyLight(); //this is to prevent the camera from triggering in an initially-unlit room

  while(true) {
    waitUntilConsistentlyDark();
    //comment out the next two lines to test motor control and sensing only
    requestPhoto();
    Serial.println("After request");
    response = waitForPhotoResponse();
    Serial.println("After response");
    if(response.equals("left")) {
      activateMotor("left",500);
    } else if(response.equals("right")) {
      activateMotor("right",500);
    } else if(response.equals("error")) {
      digitalWrite(exceptionPin,HIGH);
    }
    waitUntilConsistentlyLight();
    digitalWrite(exceptionPin,LOW);
  }
}

bool isLight() {return fromAdcToVoltage(analogRead(dividerPin)) < dividerVoltageLightThreshold;}
bool isDark()  {return fromAdcToVoltage(analogRead(dividerPin)) > dividerVoltageDarkThreshold;}

float fromAdcToVoltage(int pinReading) { 
  return (float)pinReading * arduinoVoltage / 1023.0; 
}

/*
float dutyCycleToPwm(float dutyCycle)  { 
  return (int)(dutyCycle * 255); 
}
*/

void activateMotor(String motorDirection, int time)   {
  digitalWrite(motorEnablePin,HIGH);
  
  if(motorDirection.equals("left")) { 
    digitalWrite(motorBackwardPin,HIGH);
  } else {
    digitalWrite(motorForwardPin,HIGH);
  }
  
  delay(time);
  
  digitalWrite(motorEnablePin,LOW);
  digitalWrite(motorForwardPin,LOW);
  digitalWrite(motorBackwardPin,LOW);
}

void waitUntilConsistentlyDark(void) {
  int timeInDark = 0;
  int initialTimeInDark;

  //Serial.println(millis());
  while(timeInDark<timeInDarkThreshold) {
    //Serial.println(timeInDark);
    //Serial.println(analogRead(dividerPin));
    delay(sampleDelay);

    if(isDark()) {
      if(timeInDark==0) {
        initialTimeInDark = millis();
        timeInDark = sampleDelay;
      } else {
        timeInDark = millis()-initialTimeInDark;
      }
    } else {
      timeInDark = 0;
    }
  }
  //Serial.println(millis());
  //Serial.println(timeInDark);
}

void waitUntilConsistentlyLight(void) {
  int timeInLight = 0;
  int initialTimeInLight;

  while(timeInLight<timeInLightThreshold) {
    //Serial.println(timeInLight);
    //Serial.println(isLight());
    //Serial.println(dividerVoltageLightThreshold);
    //Serial.println(dividerVoltageDarkThreshold);
    delay(sampleDelay);

    if(isLight()) {
      if(timeInLight==0) {
        initialTimeInLight = millis();
        timeInLight = sampleDelay;
      } else {
        timeInLight = millis()-initialTimeInLight;
      }
    } else {
      timeInLight = 0;
    }
  }
  //Serial.println(timeInLight);
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

    while(!Serial.available()) {
    }
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

String waitForPhotoResponse() { //serial read
  String photoReplyString;

  while(1) {
    photoReplyString = "";
  
    while(!Serial.available()) {
    }
    while(Serial.available()) {
      delay(30);
      if(Serial.available()>0) {
        char c = Serial.read();
        photoReplyString += c;
      }
    }
    
    if(photoReplyString.equals("left") || photoReplyString.equals("right") || photoReplyString.equals("error")) {
      return photoReplyString;
    }
  }
}



