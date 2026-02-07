
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Stepper.h>

// ---- Stepper pins ----
const int stepperXPin1 = 6;
const int stepperXPin2 = 7;
const int stepperXPin3 = 8;
const int stepperXPin4 = 9;

const int stepperYPin5 = 2;
const int stepperYPin6 = 3;
const int stepperYPin7 = 4;
const int stepperYPin8 = 5;

// ---- Stepper objects ----
int speed = 30; // RPM
Stepper stepperX(200, stepperXPin1, stepperXPin2, stepperXPin3, stepperXPin4);
Stepper stepperY(200, stepperYPin5, stepperYPin6, stepperYPin7, stepperYPin8);

// ---- Joysticks ----
const int JOYSTICK_PIN_X = A0;  // joystick for stepper X
const int JOYSTICK_PIN_Y = A1;  // joystick for stepper Y

int CENTER_X = 498;  // will calibrate at startup
int CENTER_Y = 505;  
const int DEADZONE = 80;

// ---- WiFi/UDP (optional, unused here) ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
IPAddress stationIP(192, 168, 4, 4);
WiFiUDP Udp;
unsigned int localPort = 7777;

// ---- Helper: smooth joystick readings ----
int readJoystick(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 10;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) continue;

  // Init stepper speed
  stepperX.setSpeed(speed);
  stepperY.setSpeed(speed);

  // Calibrate joystick centers
  CENTER_X = readJoystick(JOYSTICK_PIN_X);
  CENTER_Y = readJoystick(JOYSTICK_PIN_Y);

  Serial.print("Calibrated CENTER_X = ");
  Serial.println(CENTER_X);
  Serial.print("Calibrated CENTER_Y = ");
  Serial.println(CENTER_Y);
}

void loop() {
  // ---- Joystick control ----
  int joystickX = readJoystick(JOYSTICK_PIN_X);
  int joystickY = readJoystick(JOYSTICK_PIN_Y);

  // Debug joystick values
  Serial.print("JoystickX: ");
  Serial.print(joystickX);
  Serial.print(" | JoystickY: ");
  Serial.println(joystickY);

  // Apply deadzone for X
  if (joystickX < CENTER_X - DEADZONE) {
    stepperX.step(-1);
  } else if (joystickX > CENTER_X + DEADZONE) {
    stepperX.step(1);
  }

  // Apply deadzone for Y
  if (joystickY < CENTER_Y - DEADZONE) {
    stepperY.step(-1);
  } else if (joystickY > CENTER_Y + DEADZONE) {
    stepperY.step(1);
  }

  delay(5); // smooth stepping
}
