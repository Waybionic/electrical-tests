#include <WiFi.h>
#include <AccelStepper.h>
#include <Stepper.h>

// ---- Stepper pins ----
const int EN_PIN_1 = 4;
const int STEP_PIN_1 = 5;
const int DIR_PIN_1 = 6;

const int EN_PIN_2 = 8;
const int STEP_PIN_2 = 9;
const int DIR_PIN_2 = 10;

// ---- Other constants ----
const int DEADZONE = 100;
const int SPEED = 1000;
const int CENTER = 512;

// ---- Steppers ----
int speed = 30; // RPM
AccelStepper accelStepper1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper accelStepper2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

// ---- WiFi/UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";

IPAddress stationIP(192, 168, 4, 1);  // static IP for ESP32
IPAddress localIP(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiClient client;
unsigned int localPort = 7777;

#define DELAY_MS 10   // small delay between packets

void setup() {
  Serial.begin(9600);
  while (!Serial)
    continue;

  // Connect to WiFi
  WiFi.config(localIP, gateway, subnet);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  while (!client.connect(stationIP, 7777)) {
    Serial.println("Connecting to controller...");
    delay(1000); 
  }
  Serial.println("Connected to controller");

  // Init stepper speeds
  accelStepper1.setSpeed(speed);
  accelStepper2.setSpeed(speed);
}

void moveSteppers(uint8_t output[]) {
  

  /* int center = 512;
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
      accelStepper1.step(1);
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

  if (steps2 > center + deadzone) {
    int distance = steps2 - center - deadzone;
    int num_steps = 1;
    if (distance > 200) num_steps = 2;
    if (distance > 300) num_steps = 6;
    if (distance > 400) num_steps = 10;
    
    for (int i = 0; i < num_steps; i++) {
      stepper2.step(1);
      if (i < num_steps - 1) delay(2);
    }
  }
  else if (steps2 < center - deadzone) {
    int distance = center - deadzone - steps2;
    int num_steps = 1;
    if (distance > 200) num_steps = 2;
    if (distance > 300) num_steps = 6;
    if (distance > 400) num_steps = 10;
  
    for (int i = 0; i < num_steps; i++) {
      stepper2.step(-1);
      if (i < num_steps - 1) delay(2);
    }
  } */
}

void loop() {

  if (client.connected() && client.available() >= 4) {
    Serial.println("Client is connected.");
    uint8_t buff[4];
    client.readBytes(buff, 4);
    moveSteppers(buff);
  }
  else if (!client.connected()) {
    Serial.println("Disconnected, trying to reconnect...");
    while (!client.connect(stationIP, 7777)) {
      delay(1000);
    }
  }
  delay(DELAY_MS);
}