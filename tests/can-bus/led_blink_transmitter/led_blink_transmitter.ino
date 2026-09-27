#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 10

MCP_CAN CAN0(CAN_CS);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("TRANSMITTER");

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN INIT FAILED");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
  Serial.println("CAN READY");
}

void loop() {
  byte data[1];

  data[0] = 1;
  if (CAN0.sendMsgBuf(0x100, 0, 1, data) == CAN_OK) {
    Serial.println("Sent: ON");
  } else {
    Serial.println("SEND FAILED");
  }
  delay(1000);

  data[0] = 0;
  if (CAN0.sendMsgBuf(0x100, 0, 1, data) == CAN_OK) {
    Serial.println("Sent: OFF");
  } else {
    Serial.println("SEND FAILED");
  }
  delay(1000);
}
