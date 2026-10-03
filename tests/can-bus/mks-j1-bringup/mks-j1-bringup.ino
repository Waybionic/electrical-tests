#include "AA_MCP2515.h"

// Confirmed bench configuration:
// Arduino UNO R4 WiFi + DAOKI MCP2515/TJA1050 (8 MHz) at 500 kbps.
const CANBitrate::Config CAN_BITRATE =
    CANBitrate::Config_8MHz_500kbps;
const uint8_t CAN_CS = 10;
const int8_t CAN_INT = 2;

CANConfig config(CAN_BITRATE, CAN_CS, CAN_INT);
CANController CAN(config);

const uint16_t MOTOR_ID = 1;

uint8_t checksum(uint16_t id, const uint8_t *data, uint8_t len) {
  uint16_t sum = id;

  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }

  return sum & 0xFF;
}

bool sendFrame(uint16_t id, uint8_t *data, uint8_t len) {
  CANFrame frame(id, data, len);
  const auto result = CAN.write(frame);
  frame.print("MKS TX");

  if (result != CANController::IOResult::OK) {
    Serial.print("CAN write failed: ");
    Serial.println(static_cast<int8_t>(result));
    return false;
  }

  return true;
}

void enableMotor() {
  uint8_t data[] = {0xF3, 0x01, 0x00};
  data[2] = checksum(MOTOR_ID, data, 2);
  sendFrame(MOTOR_ID, data, sizeof(data));
}

void moveSpeed(bool ccw) {
  // Confirmed bring-up values: speed 0x0140 (320), acceleration 2.
  const uint16_t speed = 320;
  uint8_t data[] = {
      0xF6,
      static_cast<uint8_t>((ccw ? 0x80 : 0x00) | ((speed >> 8) & 0x0F)),
      static_cast<uint8_t>(speed & 0xFF),
      0x02,
      0x00,
  };
  data[4] = checksum(MOTOR_ID, data, 4);
  sendFrame(MOTOR_ID, data, sizeof(data));
}

void stopMotor() {
  uint8_t data[] = {0xF6, 0x00, 0x00, 0x02, 0x00};
  data[4] = checksum(MOTOR_ID, data, 4);
  sendFrame(MOTOR_ID, data, sizeof(data));
}

void printHelp() {
  Serial.println();
  Serial.println("MKS J1 bring-up commands:");
  Serial.println("  enable   Enable CAN ID 1");
  Serial.println("  run      Run with the confirmed speed frame");
  Serial.println("  reverse  Run in the opposite direction");
  Serial.println("  stop     Stop speed mode");
  Serial.println("  help     Show this list");
  Serial.println();
  Serial.println("No command is sent automatically at startup.");
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "enable") {
    enableMotor();
  } else if (command == "run") {
    moveSpeed(false);
  } else if (command == "reverse") {
    moveSpeed(true);
  } else if (command == "stop") {
    stopMotor();
  } else if (command == "help" || command == "h") {
    printHelp();
  } else if (command.length() > 0) {
    Serial.print("Unknown command: ");
    Serial.println(command);
    printHelp();
  }
}

void printResponses() {
  CANFrame frame;

  while (CAN.read(frame) == CANController::IOResult::OK) {
    frame.print("MKS RX");
  }
}

void setup() {
  Serial.begin(115200);

  while (CAN.begin(CANController::Mode::Normal) != CANController::OK) {
    Serial.println("CAN initialization FAILED");
    delay(1000);
  }

  Serial.println("CAN initialization OK");
  printHelp();
}

void loop() {
  printResponses();

  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }
}
