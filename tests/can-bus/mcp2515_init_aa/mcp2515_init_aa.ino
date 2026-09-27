#include <SPI.h>
#include <AA_MCP2515.h>

const uint8_t CAN_CS = 10;
const int8_t CAN_INT = -1;  // INT not connected

const CANBitrate::Config CAN_BITRATE =
    CANBitrate::Config_8MHz_500kbps;

// bitrate, CS pin, INT pin
CANConfig config(CAN_BITRATE, CAN_CS, CAN_INT);
CANController CAN(config);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting MCP2515 test...");

  auto result = CAN.begin(CANController::Mode::Normal);

  if (result == CANController::OK) {
    Serial.println("SUCCESS: MCP2515 initialized!");
  } else {
    Serial.print("FAILED: ");
    Serial.println((int)result);
  }
}

void loop() {
}
