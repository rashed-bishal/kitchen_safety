#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();
void setup() {
 
  Serial.begin(115200);
  mySwitch.enableTransmit(4);

}
void loop() {

  mySwitch.send(505, 24);
  delay(2000);

}
