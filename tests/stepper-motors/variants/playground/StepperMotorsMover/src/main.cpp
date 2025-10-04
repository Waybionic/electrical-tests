#include <WiFi.h>
#include <WiFiUdp.h>
#include <Stepper.h>

// ---- Stepper pins (safe on ESP32) ----
const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

// ---- Stepper ----
int speed = 30; // RPM
Stepper stepper1(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);

// ---- Joystick ----
// const int JOYSTICK_PIN_Y = A1; 
  // analog input (safe, input-only)
const int JOYSTICK_PIN_X = A0;
int CENTER = 498;                  // will calibrate at startup
const int DEADZONE = 80;         // widen this if motor still creeps

// ---- WiFi/UDP (optional, can be removed) ----
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
  stepper1.setSpeed(speed);

  // Calibrate joystick center at startup
  CENTER = readJoystick(JOYSTICK_PIN_X);
  Serial.print("Calibrated CENTER (X) = ");
  Serial.println(CENTER);
}

void loop() {
  // ---- Joystick control (Y axis) ----
  int joystickX = readJoystick(JOYSTICK_PIN_X);

  // Debug joystick values
  Serial.print("JoystickX: ");
  Serial.println(joystickX);

  // Apply deadzone
  if (joystickX < CENTER - DEADZONE) {
    stepper1.step(-1);   // move one way
  } else if (joystickX > CENTER + DEADZONE) {
    stepper1.step(1);    // move other way
  } else {
    // inside deadzone → no movement
  }

  delay(5);  // smooth stepping
}
