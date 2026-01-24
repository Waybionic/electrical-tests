#include <AccelStepper.h>

// --- Wiring ---
#define EN_PIN_1 4
#define STEP_PIN_1 2
#define DIR_PIN_1 3

#define JOYSTICK_PIN A0

// --- Buttons ---
#define BTN1 10 // green
#define BTN2 11 // blue
#define BTN3 12 // yellow

AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);

// --- Acceleration presets (steps/sec²) ---
const float ACCEL1 = 1000;  // slow acceleration
const float ACCEL2 = 2000;  // medium acceleration
const float ACCEL3 = 4000;  // fast acceleration

float selectedAcceleration = ACCEL1; // default acceleration
int currentSpeedLevel = 1; // 1, 2, or 3

// --- Joystick tuning ---
const int DEADZONE = 30;
int BASE_JOYSTICK_POSITION;

// --- Button state tracking ---
bool btn1LastState = HIGH;
bool btn2LastState = HIGH;
bool btn3LastState = HIGH;

// --- Joystick position tracking ---
long currentTargetPosition = 0;

void setup()
{
  Serial.begin(9600);

  while (!Serial)
    ;

  pinMode(EN_PIN_1, OUTPUT);
  digitalWrite(EN_PIN_1, LOW); // Enable driver

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  stepper1.setMaxSpeed(8000); // set a high max speed for acceleration control
  stepper1.setAcceleration(ACCEL1); // default acceleration

  BASE_JOYSTICK_POSITION = analogRead(JOYSTICK_PIN);
  
  Serial.println("JoystickStepper System Ready");
  Serial.println("Speed Level: 1 (400 steps/sec)");
}

void loop()
{
  // --- Button speed selection with edge detection ---
  bool btn1Current = digitalRead(BTN1);
  bool btn2Current = digitalRead(BTN2);
  bool btn3Current = digitalRead(BTN3);

  // Check for button press (transition from HIGH to LOW)
  if (btn1Current == LOW && btn1LastState == HIGH)
  {
    selectedAcceleration = ACCEL1;
    currentSpeedLevel = 1;
    stepper1.setAcceleration(selectedAcceleration);
    Serial.println("Speed Level: 1 (1000 steps/sec²)");
  }
  else if (btn2Current == LOW && btn2LastState == HIGH)
  {
    selectedAcceleration = ACCEL2;
    currentSpeedLevel = 2;
    stepper1.setAcceleration(selectedAcceleration);
    Serial.println("Speed Level: 2 (2000 steps/sec²)");
  }
  else if (btn3Current == LOW && btn3LastState == HIGH)
  {
    selectedAcceleration = ACCEL3;
    currentSpeedLevel = 3;
    stepper1.setAcceleration(selectedAcceleration);
    Serial.println("Speed Level: 3 (4000 steps/sec²)");
  }

  // Update button states
  btn1LastState = btn1Current;
  btn2LastState = btn2Current;
  btn3LastState = btn3Current;

  // --- Joystick motion control ---
  int currentPosition = analogRead(JOYSTICK_PIN);
  int delta = currentPosition - BASE_JOYSTICK_POSITION;

  if (abs(delta) > DEADZONE)
  {
    // Map joystick to position change rate
    float normalized = constrain((float)delta / 512.0, -1.0, 1.0);
    // Increase target position based on joystick
    currentTargetPosition += (long)(normalized * 50.0); // adjust 50 for sensitivity
    stepper1.moveTo(currentTargetPosition);
  }

  stepper1.run();
}
