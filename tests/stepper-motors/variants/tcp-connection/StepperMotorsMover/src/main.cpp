#include <WiFi.h>
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

/* const int stepperPin9 = 10;
const int stepperPin10 = 11;
const int stepperPin11 = 12;
const int stepperPin12 = 13; */

// ---- Steppers ----
int speed = 30; // RPM
Stepper stepper1(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);
Stepper stepper2(200, stepperPin5, stepperPin6, stepperPin7, stepperPin8);
//Stepper stepper3(200, stepperPin9, stepperPin10, stepperPin11, stepperPin12);

// ---- WiFi/UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";

//last digit may be 3 or 4
IPAddress stationIP(192, 168, 4, 3);  // static IP for ESP32

WiFiClient client;
unsigned int localPort = 7777;

#define DELAY_MS 10   // small delay between packets

void setup() {
  Serial.begin(9600);
  while (!Serial)
    continue;

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

  if (client.connect(stationIP, 7777)) {
    Serial.println("Connected to TCP server.");
  }
  else {
    Serial.println("Failed to connect to TCP server.");
  }

  // Init stepper speeds
  stepper1.setSpeed(speed);
  stepper2.setSpeed(speed);
  //stepper3.setSpeed(speed);
}

void moveSteppers(uint8_t output[]) {
  int center = 512;
  int deadzone = 30;
  int steps1 = output[0] * 4;
  int steps2 = output[1] * 4;
  int steps3 = output[2] * 4;

  if (steps1 > center + deadzone) {
    int distance = steps1 - center - deadzone;
    int num_steps = 1;
    if (distance > 200) num_steps = 2;
    if (distance > 300) num_steps = 6;
    if (distance > 400) num_steps = 10;
    
    for (int i = 0; i < num_steps; i++) {
      stepper1.step(1);
      if (i < num_steps - 1) delay(2);
    }
  }
  else if (steps1 < center - deadzone) {
    int distance = center - deadzone - steps1;
    int num_steps = 1;
    if (distance > 200) num_steps = 2;
    if (distance > 300) num_steps = 6;
    if (distance > 400) num_steps = 10;
  
    for (int i = 0; i < num_steps; i++) {
      stepper1.step(-1);
      if (i < num_steps - 1) delay(2);
    }
  }
}

void loop() {
  if (client.connected() && client.available() >= 4) {
    uint8_t buff[4];
    client.readBytes(buff, 4);
    moveSteppers(buff);
  }

  if (!client.connected()) {
    if (client.connect(stationIP, 7777)) {
      Serial.println("Reconnected to TCP server.");
    }
  }

  delay(DELAY_MS);
}