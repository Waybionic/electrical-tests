#include "AA_MCP2515.h"

// EXPERIMENTAL: the three-controller console and the F1/F7/FD paths require
// another hardware validation pass. No command is sent automatically at boot.
const CANBitrate::Config CAN_BITRATE =
    CANBitrate::Config_8MHz_500kbps;
const uint8_t CAN_CS = 10;
const int8_t CAN_INT = 2;

CANConfig config(CAN_BITRATE, CAN_CS, CAN_INT);
CANController CAN(config);

const uint16_t J1 = 1;
const uint16_t J2 = 2;
const uint16_t J3 = 3;

const uint16_t DEFAULT_SPEED = 320;
const uint8_t DEFAULT_ACCEL = 2;

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

void setMotorEnabled(uint16_t id, bool enabled) {
  uint8_t data[] = {0xF3, enabled ? 0x01 : 0x00, 0x00};
  data[2] = checksum(id, data, 2);
  sendFrame(id, data, sizeof(data));
}

void moveSpeed(uint16_t id, uint16_t speed, uint8_t accel, bool ccw) {
  if (speed > 3000) {
    speed = 3000;
  }

  uint8_t data[] = {
      0xF6,
      static_cast<uint8_t>((ccw ? 0x80 : 0x00) | ((speed >> 8) & 0x0F)),
      static_cast<uint8_t>(speed & 0xFF),
      accel,
      0x00,
  };
  data[4] = checksum(id, data, 4);
  sendFrame(id, data, sizeof(data));
}

void stopMotor(uint16_t id) {
  moveSpeed(id, 0, DEFAULT_ACCEL, false);
}

void emergencyStop(uint16_t id) {
  uint8_t data[] = {0xF7, 0x00};
  data[1] = checksum(id, data, 1);
  sendFrame(id, data, sizeof(data));
}

void moveRelative(uint16_t id, uint32_t pulses, uint16_t speed,
                  uint8_t accel, bool ccw) {
  if (speed > 3000) {
    speed = 3000;
  }
  if (pulses > 0xFFFFFF) {
    pulses = 0xFFFFFF;
  }

  uint8_t data[] = {
      0xFD,
      static_cast<uint8_t>((ccw ? 0x80 : 0x00) | ((speed >> 8) & 0x0F)),
      static_cast<uint8_t>(speed & 0xFF),
      accel,
      static_cast<uint8_t>((pulses >> 16) & 0xFF),
      static_cast<uint8_t>((pulses >> 8) & 0xFF),
      static_cast<uint8_t>(pulses & 0xFF),
      0x00,
  };
  data[7] = checksum(id, data, 7);
  sendFrame(id, data, sizeof(data));
}

void queryMotor(uint16_t id) {
  uint8_t data[] = {0xF1, 0x00};
  data[1] = checksum(id, data, 1);
  sendFrame(id, data, sizeof(data));
}

void forEachMotor(void (*action)(uint16_t)) {
  action(J1);
  action(J2);
  action(J3);
}

void enableOne(uint16_t id) { setMotorEnabled(id, true); }
void disableOne(uint16_t id) { setMotorEnabled(id, false); }
void stopOne(uint16_t id) { stopMotor(id); }
void emergencyStopOne(uint16_t id) { emergencyStop(id); }
void queryOne(uint16_t id) { queryMotor(id); }

void printHelp() {
  Serial.println();
  Serial.println("Experimental MKS three-joint console:");
  Serial.println("  enable       Enable J1, J2, and J3");
  Serial.println("  disable      Disable J1, J2, and J3");
  Serial.println("  j1 cw        Run J1 clockwise");
  Serial.println("  j1 ccw       Run J1 counter-clockwise");
  Serial.println("  j2 cw|ccw    Run J2");
  Serial.println("  j3 cw|ccw    Run J3");
  Serial.println("  all cw|ccw   Run all three motors");
  Serial.println("  relative     Move each motor +1000 pulses");
  Serial.println("  stop         Normal stop for all motors");
  Serial.println("  estop        Emergency stop for all motors");
  Serial.println("  query        Query all three motor IDs");
  Serial.println("  help         Show this list");
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "enable") {
    forEachMotor(enableOne);
  } else if (command == "disable") {
    forEachMotor(disableOne);
  } else if (command == "j1 cw") {
    moveSpeed(J1, DEFAULT_SPEED, DEFAULT_ACCEL, false);
  } else if (command == "j1 ccw") {
    moveSpeed(J1, DEFAULT_SPEED, DEFAULT_ACCEL, true);
  } else if (command == "j2 cw") {
    moveSpeed(J2, DEFAULT_SPEED, DEFAULT_ACCEL, false);
  } else if (command == "j2 ccw") {
    moveSpeed(J2, DEFAULT_SPEED, DEFAULT_ACCEL, true);
  } else if (command == "j3 cw") {
    moveSpeed(J3, DEFAULT_SPEED, DEFAULT_ACCEL, false);
  } else if (command == "j3 ccw") {
    moveSpeed(J3, DEFAULT_SPEED, DEFAULT_ACCEL, true);
  } else if (command == "all cw" || command == "all ccw") {
    const bool ccw = command.endsWith("ccw");
    moveSpeed(J1, DEFAULT_SPEED, DEFAULT_ACCEL, ccw);
    moveSpeed(J2, DEFAULT_SPEED, DEFAULT_ACCEL, ccw);
    moveSpeed(J3, DEFAULT_SPEED, DEFAULT_ACCEL, ccw);
  } else if (command == "relative") {
    moveRelative(J1, 1000, 300, 20, false);
    moveRelative(J2, 1000, 300, 20, false);
    moveRelative(J3, 1000, 300, 20, false);
  } else if (command == "stop") {
    forEachMotor(stopOne);
  } else if (command == "estop") {
    forEachMotor(emergencyStopOne);
  } else if (command == "query") {
    forEachMotor(queryOne);
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
  Serial.println("EXPERIMENTAL: verify IDs 1, 2, and 3 before moving motors.");
  printHelp();
}

void loop() {
  printResponses();

  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }
}
