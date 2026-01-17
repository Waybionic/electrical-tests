#include <AccelStepper.h>

// --- Wiring ---
#define EN_PIN_1   8
#define STEP_PIN_1 10
#define DIR_PIN_1  9

#define JOYSTICK_PIN A0

AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);

// --- Tuning ---
const float MAX_SPEED = 1500;  // steps/sec (set for your motor/driver)
const int   DEADZONE  = 20;      // ADC counts around center
int BASE_JOYSTICK_POSITION;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  pinMode(EN_PIN_1, OUTPUT);
  digitalWrite(EN_PIN_1, LOW); // Enable the driver (LOW is enabled)
  stepper1.setMaxSpeed(MAX_SPEED * 2);  // runSpeed uses this as a guard
  stepper1.setAcceleration(1000.0); // not used by runSpeed, but good practice
  stepper1.setCurrentPosition(0);
  BASE_JOYSTICK_POSITION = analogRead(JOYSTICK_PIN);
}

void loop() {
  int currentPosition = analogRead(JOYSTICK_PIN);
  if (abs(currentPosition - BASE_JOYSTICK_POSITION) > DEADZONE) {
    // Map joystick position to speed
    float speed = map(currentPosition, 0, 1023, -MAX_SPEED, MAX_SPEED);
    stepper1.setSpeed(speed);
  } else {
    // Within deadzone, stop the motor
    stepper1.setSpeed(0);
  }
  stepper1.runSpeed();
}
