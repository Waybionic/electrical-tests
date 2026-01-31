// Controller = TCP Client

// This code is for the sender of rotation coordinates to the arduino

#define STEPPER_1_ANALOG A4
#define STEPPER_2_ANALOG A5
#define STEPPER_3_ANALOG A1
#define STEPPER_4_ANALOG A0
#define STEPPER_5_ANALOG A2
#define STEPPER_6_ANALOG A3

const int UPDATE_DELAY_MILLIS = 1; // Delay for joystick updates

#include "Arduino.h"
#include "JoystickPair.h"
#include "Encoding.h"
#include "WiFiS3.h"

char ssid[] = "CHANGE_ME_SSID";      // your network SSID (name)
char pass[] = "CHANGE_ME_PASSWORD"; // your network password (use for WPA, or use as key for WEP)

bool const DEBUG_PRINTS = true; // Set to true to enable debug prints
int const PACKET_SIZE = 6;
uint8_t lastSentPacket[6] = {255, 255, 255, 255, 255, 255};

int status = WL_IDLE_STATUS;

IPAddress stationIP(192, 168, 4, 2);     // Local IP address for the access point
IPAddress accessPointIP(192, 168, 4, 1); // IP address of the access point
unsigned int remotePort = 8888;          // local port to listen for UDP packets
unsigned int localPort = 2390;           // local port to listen for UDP packets

//haven't flashed yet, previous flash had different args passed
JoystickReader joystickReaderStepper1(0, 180, false);
JoystickReader joystickReaderStepper2(0, 135, false);
JoystickReader joystickReaderStepper3(90, 180, false);
JoystickReader joystickReaderStepper4(0, 180, false);
JoystickReader joystickReaderStepper5(90, 180, false);
JoystickReader joystickReaderStepper6(0, 180, false);

/* JoystickPair joystickPair1(&joystickReaderServo1, &joystickReaderServo2);
JoystickPair joystickPair2(&joystickReaderServo3, &joystickReaderServo4);
JoystickPair joystickPair3(&joystickReaderServo5, &joystickReaderServo6); */

WiFiClient client;
unsigned long lastUpdateMillis = 0;

// Checks if the device is connected to the WiFi network
bool isConnected() {
  return WiFi.status() == WL_AP_LISTENING || WiFi.status() == WL_AP_CONNECTED;
}

// Initialize all controller readers
void initializeReaders() {
  joystickReaderStepper1.setUp(STEPPER_1_ANALOG);
  joystickReaderStepper2.setUp(STEPPER_2_ANALOG);
  joystickReaderStepper3.setUp(STEPPER_3_ANALOG);
  joystickReaderStepper4.setUp(STEPPER_4_ANALOG);
  joystickReaderStepper5.setUp(STEPPER_5_ANALOG);
  joystickReaderStepper6.setUp(STEPPER_6_ANALOG);
}

// Sends the joystick data as an encoded integer over UDP
// The encoding combines the x and y coordinates into a single integer
// The x coordinate is shifted left by 16 bits and combined with the y coordinate
void sendJoystickData(uint8_t packet[], int n) {
  // Revert to original 6-byte payload format (no sequence numbers)
  if (client.connected()) {
    Serial.println("Client connected, sending...");
    client.write(packet, n);
  }
  else {
    Serial.println("Client not connected, trying to reconnect...");
    client.stop();
    while (!client.connect(stationIP, remotePort)) {
      Serial.println("Reconnecting...");
      delay(1000);
    }
    Serial.println("Reconnected to Mover");
  }
}

// The main communication loop for sending joystick data over UDP
void mainCommunicationLoop() {
  if (!isConnected()) {
    Serial.print("Not connected to WiFi. Status: ");
    Serial.println(WiFi.status());
    return;
  }
  /* unsigned long currentMillis = millis();
  uint8_t packet[PACKET_SIZE] = {0, 0, 0, 0, 0, 0};
  if (currentMillis - lastUpdateMillis < UPDATE_DELAY_MILLIS) {
    return; // Skip this loop iteration if the delay hasn't passed
  }
  lastUpdateMillis = currentMillis;

  //// ----- ////// new code to send stuff to the mover
  bool changed = false;
  for (int i = 0; i < PACKET_SIZE; i++) {
    // if there as been difference
    if (packet[i] != lastSentPacket[i]) {
      changed = true;
      break;
    }
  }

  if (changed) {
    memcpy(lastSentPacket, packet, PACKET_SIZE); // copy the packets
    sendJoystickData(packet, PACKET_SIZE); // send the packets
  } */

  uint8_t packet[PACKET_SIZE] = {analogRead(STEPPER_1_ANALOG) / 4, 
    analogRead(STEPPER_2_ANALOG) / 4,
    analogRead(STEPPER_3_ANALOG) / 4,
    analogRead(STEPPER_4_ANALOG) / 4,
    analogRead(STEPPER_5_ANALOG) / 4,
    analogRead(STEPPER_6_ANALOG) / 4};
  sendJoystickData(packet, PACKET_SIZE);
}

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  initializeReaders();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  WiFi.config(accessPointIP);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);
  status = WiFi.beginAP(ssid, pass); //Create open network

  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    while (true)
      ;
  }
  delay(10000); // wait 10 seconds for connection:

  // start the web server on port 80
  while (!client.connect(stationIP, remotePort)) {
    delay(1000);
  }
  Serial.println("Connected to Mover!");
}

void loop()
{
  if (status != WiFi.status()) {
    status = WiFi.status();
  }
  mainCommunicationLoop();
}