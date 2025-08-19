#include <Stepper.h>

const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

const int buttonPin1 = 7;
const int buttonPin2 = 8;

int buttonState1 = 0;
int buttonState2 = 0;

//step angle = 1.8 degrees, steps = 360/1.8 = 200
Stepper stepper1 = Stepper(200, 2, 3, 4, 5);


void setup() {
  stepper1.setSpeed(40);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
}

void loop() {
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);

  if (buttonState1 == HIGH && buttonState2 != HIGH) {
    stepper1.step(1); //screw around w this
  }
  else if (buttonState2 == HIGH && buttonState1 != HIGH) {
    stepper1.step(-1);
  }
}
