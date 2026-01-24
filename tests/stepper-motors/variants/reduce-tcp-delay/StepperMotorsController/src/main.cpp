// Controller = TCP Client

// This code is for the sender of rotation coordinates to the arduino

#define SERVO_1_ANALOG A4
#define SERVO_2_ANALOG A5
#define SERVO_3_ANALOG A1
#define SERVO_4_ANALOG A0
#define SERVO_5_ANALOG A2
#define SERVO_6_ANALOG A3

const int UPDATE_DELAY_MILLIS = 1; // Delay for joystick updates

#include "Arduino.h"
#include "JoystickPair.h"
#include "Encoding.h"how ca
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
JoystickReader joystickReaderServo1(0, 45, true);
JoystickReader joystickReaderServo2(0, 135, true);
JoystickReader joystickReaderServo3(90, 180, false);
JoystickReader joystickReaderServo4(0, 180, false);
JoystickReader joystickReaderServo5(90, 180, false); // Added joystick for servo5
JoystickReader joystickReaderServo6(0, 180, false);  // Added joystick for servo6

JoystickPair joystickPair1(&joystickReaderServo1, &joystickReaderServo2);
JoystickPair joystickPair2(&joystickReaderServo3, &joystickReaderServo4);
JoystickPair joystickPair3(&joystickReaderServo5, &joystickReaderServo6);

WiFiClient client;

unsigned long lastUpdateMillis = 0;

// Checks if the device is connected to the WiFi network
bool isConnected()
{
  return WiFi.status() == WL_AP_LISTENING || WiFi.status() == WL_AP_CONNECTED;
}

// Initialize all controller readers
void initializeReaders()
{

  joystickReaderServo1.setUp(SERVO_1_ANALOG);
  joystickReaderServo2.setUp(SERVO_2_ANALOG);
  joystickReaderServo3.setUp(SERVO_3_ANALOG);
  joystickReaderServo4.setUp(SERVO_4_ANALOG);
  joystickReaderServo5.setUp(SERVO_5_ANALOG); // Added joystick setup for servo5
  joystickReaderServo6.setUp(SERVO_6_ANALOG);
}

// void debugPrintJoystickData(u_int8_t packet[], int n)
// {
//   if (!DEBUG_PRINTS)
//     return;
//   Serial.print("Sending joystick data: ");
//   for (int i = 0; i < n; i++)
//   {
//     Serial.print(packet[i]);
//     if (i < n - 1)
//       Serial.print(", ");
//   }
//   Serial.println();
// }

// Sends the joystick data as an encoded integer over UDP
// The encoding combines the x and y coordinates into a single integer
// The x coordinate is shifted left by 16 bits and combined with the y coordinate
void sendJoystickData(uint8_t packet[], int n)
{
  // Revert to original 6-byte payload format (no sequence numbers)
  // debugPrintJoystickData(packet, n);

  if (client.connected()) {
    Serial.println("Client connected, sending...");
    client.write(packet, n);
  }
  else {
    Serial.println("Client not connected, trying to reconnect...");
    client.stop();
    while (!client.connect(stationIP, remotePort))
    {
      Serial.println("Reconnecting...");
      delay(1000);
    }
    Serial.println("Reconnected to Mover");
  }
}

// The main communication loop for sending joystick data over UDP
void mainCommunicationLoop()
{
  if (!isConnected())
  {
    Serial.print("Not connected to WiFi. Status: ");
    Serial.println(WiFi.status());
    return;
  }
  unsigned long currentMillis = millis();
  uint8_t packet[PACKET_SIZE] = {0, 0, 0, 0, 0, 0};
  joystickPair1.updateBothJoysticksMax(currentMillis, &packet[0]);
  joystickPair2.updateBothJoysticksMax(currentMillis, &packet[2]);
  joystickPair3.updateBothJoysticksMax(currentMillis, &packet[4]);
  if (currentMillis - lastUpdateMillis < UPDATE_DELAY_MILLIS)
  {
    return; // Skip this loop iteration if the delay hasn't passed
  }
  lastUpdateMillis = currentMillis;

  //// ----- ////// new code to send stuff to the mover
  bool changed = false;
  for (int i = 0; i < 6; i++)
  {
    // if there as been difference
    if (packet[i] != lastSentPacket[i])
    {
      changed = true;
      break;
    }
  }

  if (changed)
  {
    memcpy(lastSentPacket, packet, 6); // copy the packets
    sendJoystickData(packet, 6); // send the packets

    for (int i = 0; i < 6; i++)
    {
      // Serial.print(packet[i]);
      // Serial.print(" ");
    }
    // Serial.println();
  }

  /// --- /// new code to send stuff to the mover
}

// Prints the WiFi status to the serial monitor
void printWiFiStatus()
{

  // print the SSID of the network you're attached to:

  // Serial.print("SSID: ");

  // Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:

  IPAddress ip = WiFi.localIP();

  // Serial.print("IP Address: ");

  // Serial.println(ip);
}

void setup() {

  // Initialize serial and wait for port to open:

  Serial.begin(9600);

  while (!Serial)
  {

    ; // wait for serial port to connect. Needed for native USB port only
  }

  initializeReaders();

  // Serial.println("Access Point Web Server");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();

  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  WiFi.config(accessPointIP);

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:

  status = WiFi.beginAP(ssid, pass);

  if (status != WL_AP_LISTENING)
  {
    Serial.println("Creating access point failed");
    // don't continue
    while (true)
      ;
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80

  // you're connected now, so print out the status

  printWiFiStatus();

  while (!client.connect(stationIP, remotePort))
  {
    // Serial.println("Connecting to Mover...");
    delay(1000);
  }
  Serial.println("Connected to Mover!");
}

void loop()
{
  // compare the previous status to the current status

  if (status != WiFi.status())
  {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED)
    {
      // a device has connected to the AP
      // Serial.println("Device connected to AP");
    }
    else
    {
      // a device has disconnected from the AP, and we are back in listening mode
      // Serial.println("Device disconnected from AP");
    }
  }
  mainCommunicationLoop();
}