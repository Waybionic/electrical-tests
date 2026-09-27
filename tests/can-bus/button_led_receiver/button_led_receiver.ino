#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define LED_PIN 13

MCP_CAN CAN0(CAN_CS_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN initialized!");
  } else {
    Serial.println("CAN initialization FAILED!");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
}

void loop() {
  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    unsigned long canId;
    byte len = 0;
    byte data[8];

    CAN0.readMsgBuf(&canId, &len, data);

    // Only respond to the button message.
    if (canId == 0x100 && len >= 1) {
      bool buttonPressed = data[0];
      digitalWrite(LED_PIN, buttonPressed ? HIGH : LOW);

      Serial.print("Received button: ");
      Serial.println(buttonPressed ? "PRESSED" : "RELEASED");
    }
  }
}
