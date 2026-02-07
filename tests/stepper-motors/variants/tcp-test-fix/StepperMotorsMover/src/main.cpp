// Mover = UDP Receiver (was TCP Server)
// No handshake, no connection overhead — just fire-and-forget datagrams.

#include <WiFi.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>

// ---- Stepper pins ----
const int EN_PIN_1 = 4;
const int STEP_PIN_1 = 5;
const int DIR_PIN_1 = 6;

const int EN_PIN_2 = 8;
const int STEP_PIN_2 = 9;
const int DIR_PIN_2 = 10;

// ---- Other constants ----
const int DEADZONE = 30;
const int CENTER = 512;
const int SPEED_X = 2000;
const int SPEED_Y = 2000;
const int ACCELERATION_X = 4000;
const int ACCELERATION_Y = 4000;
const int MAX_SPEED_X = 6000;
const int MAX_SPEED_Y = 6000;

// ---- Steppers ----
AccelStepper accelStepperX(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper accelStepperY(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

// ---- WiFi / UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
int status = WL_IDLE_STATUS;

IPAddress stationIP(192, 168, 4, 2);  // Mover IP
const unsigned int LOCAL_UDP_PORT = 5677;

WiFiUDP udp;

// ---- Failsafe ----
const unsigned long PACKET_TIMEOUT_MS = 500; // 500ms without data -> stop motors
unsigned long lastPacketMillis = 0;
bool failsafeActive = false;

const int PACKET_SIZE = 6;

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true) ;
  }

  // Connect to the Controller's access point
  WiFi.config(stationIP);
  while (status != WL_CONNECTED) {
    Serial.print("Connecting to AP: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(3000);  // shorter retry than before
  }
  Serial.println("WiFi connected!");

  // Start listening for UDP packets
  udp.begin(LOCAL_UDP_PORT);
  Serial.print("UDP listening on port ");
  Serial.println(LOCAL_UDP_PORT);

  lastPacketMillis = millis();

  // Stepper enable pins
  pinMode(EN_PIN_1, OUTPUT);
  pinMode(EN_PIN_2, OUTPUT);
  digitalWrite(EN_PIN_1, LOW);
  digitalWrite(EN_PIN_2, LOW);

  // Init stepper speeds
  accelStepperX.setMaxSpeed(MAX_SPEED_X);
  accelStepperX.setAcceleration(ACCELERATION_X);
  accelStepperX.setCurrentPosition(0);

  accelStepperY.setMaxSpeed(MAX_SPEED_Y);
  accelStepperY.setAcceleration(ACCELERATION_Y);
  accelStepperY.setCurrentPosition(0);
}

void moveSteppers(uint8_t output[]) {
  int joystickX = output[0] * 4;
  int joystickY = output[1] * 4;

  // Scale speed proportionally to joystick deflection
  if (joystickX < CENTER - DEADZONE) {
    float scale = (float)(CENTER - joystickX) / CENTER;
    accelStepperX.setSpeed(SPEED_X * scale);
  }
  else if (joystickX > CENTER + DEADZONE) {
    float scale = (float)(joystickX - CENTER) / CENTER;
    accelStepperX.setSpeed(-SPEED_X * scale);
  }
  else {
    accelStepperX.setSpeed(0);
  }

  if (joystickY < CENTER - DEADZONE) {
    float scale = (float)(CENTER - joystickY) / CENTER;
    accelStepperY.setSpeed(SPEED_Y * scale);
  }
  else if (joystickY > CENTER + DEADZONE) {
    float scale = (float)(joystickY - CENTER) / CENTER;
    accelStepperY.setSpeed(-SPEED_Y * scale);
  }
  else {
    accelStepperY.setSpeed(0);
  }
}

void loop() {
  // CRITICAL: drive steppers every iteration
  accelStepperX.runSpeed();
  accelStepperY.runSpeed();

  // Failsafe: stop motors if no data received recently
  if (millis() - lastPacketMillis > PACKET_TIMEOUT_MS && !failsafeActive) {
    accelStepperX.setSpeed(0);
    accelStepperY.setSpeed(0);
    failsafeActive = true;
    Serial.println("Failsafe: no data, motors stopped.");
  }

  // Drain all queued UDP packets, use the latest one
  uint8_t latestBuff[PACKET_SIZE];
  bool gotPacket = false;

  int packetLen = udp.parsePacket();
  while (packetLen > 0) {
    if (packetLen >= PACKET_SIZE) {
      udp.read(latestBuff, PACKET_SIZE);
      gotPacket = true;
    }
    // Discard any remaining bytes in this datagram
    udp.flush();
    packetLen = udp.parsePacket();  // check for another queued packet
  }

  if (gotPacket) {
    moveSteppers(latestBuff);
    lastPacketMillis = millis();
    failsafeActive = false;
  }
}