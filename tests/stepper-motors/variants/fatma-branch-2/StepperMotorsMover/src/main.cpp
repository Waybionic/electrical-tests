#include <AccelStepper.h>
#include <Arduino.h>

#define EN_PIN_1   8
#define STEP_PIN_1 9
#define DIR_PIN_1  10

#define EN_PIN_2   4
#define STEP_PIN_2 5
#define DIR_PIN_2  6

AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

void setup() {
  pinMode(EN_PIN_1, OUTPUT);
  pinMode(EN_PIN_2, OUTPUT);

  digitalWrite(EN_PIN_1, LOW);
  digitalWrite(EN_PIN_2, LOW);

  stepper1.setMaxSpeed(1000);
  stepper1.setAcceleration(500);

  stepper2.setMaxSpeed(1000);
  stepper2.setAcceleration(500);
}

void loop() {
  stepper1.move(500);
  stepper2.move(500);

  stepper1.run();
  stepper2.run();
}
