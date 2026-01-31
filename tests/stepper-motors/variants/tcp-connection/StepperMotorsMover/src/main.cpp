//Mover = TCP Server

//Current issues
//Joystick values aren't stable so the stepper motor isn't staying stable
  //To do: add deadzones to every location the steppers is in
  //so minimal joystick movements don't move the stepper
//Stepper movement itself is slow itself (slow on Korede's part as well even without TCP)
  //To do: figure out which constants to adjust to move steppers as fast as possible
//Initial TCP handshake + connection time takes about 10 seconds
  //To do: find ways to minimize this handshake time
//Delays: controller needs to be held for a long time to go from 0 to 180
  //To do: either increase speed of controller values going up or control acceleration
//Delays: a bit of a pause after letting go of controller 

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
const int DEADZONE = 10;
const int CENTER = 90;
const int SPEED_X = 500;
const int SPEED_Y = 200;
const int ACCELERATION_X = 1000;
const int ACCELERATION_Y = 1000;
const int MAX_SPEED_X = 800;
const int MAX_SPEED_Y = 800;
const float MIN_ANGLE_X = -135.0; //change depending on desired angle limits
const float MAX_ANGLE_X = 135.0; //ditto
const float MIN_ANGLE_Y = -135.0; //can test with negative values
const float MAX_ANGLE_Y = 135.0;

const float MOVEMENT_SCALE = 3; //>1 = faster, <1 = slower

const int STEPS_PER_REV = 200;   // 200 for 1.8° motors, 400 for 0.9°
const int MICROSTEPS = 4;        // Use microstepping! 1, 2, 4, 8, 16, 32 (set on driver)
const float STEPS_PER_DEGREE = (STEPS_PER_REV * MICROSTEPS) / 360.0;


// ---- Steppers ----
//int speed = 30; // RPM
AccelStepper accelStepperX(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper accelStepperY(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

// ---- WiFi/UDP ----
char ssid[] = "CHANGE_ME_SSID";
char pass[] = "CHANGE_ME_PASSWORD";
int status = WL_IDLE_STATUS;

IPAddress stationIP(192, 168, 4, 2);  // Mover IP
IPAddress gateway(192, 168, 4, 1); // Controller IP
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(8888);
WiFiClient client;
unsigned int localPort = 8888;

// ---- Failsafe: if no valid packets are received for this duration, stop updating steppers ----
const unsigned long PACKET_TIMEOUT_MS = 1000; // 1s without data -> enter failsafe
unsigned long lastPacketMillis = 0;
bool failsafeActive = false;

#define DELAY_MS 10   // small delay between packets

void setup() {
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

      Serial.print("Wifi.status() after begin(): ");
      Serial.print(status);
      Serial.print("\n");

      // wait 10 seconds for connection:
      delay(5000);
  }
  Serial.println("Connected to WiFi");

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  server.begin();
  Serial.println("TCP server started, waiting for Controller...");

  // initialize packet timer
  lastPacketMillis = millis();

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
  int joystickX = output[0];
  int joystickY = output[1];
  
  /* Serial.print("JoystickX: ");
  Serial.println(joystickX);
  Serial.print("JoystickY: ");
  Serial.println(joystickY); */

  //new code for moving steppers passing position angles to mover (like we did for servos)
  if (abs(joystickX - CENTER) > DEADZONE) {
    float targetAngle = (map(joystickX, 0, 180, (long)(MIN_ANGLE_X * 100), (long)(MAX_ANGLE_X * 100)) / 100.0) * MOVEMENT_SCALE;
    targetAngle = constrain(targetAngle, MIN_ANGLE_X, MAX_ANGLE_X);
    long targetSteps = (long)(targetAngle * STEPS_PER_DEGREE);
    accelStepperX.moveTo(targetSteps);
  }
  
  if (abs(joystickY - CENTER) > DEADZONE) {
    float targetAngle = (map(joystickY, 0, 180, (long)(MIN_ANGLE_Y * 100), (long)(MAX_ANGLE_Y * 100)) / 100.0) * MOVEMENT_SCALE;
    targetAngle = constrain(targetAngle, MIN_ANGLE_Y, MAX_ANGLE_Y);
    long targetSteps = (long)(targetAngle * STEPS_PER_DEGREE);
    accelStepperY.moveTo(targetSteps);
  }
  
  //previously used code for moving steppers
  /* if (joystickX < CENTER - DEADZONE) {
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
  accelStepperY.runSpeed(); */
}

void loop() {
  accelStepperX.run();
  accelStepperY.run();

  WiFiClient client = server.available();

  if (client) {
    //Serial.println("Controller connected!");
    
    while (client.connected()) {
      accelStepperX.run();
      accelStepperY.run();
      
      if (client.available() >= 6) {  // Controller sends 6 bytes
        //Serial.println("Receiving data...");
        uint8_t buff[6];
        client.readBytes(buff, 6);
        
        /* Serial.print("Received: [");
        for (int i = 0; i < 6; i++) {
          Serial.print(buff[i]);
          if (i < 5) Serial.print(", ");
        }
        Serial.println("]"); */
        
        // Use first 2 values (packet[0] and packet[1]) for the joystick
        moveSteppers(buff);
        lastPacketMillis = millis();
        failsafeActive = false;
      }
    }
    
    Serial.println("Controller disconnected.");
    client.stop();
  }

}