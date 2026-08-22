/*
  Arduino Uno R4 WiFi USB Servo Controller

  Receives one command per line over USB serial:
    servoNumber,physicalAngle

  Examples:
    1,135
    4,270

  Servo signal pins:
    Servo 1 -> D3
    Servo 2 -> D5
    Servo 3 -> D6
    Servo 4 -> D9

  IMPORTANT: For several servos, use a separate regulated 5 V supply.
  Connect the supply ground to Arduino GND.
*/

#include <Servo.h>

const byte SERVO_COUNT = 4;
const byte SERVO_PINS[SERVO_COUNT] = {3, 5, 6, 9};
const unsigned long BAUD_RATE = 115200;
const float PHYSICAL_SERVO_RANGE = 270.0;
const float START_PHYSICAL_ANGLE = 90.0;

// Physical limits after converting the original 180-degree scale by 1.5:
// Servo 1: 0-180 -> 0-270
// Servo 2: 0-75  -> 0-112.5
// Servo 3: 60-180 -> 90-270
// Servo 4: 0-180 -> 0-270
const float SERVO_MIN_DEGREES[SERVO_COUNT] = {0.0, 0.0, 90.0, 0.0};
const float SERVO_MAX_DEGREES[SERVO_COUNT] = {270.0, 112.5, 270.0, 270.0};

Servo servos[SERVO_COUNT];

void setup() {
  Serial.begin(BAUD_RATE);

  for (byte i = 0; i < SERVO_COUNT; i++) {
    servos[i].attach(SERVO_PINS[i]);
    int startCommand = round(START_PHYSICAL_ANGLE * 180.0 / PHYSICAL_SERVO_RANGE);
    servos[i].write(startCommand);  // Start at 90 physical degrees.
  }

  Serial.println("READY");
}

void loop() {
  if (Serial.available() == 0) {
    return;
  }

  // The GUI sends physical 270-degree positions, such as "2,112.5".
  String command = Serial.readStringUntil('\n');
  command.trim();

  int commaPosition = command.indexOf(',');
  if (commaPosition < 1) {
    Serial.println("ERROR: use servoNumber,angle");
    return;
  }

  int servoNumber = command.substring(0, commaPosition).toInt();
  float physicalAngle = command.substring(commaPosition + 1).toFloat();

  if (servoNumber < 1 || servoNumber > SERVO_COUNT) {
    Serial.println("ERROR: invalid servo number");
    return;
  }

  byte servoIndex = servoNumber - 1;
  if (physicalAngle < SERVO_MIN_DEGREES[servoIndex] ||
      physicalAngle > SERVO_MAX_DEGREES[servoIndex]) {
    Serial.println("ERROR: angle outside servo limit");
    return;
  }

  // Servo.write() uses a logical 0-180 scale even when the physical servo
  // travels 270 degrees. Convert the displayed physical angle to that scale.
  int servoCommand = round(physicalAngle * 180.0 / PHYSICAL_SERVO_RANGE);
  servoCommand = constrain(servoCommand, 0, 180);
  servos[servoIndex].write(servoCommand);

  // Do not acknowledge every valid slider update. A continuously moving
  // slider can otherwise fill the PC's incoming serial buffer over time.
}
