#include <RCSwitch.h>


RCSwitch mySwitch = RCSwitch();



void setup() {
  Serial.begin(115200);
 mySwitch.enableReceive(4);  // Receiver on interrupt 0 => that is pin #2 pinMode(led,OUTPUT);
 pinMode(5, OUTPUT);
}


void loop() {


  
  if (mySwitch.available()) {
    
    int value = mySwitch.getReceivedValue();
    
    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
          if(value == 200)
          {
            digitalWrite(5, HIGH);
          }
          else if(value == 400)
          {
            digitalWrite(5, LOW);
          }
         Serial.print("Received ");       
          Serial.print( mySwitch.getReceivedValue() );      
          Serial.println("");        
          mySwitch.resetAvailable();
          //delay(2000);
          }
  }
}
