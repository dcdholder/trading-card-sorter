int counter = 1;
int ledPin = 13;

void setup() {
  Serial.begin(9600);
  startNotification();
  waitForStartReply();
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin,HIGH);
  delay(100);
  digitalWrite(ledPin,LOW);
  delay(100);
 
  Serial.println(counter);
  Serial.flush();
  
  if(counter==10) {counter = 1;}
  counter++;
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

void startNotification(void) {
  String startNotification = "Arduino ready";
  
  Serial.println(startNotification);
  Serial.flush();
}
