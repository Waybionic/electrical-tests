#include <SPI.h>
#include <mcp_can.h>
#include <Servo.h>

#define CAN_CS 10
#define SERVO_PIN 9

MCP_CAN CAN0(CAN_CS);
Servo servo1;

void setup() {
  Serial.begin(115200);

  servo1.attach(SERVO_PIN);
  servo1.write(0);

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN INIT FAILED");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
  Serial.println("MASTER READY");
}

void loop() {
  Serial.println("Servo 1");
  servo1.write(90);
  delay(1000);
  servo1.write(0);
  delay(500);

  byte data[1] = {1};
  if (CAN0.sendMsgBuf(0x100, 0, 1, data) == CAN_OK) {
    Serial.println("Servo 2 command sent");
  } else {
    Serial.println("CAN SEND FAILED");
  }

  // Give Servo 2 its one-second movement period, then a break.
  delay(1000);
  delay(500);
}
