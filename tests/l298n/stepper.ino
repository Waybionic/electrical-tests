// Stepper motor pins (usually 4 pins for common steppers)
const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

// Button pins
const int buttonPin1 = 7;  // Clockwise rotation
const int buttonPin2 = 8;  // Counter-clockwise rotation
const int buttonPin3 = 9;  // Faster speed
const int buttonPin4 = 10; // Slower speed

// Variables
int buttonState1 = 0;
int buttonState2 = 0;
int buttonState3 = 0;
int buttonState4 = 0;

int stepDelay = 5; // Delay between steps (controls speed)
int stepPosition = 0; // Current step position

// Step sequence for 4-wire stepper motor
int stepSequence[4][4] = {
  {1, 0, 0, 1},
  {1, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 1},
};

void setup() {
  pinMode(stepperPin1, OUTPUT);
  pinMode(stepperPin2, OUTPUT);
  pinMode(stepperPin3, OUTPUT);
  pinMode(stepperPin4, OUTPUT);

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  pinMode(buttonPin4, INPUT);
}

void loop() {
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);
  buttonState4 = digitalRead(buttonPin4);

  // Direction control
  if (buttonState1 == HIGH && buttonState2 != HIGH) {
    // Rotate clockwise
    stepClockwise();
    delay(stepDelay);
  }
  else if (buttonState2 == HIGH && buttonState1 != HIGH) {
    // Rotate counter-clockwise
    stepCounterClockwise();
    delay(stepDelay);
  }
}

void stepClockwise() {
  stepPosition++;
  if (stepPosition >= 4) {
    stepPosition = 0;
  }
  setStep(stepPosition);
}

void stepCounterClockwise() {
  stepPosition--;
  if (stepPosition < 0) {
    stepPosition = 3;
  }
  setStep(stepPosition);
}

void setStep(int step) {
  digitalWrite(stepperPin1, stepSequence[step][0]);
  digitalWrite(stepperPin2, stepSequence[step][1]);
  digitalWrite(stepperPin3, stepSequence[step][2]);
  digitalWrite(stepperPin4, stepSequence[step][3]);
}