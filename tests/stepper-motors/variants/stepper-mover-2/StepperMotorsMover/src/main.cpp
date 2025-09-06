#include <WiFiS3.h>
#include <Stepper.h>
#include <Arduino.h>


#define SERVO_1 5
#define SERVO_2 6
#define SERVO_3 9
#define SERVO_4 10
#define DELAY_MS 50



//my stuff for stepper 
const int stepperPin1 = 2;
const int stepperPin2 = 3;
const int stepperPin3 = 4;
const int stepperPin4 = 5;

const int joystickPinX = A0;
const int joystickPinY = A1;

const int buttonPin1 = 7;
const int buttonPin2 = 8;

//10-13 second l298n
const int stepperPin5 = 10;
const int stepperPin6 = 11;
const int stepperPin7 = 12;
const int stepperPin8 = 13;

int buttonState1 = 0;
int buttonState2 = 0;
int joystickX = 0;
int joystickY = 0;

int speed = 30;

//step angle = 1.8 degrees, steps = 360/1.8 = 200
Stepper stepper1 = Stepper(200, stepperPin1, stepperPin2, stepperPin3, stepperPin4);
Stepper stepper2 = Stepper(200, stepperPin5, stepperPin6, stepperPin7, stepperPin8);





int status = WL_IDLE_STATUS;

char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
WiFiUDP Udp;


int servoRotationsXY[4] = {0, 45, 130, 0};

IPAddress stationIP(192, 168, 4, 2);

unsigned int localPort = 8888;

// Reads the encoded int and transforms it into an int
int readEncodedInt()
{
    int incomingInt;
    Udp.read((char *)&incomingInt, sizeof(int));
    return incomingInt;
}

// servos have explicit angles, but steppers can keep rotating - harold
void decodeAllRotations(int rep, int output[])
{
    output[0] = (rep >> 24) & 0xFF; // Extract angle1
    output[1] = (rep >> 16) & 0xFF; // Extract angle2
    output[2] = (rep >> 8) & 0xFF;  // Extract angle3
    output[3] = rep & 0xFF;         // Extract angle4
}


void moveServos(int output[])
{
    servo1.write(output[0]);
    servo2.write(output[1]);
    servo3.write(output[2]);
    servo4.write(output[3]);
}



void printWifiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void setup()
{
    // Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

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

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        WiFi.config(stationIP);
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    Serial.println("Connected to WiFi");
    printWifiStatus();

    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    Udp.begin(localPort);
}

void loop()
{
    int packetSize = Udp.parsePacket();
    if (packetSize >= sizeof(int))
    {
        decodeAllRotations(readEncodedInt(), servoRotationsXY);
        moveServos(servoRotationsXY);
    }
    delay(DELAY_MS);
}