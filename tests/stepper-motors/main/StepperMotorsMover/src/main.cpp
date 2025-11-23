#include <AccelStepper.h>
#include <Arduino.h>

/*
Moves one Stepper Motor with limits
  - currently can only move 90 degrees (DEG_90 limit) in both directions
*/

const int DEG_90 = 360;
const int DEG_180 = 720;

const int EN_PIN = 8;
const int STEP_PIN = 9;
const int DIR_PIN = 10;

AccelStepper accelStepperX(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

const int JOYSTICK_X = A1;

const int DEADZONE = 100; 
const int SPEEDX = 1000;
int center = 512;   


void setup() {
  Serial.begin(9600);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  accelStepperX.setMaxSpeed(1500);
  accelStepperX.setAcceleration(800);
  accelStepperX.setCurrentPosition(0); //for now unless we want a diff starting pos
}

void loop() {
  int joystickX = analogRead(JOYSTICK_X);

  if (joystickX < center - DEADZONE) {
    accelStepperX.setSpeed(SPEEDX);
  }
  else if (joystickX > center + DEADZONE) {
    accelStepperX.setSpeed(-SPEEDX);
  } else {
    accelStepperX.setSpeed(0);
  }

  long pos = accelStepperX.currentPosition();
  Serial.println(pos);

  if (pos >= DEG_90 & accelStepperX.speed() > 0) {
      accelStepperX.setSpeed(0);
  }
  else if (pos <= -DEG_90 && accelStepperX.speed() < 0) {
      accelStepperX.setSpeed(0);
  }
  accelStepperX.runSpeed();
}