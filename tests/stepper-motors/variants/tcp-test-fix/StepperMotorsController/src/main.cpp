// Controller = UDP Sender (was TCP Client)
// Fire-and-forget UDP datagrams — no handshake, no connection overhead.

#define STEPPER_1_ANALOG A4
#define STEPPER_2_ANALOG A5
#define STEPPER_3_ANALOG A1
#define STEPPER_4_ANALOG A0
#define STEPPER_5_ANALOG A2
#define STEPPER_6_ANALOG A3

#include "Arduino.h"
#include "JoystickPair.h"
#include "Encoding.h"
#include "WiFiS3.h"
#include <WiFiUdp.h>

char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";

int const PACKET_SIZE = 6;
int status = WL_IDLE_STATUS;

IPAddress accessPointIP(192, 168, 4, 1); // This board's AP IP
IPAddress moverIP(192, 168, 4, 2);       // Mover's IP
const unsigned int MOVER_UDP_PORT = 5677;

WiFiUDP udp;

// Joystick readers (kept from original)
JoystickReader joystickReaderStepper1(0, 180, false);
JoystickReader joystickReaderStepper2(0, 135, false);
JoystickReader joystickReaderStepper3(90, 180, false);
JoystickReader joystickReaderStepper4(0, 180, false);
JoystickReader joystickReaderStepper5(90, 180, false);
JoystickReader joystickReaderStepper6(0, 180, false);

bool isConnected() {
  return WiFi.status() == WL_AP_LISTENING || WiFi.status() == WL_AP_CONNECTED;
}

void initializeReaders() {
  joystickReaderStepper1.setUp(STEPPER_1_ANALOG);
  joystickReaderStepper2.setUp(STEPPER_2_ANALOG);
  joystickReaderStepper3.setUp(STEPPER_3_ANALOG);
  joystickReaderStepper4.setUp(STEPPER_4_ANALOG);
  joystickReaderStepper5.setUp(STEPPER_5_ANALOG);
  joystickReaderStepper6.setUp(STEPPER_6_ANALOG);
}

void sendJoystickData(uint8_t packet[], int n) {
  udp.beginPacket(moverIP, MOVER_UDP_PORT);
  udp.write(packet, n);
  udp.endPacket();
}

void mainCommunicationLoop() {
  if (!isConnected()) {
    return;
  }

  uint8_t packet[PACKET_SIZE] = {
    (uint8_t)(analogRead(STEPPER_1_ANALOG) / 4),
    (uint8_t)(analogRead(STEPPER_2_ANALOG) / 4),
    (uint8_t)(analogRead(STEPPER_3_ANALOG) / 4),
    (uint8_t)(analogRead(STEPPER_4_ANALOG) / 4),
    (uint8_t)(analogRead(STEPPER_5_ANALOG) / 4),
    (uint8_t)(analogRead(STEPPER_6_ANALOG) / 4)
  };
  sendJoystickData(packet, PACKET_SIZE);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  initializeReaders();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true) ;
  }

  // Create access point
  WiFi.config(accessPointIP);
  Serial.print("Creating access point: ");
  Serial.println(ssid);
  status = WiFi.beginAP(ssid, pass);

  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    while (true) ;
  }

  delay(3000); // shorter wait for AP to stabilize (was 10s)

  // Start UDP — no connection needed, just start sending
  udp.begin(0);  // any local port
  Serial.println("UDP ready, sending to Mover.");
}

void loop() {
  if (status != WiFi.status()) {
    status = WiFi.status();
  }
  mainCommunicationLoop();
}