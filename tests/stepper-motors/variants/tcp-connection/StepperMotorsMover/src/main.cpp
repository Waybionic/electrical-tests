#include <WiFi.h>
#include <WiFiUdp.h>
#include <Stepper.h>

// ---- Stepper pins ----
const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

const int stepperPin5 = 6;
const int stepperPin6 = 7;
const int stepperPin7 = 8;
const int stepperPin8 = 9;

const int stepperPin9 = 10;
const int stepperPin10 = 11;
const int stepperPin11 = 12;
const int stepperPin12 = 13;

// ---- Steppers ----
int speed = 30; // RPM
Stepper stepper1(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);
Stepper stepper2(200, stepperPin5, stepperPin6, stepperPin7, stepperPin8);
Stepper stepper3(200, stepperPin9, stepperPin10, stepperPin11, stepperPin12);

// ---- WiFi/UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
IPAddress stationIP(192, 168, 4, 2);  // static IP for ESP32

WiFiUDP Udp;
unsigned int localPort = 8888;

#define DELAY_MS 10   // small delay between packets

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.config(stationIP);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.println(WiFi.localIP());

  // Start UDP listener
  Udp.begin(localPort);
  Serial.printf("Listening on UDP port %d\n", localPort);

  // Init stepper speeds
  stepper1.setSpeed(speed);
  stepper2.setSpeed(speed);
  stepper3.setSpeed(speed);
}

// Example: buffer[0] = steps for motor1, buffer[1] = steps for motor2, buffer[2] = steps for motor3
void moveSteppers(uint8_t output[]) {
  int steps1 = (int8_t)output[0]; // cast to signed for forward/back
  int steps2 = (int8_t)output[1];
  int steps3 = (int8_t)output[2];

  if (steps1 != 0) stepper1.step(steps1);
  if (steps2 != 0) stepper2.step(steps2);
  if (steps3 != 0) stepper3.step(steps3);
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize >= 3) {
    uint8_t buff[3];
    Udp.read(buff, 3);
    moveSteppers(buff);
  } else if (packetSize > 0) {
    // discard incomplete packets
    while (Udp.available()) Udp.read();
  }

  delay(DELAY_MS);
}