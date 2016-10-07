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

int ledPin = 13;
int motorPin = X; //figure something out

int motorAverageVoltage = 1.5; //average volts out

int dividerUpperThreshold = 10; //find good values for these, maybe calculate at compile time - should be high enough for the webcam to take a good picture
int dividerLowerThreshold = 1; 

int timeInLightThreshold = 1000;
int timeInDarkThreshold  = 1000;

int sampleDelay = 5;

void setup() {
  Serial.begin(9600);
  startNotification();
  waitForStartReply();
  pinMode(ledPin, OUTPUT);
}

void loop() {
	waitUntilConsistentlyLight(); //this is to prevent the camera from triggering in an initially-unlit room

	while(true) {
		waitUntilConsistentlyDark();
		requestPhoto();
		waitForPhotoResponse();
		activateMotor(); //for now, just activate the LED
		waitUntilConsistentlyLight();
		deactivateMotor(); //for now, just deactivate LED
	}
}

float fromAnalogToVoltage(int pinReading) { return (float)pinReading * 5.0 / 1023.0; }
float fromVoltageToPwm(float voltage)     { return (int)(voltage / 5.0 * 255); }

void activateMotor()   { analogWrite(motorPin,fromVoltageToPwm(motorAverageVoltage)); }
void deactivateMotor() { analogWrite(motorPin,0); }

void waitUntilConsistentlyDark(void) {
	int timeInDark = 0;
	int initialTimeInDark;

	while(timeInDark<timeInDarkThreshold) {
		delay(sampleDelay);
		
		if(toVoltage(analogRead(dividerPin))<dividerLowerThreshold) {
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
		
		if(toVoltage(analogRead(dividerPin))>dividerUpperThreshold) {
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
  while(1) {  
    String replyString = "";
    
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
	String photoRequest = "Say cheese!";
	
	Serial.println(photoRequest);
	Serial.flush();
}

void waitForPhotoResponse() { //serial read
	while(1) {
		String photoReplyString = "";
		
		while(!Serial.available()) {}
		while(Serial.available()) {
			delay(30);
			if(Serial.available()>0) {
				char c = Serial.read();
				replyString += c;
			}
		}
	}
	
	if(photoReplyString.equals("Cheese!")) {
		return;
	}
}
