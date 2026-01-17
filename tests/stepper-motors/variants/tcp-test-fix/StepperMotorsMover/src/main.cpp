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
const int SPEED_X = 1000;
const int SPEED_Y = 2000;
const int CENTER = 512;

// ---- Steppers ----
int speed = 30; // RPM
AccelStepper accelStepperX(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper accelStepperY(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

// ---- WiFi/UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
int status = WL_IDLE_STATUS;

IPAddress stationIP(192, 168, 4, 1);  // static IP for ESP32
IPAddress localIP(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(8888);
WiFiClient client;
unsigned int localPort = 7777;

// ---- Failsafe: if no valid packets are received for this duration, stop updating steppers ----
const unsigned long PACKET_TIMEOUT_MS = 1000; // 1s without data -> enter failsafe
unsigned long lastPacketMillis = 0;
bool failsafeActive = false;

#define DELAY_MS 10   // small delay between packets

void setup() {
  /* Serial.begin(9600);
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
  Serial.println("Connected to controller"); */

  Serial.begin(9600);
  while (!Serial) { ; }
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
      Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.config(stationIP);
    status = WiFi.begin(ssid, pass);

    Serial.print("Wifi.status() after begin(): ");
    Serial.print(status);
    Serial.print("\n");

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  server.begin();
  Serial.println("TCP server started, waiting for Controller...");

  // initialize packet timer
  lastPacketMillis = millis();

  // Init stepper speeds
  accelStepperX.setSpeed(speed);
  accelStepperY.setSpeed(speed);
}

void moveSteppers(uint8_t output[]) {
  int joystickX = output[0] * 4;
  int joystickY = output[1] * 4;
  int degree = 270; //CHANGE THIS VALUE TO CHANGE POSITION LIMITS
  int limit = degree * 4;

  if (joystickX < CENTER - DEADZONE) {
    accelStepperX.setSpeed(SPEED_X);
  }
  else if (joystickX > CENTER + DEADZONE) {
    accelStepperX.setSpeed(-SPEED_X);
  }
  else {
    accelStepperX.setSpeed(0);
  }

  if (joystickY < CENTER - DEADZONE) {
    accelStepperY.setSpeed(SPEED_Y);
  }
  else if (joystickY > CENTER + DEADZONE) {
    accelStepperY.setSpeed(-SPEED_Y);
  }
  else {
    accelStepperY.setSpeed(0);
  }

  long posX = accelStepperX.currentPosition();
  long posY = accelStepperY.currentPosition();

  if ((posX >= limit && accelStepperX.speed() > 0) || (posX <= -limit && accelStepperX.speed() < 0)) {
      accelStepperX.setSpeed(0);
  }
  if ((posY >= limit && accelStepperY.speed() > 0) || (posY <= -limit && accelStepperY.speed() < 0)) {
      accelStepperY.setSpeed(0);
  }
  accelStepperX.runSpeed();
  accelStepperY.runSpeed();
}

void loop() {

  /* if (client.connected() && client.available() >= 4) {
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
  delay(DELAY_MS); */


}