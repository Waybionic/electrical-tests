#include <WiFi.h>
#include <WiFiUdp.h>
#include <Stepper.h>

// ---- Stepper pins ----
const int stepperPin1 = 8;
const int stepperPin2 = 9;
const int stepperPin3 = 10;
const int stepperPin4 = 11;

// ---- Stepper ----
int speed = 30; // RPM
Stepper stepper1(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);

// ---- Joystick ----
const int JOYSTICK_PIN_X = A1;   // adjust to your wiring
const int JOYSTICK_PIN_Y = A0;   // not used right now
int CENTER = 675;                // expected joystick center (will calibrate at startup)
const int DEADZONE = 40;         // widen this if motor still creeps

// ---- WiFi/UDP ----
// char ssid[] = "CHANGE_ME_SSID";
// char pass[] = "CHANGE_ME_PASSWORD";
// IPAddress stationIP(192, 168, 4, 4);  // static IP for ESP32

// WiFiUDP Udp;
// unsigned int localPort = 7777;

// helper to smooth analog readings
int readJoystick(int pin) {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 5;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  pinMode(JOYSTICK_PIN_X, INPUT);
  pinMode(JOYSTICK_PIN_Y, INPUT);

  // Connect to WiFi
  // WiFi.config(stationIP);
  // WiFi.begin(ssid, pass);
  // Serial.print("Connecting to WiFi");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println(" connected!");
  // Serial.println(WiFi.localIP());

  // // Start UDP listener
  // Udp.begin(localPort);
  // Serial.print("Listening on UDP port ");
  // Serial.println(localPort);

  // // Init stepper speed
  // stepper1.setSpeed(speed);

  // // Calibrate joystick center at startup
  // CENTER = readJoystick(JOYSTICK_PIN_X);
  // Serial.print("Calibrated CENTER = ");
  // Serial.println(CENTER);
}

void loop() {
  // ---- Joystick control ----
  int joystickX = analogRead(JOYSTICK_PIN_X);
  int joystickY = analogRead(JOYSTICK_PIN_Y);

  // Debug joystick values
  Serial.print("JoystickX: ");
  Serial.print(joystickX);
  Serial.print(" | JoystickY: ");
  Serial.println(joystickY);

  // Apply deadzone
  if (joystickX < CENTER - DEADZONE) {
    Serial.println("1");
    stepper1.step(1);
  } else if (joystickX > CENTER + DEADZONE) {
    Serial.println("-1");
    stepper1.step(-1);
  } else {
    // inside deadzone → do nothing
  }

  // ---- UDP handling (for external control if needed) ----
  // int packetSize = Udp.parsePacket();
  // if (packetSize >= 3) {
  //   uint8_t buff[3];
  //   Udp.read(buff, 3);
  //   int steps1 = (int8_t)buff[0]; // signed for forward/back
  //   if (steps1 != 0) stepper1.step(steps1);
  // } else if (packetSize > 0) {
  //   // discard incomplete packets
  //   while (Udp.available()) Udp.read();
  // }

  delay(20);  // small delay to control stepper speed
}
