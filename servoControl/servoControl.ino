#include <Servo.h>

Servo bigServo;

void setup() {
  // put your setup code here, to run once:
  bigServo.attach(9);
}

void loop() {
  // put your main code here, to run repeatedly:
  bigServo.write(270);
  delay(10000);
  bigServo.write(0);
  delay(10000);
}

void exit() {

}
