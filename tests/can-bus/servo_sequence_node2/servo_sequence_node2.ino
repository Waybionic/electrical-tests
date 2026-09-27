#include <SPI.h>
#include <mcp_can.h>
#include <Servo.h>

#define CAN_CS 10
#define SERVO_PIN 9

MCP_CAN CAN0(CAN_CS);
Servo servo2;

void setup() {
  Serial.begin(115200);

  servo2.attach(SERVO_PIN);
  servo2.write(0);

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN INIT FAILED");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
  Serial.println("NODE 2 READY");
}

void loop() {
  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    unsigned long canId;
    byte len;
    byte data[8];

    CAN0.readMsgBuf(&canId, &len, data);

    if (canId == 0x100 && len >= 1 && data[0] == 1) {
      Serial.println("Servo 2");
      servo2.write(90);
      delay(1000);
      servo2.write(0);
    }
  }
}
