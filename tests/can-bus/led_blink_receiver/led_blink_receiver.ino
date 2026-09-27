#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS 10
#define LED_PIN LED_BUILTIN

MCP_CAN CAN0(CAN_CS);

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("RECEIVER");

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN INIT FAILED");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
  Serial.println("CAN READY");
}

void loop() {
  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    unsigned long canId;
    byte len;
    byte data[8];

    CAN0.readMsgBuf(&canId, &len, data);

    Serial.print("Received ID: 0x");
    Serial.print(canId, HEX);
    Serial.print(" Data: ");
    Serial.println(data[0]);

    if (canId == 0x100 && len >= 1) {
      digitalWrite(LED_PIN, data[0] ? HIGH : LOW);
    }
  }
}
