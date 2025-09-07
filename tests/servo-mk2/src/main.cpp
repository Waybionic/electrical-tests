// This code is for the sender of rotation coordinates to the arduino

#define SERVO_1_ANALOG A0
#define SERVO_2_ANALOG A1
#define SERVO_3_ANALOG A2 

#include "Arduino.h"
#include "JoystickReader.h"
#include "ButtonIncrementPair.h"
#include "Encoding.h"
#include "WiFiS3.h"

char ssid[] = "CHANGE_ME_SSID";      // your network SSID (name)
char pass[] = "CHANGE_ME_PASSWORD"; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

IPAddress stationIP(192, 168, 4, 2);     // Local IP address for the access point
IPAddress accessPointIP(192, 168, 4, 1); // IP address of the access point
unsigned int remotePort = 8888;          // local port to listen for UDP packets
unsigned int localPort = 2390;           // local port to listen for UDP packets

JoystickReader joystickReaderStepper1(0, 180, false);  //replace object with stepper. (angles based on ranges of arm) (this one was bottom plate)
JoystickReader joystickReaderStepper2(0, 180, false); // (45-90 is for the elbow)
JoystickReader joystickReaderStepper3(0, 180, false); //(more info in the discord)
ButtonIncrementPair buttonIncrementPair4 = {0, (const uint8_t[]){6, 5}, (bool[]){false, false}}; // Button on pin 4, increment on pin 3, increment value 1

WiFiUDP Udp;

// Checks if the device is connected to the WiFi network
bool isConnected()
{
  return WiFi.status() == WL_AP_CONNECTED;
}

// Initialize all controller readers
void initializeReaders()
{

  joystickReaderStepper1.setUp(SERVO_1_ANALOG); //replace with stepper
  joystickReaderStepper2.setUp(SERVO_2_ANALOG);
  joystickReaderStepper3.setUp(SERVO_3_ANALOG);
}

// Sends the joystick data as an encoded integer over UDP
// The encoding combines the x and y coordinates into a single integer
// The x coordinate is shifted left by 16 bits and combined with the y coordinate
void sendJoystickData(int servo1, int servo2, int servo3, int servo4)
{
  // Serial.print("Sending joystick data: ");
  // Serial.print(servo1);
  // Serial.println();
  // Serial.print(", ");
  // Serial.print(servo2);
  // Serial.print(", ");
  // Serial.print(servo3);
  // Serial.print(", ");
  // Serial.println(servo4);
  uint8_t payload[4] = {servo1, servo2, servo3, servo4};
  Udp.beginPacket(stationIP, remotePort);
  Udp.write(payload, 4);
  Udp.endPacket();
}

// The main communication loop for sending joystick data over UDP
void mainCommunicationLoop()
{
  if (!isConnected())
  {
    return;
  }
  processButtonStep(&buttonIncrementPair4); //replace servo with stepper
  sendJoystickData(joystickReaderStepper1.getUpdatedCurrentAngle(), joystickReaderStepper2.getUpdatedCurrentAngle(),
                   joystickReaderStepper3.getUpdatedCurrentAngle(), buttonIncrementPair4.currentAngle);
  delay(UPDATE_DELAY_MILLIS);
}

// Prints the WiFi status to the serial monitor
void printWiFiStatus()
{

  // print the SSID of the network you're attached to:

  Serial.print("SSID: ");

  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:

  IPAddress ip = WiFi.localIP();

  Serial.print("IP Address: ");

  Serial.println(ip);
}

void setup()
{

  // Initialize serial and wait for port to open:

  Serial.begin(9600);

  while (!Serial)
  {

    ; // wait for serial port to connect. Needed for native USB port only
  }

  initializeReaders();

  Serial.println("Access Point Web Server");

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

  Serial.println("Starting UDP server...");
  // start listening for UDP packets on the specified port
  Udp.begin(localPort);

  Serial.println("UDP server started on port " + String(localPort));
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

      Serial.println("Device connected to AP");
    }
    else
    {

      // a device has disconnected from the AP, and we are back in listening mode

      Serial.println("Device disconnected from AP");
    }
  }
  mainCommunicationLoop();
}