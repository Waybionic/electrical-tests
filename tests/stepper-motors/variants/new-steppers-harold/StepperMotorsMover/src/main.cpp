#include <WiFi.h>
#include <Arduino.h>
#include <AccelStepper.h>

const int EN_PIN_1 = 4;
const int STEP_PIN_1 = 5;
const int DIR_PIN_1 = 6;

const int EN_PIN_2 = 8;
const int STEP_PIN_2 = 9;
const int DIR_PIN_2 = 10;

AccelStepper accelStepperX(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper accelStepperY(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

const int JOYSTICK_X = A1;
const int JOYSTICK_Y = A0;

const int DEADZONE = 100; 
const int SPEEDX = 1000;
const int CENTER = 512;

void setup() {
  Serial.begin(9600);
  pinMode(EN_PIN_1, OUTPUT);
  pinMode(EN_PIN_2, OUTPUT);
  digitalWrite(EN_PIN_1, LOW);
  digitalWrite(EN_PIN_2, LOW);

  accelStepperX.setMaxSpeed(1500);
  accelStepperX.setAcceleration(800);
  accelStepperX.setCurrentPosition(0); //for now unless we want a diff starting pos
  accelStepperY.setMaxSpeed(1500);
  accelStepperY.setAcceleration(800);
  accelStepperY.setCurrentPosition(0); //for now unless we want a diff starting pos
}

void loop() {
  int joystickX = analogRead(JOYSTICK_X);
  int joystickY = analogRead(JOYSTICK_Y);
  int degree = 270; //CHANGE THIS VALUE TO CHANGE POSITION LIMITS
  int limit = degree * 4;

  if (joystickX < CENTER - DEADZONE) {
    accelStepperX.setSpeed(SPEEDX);
  }
  else if (joystickX > CENTER + DEADZONE) {
    accelStepperX.setSpeed(-SPEEDX);
  }
  else {
    accelStepperX.setSpeed(0);
  }

  long pos = accelStepperX.currentPosition();
  Serial.println(pos);

  if ((pos >= limit && accelStepperX.speed() > 0) || (pos <= -limit && accelStepperX.speed() < 0)) {
      accelStepperX.setSpeed(0);
  }
  accelStepperX.runSpeed();
}
