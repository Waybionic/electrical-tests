#include <Arduino.h>
#include <AccelStepper.h>

const int JOY_X = A0;
const int JOY_Y = A1;

const int DIR_PIN_2 = 10;
const int STEP_PIN_2 = 9;
const int EN_PIN_2 = 8;

const int EN_PIN_1 = 4;
const int STEP_PIN_1 = 5;
const int DIR_PIN_1 = 6;

const int SPEED = 800;

AccelStepper StepperX(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);
AccelStepper StepperY(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);


// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(EN_PIN_2, OUTPUT);
  digitalWrite(EN_PIN_2, LOW); // Enable the stepper driver

  pinMode(EN_PIN_1, OUTPUT);
  digitalWrite(EN_PIN_1, LOW); // Enable the stepper driver

  StepperX.setMaxSpeed(1000);
  StepperX.setAcceleration(500);
  StepperY.setMaxSpeed(1000);
  StepperY.setAcceleration(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  int xVal = analogRead(JOY_X);
  int stepsToMove = map(xVal, 0, 1023, -SPEED, SPEED);
  StepperX.setSpeed(stepsToMove);
  StepperX.runSpeed();

  int yVal = analogRead(JOY_Y);
  int stepsToMove1 = map(yVal, 0, 1023, -SPEED, SPEED);
  StepperY.setSpeed(stepsToMove1);
  StepperY.runSpeed();
}