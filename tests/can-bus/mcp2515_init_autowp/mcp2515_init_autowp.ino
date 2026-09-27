#include <SPI.h>
#include <mcp2515.h>

// CS = D10. Slow SPI down to 1 MHz for troubleshooting.
MCP2515 mcp2515(10, 1000000);

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("Starting MCP2515 SPI test...");

  mcp2515.reset();
  delay(100);

  MCP2515::ERROR result =
      mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);

  if (result == MCP2515::ERROR_OK) {
    Serial.println("SUCCESS: MCP2515 responding!");
  } else {
    Serial.print("FAILED. Error = ");
    Serial.println((int)result);
  }
}

void loop() {
}
