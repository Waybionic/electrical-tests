#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define BUTTON_PIN 2

MCP_CAN CAN0(CAN_CS_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Many DAOKI MCP2515 modules use an 8 MHz crystal.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN initialized!");
  } else {
    Serial.println("CAN initialization FAILED!");
    while (1);
  }

  CAN0.setMode(MCP_NORMAL);
}

void loop() {
  // INPUT_PULLUP: pressed = LOW, released = HIGH.
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  byte message[1];
  message[0] = buttonPressed ? 1 : 0;

  // CAN ID = 0x100.
  CAN0.sendMsgBuf(0x100, 0, 1, message);

  Serial.print("Button: ");
  Serial.println(buttonPressed ? "PRESSED" : "RELEASED");
  delay(50);
}
