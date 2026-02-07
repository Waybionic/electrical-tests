#include <Stepper.h>

const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

const int stepperPin5 = 6;
const int stepperPin6 = 7;
const int stepperPin7 = 8;
const int stepperPin8 = 9;

const int stepperPin9 = 10;
const int stepperPin10 = 11;
const int stepperPin11 = 12;
const int stepperPin12 = 13;

const int joystick1PinX = A0;
const int joystick1PinY = A1;
const int joystick2PinX = A2;
const int joystick2PinY = A3;

const int buttonPin1 = 8;
const int buttonPin2 = 9;

int buttonState1 = 0;
int buttonState2 = 0;
int joystick1X = 0;
int joystick1Y = 0;
int joystick2X = 0;
int joystick2Y = 0;

int speed = 30;

//step angle = 1.8 degrees, steps = 360/1.8 = 200
Stepper stepper1 = Stepper(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);
Stepper stepper2 = Stepper(200, stepperPin5, stepperPin6, stepperPin7, stepperPin8);
Stepper stepper3 = Stepper(200, stepperPin9, stepperPin10, stepperPin11, stepperPin12);

void setup() {
  stepper1.setSpeed(speed);
  stepper2.setSpeed(speed);
  stepper3.setSpeed(speed);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(joystick1PinX, INPUT);
  pinMode(joystick1PinY, INPUT);
  pinMode(joystick2PinX, INPUT);
}

void loop() {
  int center = 512;
  int deadzone = 30;
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  joystick1X = analogRead(joystickPin1X);
  joystick1Y = analogRead(joystickPin1Y);
  joystick2X = analogRead(joystickPin)

  if (joystick1X > center + deadzone) {
    stepper1.step(1);
  }
  else if (joystick1X < center - deadzone) {
    stepper1.step(-1);
  }

  if (joystick1Y > center + deadzone) {
    stepper2.step(1);
  }
  else if (joystick1Y < center - deadzone) {
    stepper2.step(-1);
  }

  if (joystick2X > center + deadzone) {
    stepper3.step(1);
  }
  else if (joystick2X < center - deadzone) {
    stepper3.step(-1);
  }

  if (buttonState1 == HIGH && buttonState2 != HIGH) {
    if (speed > 5) {
      speed -= 5;
      stepper1.setSpeed(speed);
      stepper2.setSpeed(speed);
      stepper3.setSpeed(speed);
      delay(50);
    }
  }
  else if (buttonState2 == HIGH && buttonState1 != HIGH) {
    if (speed < 40) {
      speed += 5;
      stepper1.setSpeed(speed);
      stepper2.setSpeed(speed);
      stepper3.setSpeed(speed);
      delay(50);
    }
  }

}
