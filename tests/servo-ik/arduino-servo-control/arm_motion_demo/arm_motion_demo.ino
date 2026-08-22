/*
  Arduino Uno R4 WiFi: six servos, three joysticks, differential wrist

  Direct servo signals:
    Joint 1 -> Servo 1 -> D3
    Joint 2 -> Servo 2 -> D5
    Joint 3 -> Servo 3 -> D6
    Joint 4 -> Servo 4 -> D9

  Differential servo signals:
    Servo 5 -> D10
    Servo 6 -> D11

  Joystick axes:
    Joystick 1 X -> A0 -> Joint 1
    Joystick 1 Y -> A1 -> Joint 2
    Joystick 2 X -> A2 -> Joint 3
    Joystick 2 Y -> A3 -> Joint 4
    Joystick 3 X -> A4 -> Differential axis A
    Joystick 3 Y -> A5 -> Differential axis B

  Joystick 1 SW -> D7 -> Pause/resume automatic motion
  Joystick 2 and 3 SW -> Not connected

  Differential mixing:
    Servo 5 = center + axis A + axis B
    Servo 6 = center + axis A - axis B

  Moving any joystick immediately enters manual mode. After 10 seconds with
  all sticks centered, the arm returns smoothly to neutral and restarts the
  automatic demo unless auto motion has been paused with Joystick 1 SW.
*/

#include <Servo.h>
#include <math.h>

const byte DIRECT_JOINT_COUNT = 4;
const byte SERVO_COUNT = 6;
const byte CONTROL_COUNT = 6;
const byte DIFF_AXIS_A = 4;
const byte DIFF_AXIS_B = 5;

const byte SERVO_PINS[SERVO_COUNT] = {3, 5, 6, 9, 10, 11};
const byte JOYSTICK_PINS[CONTROL_COUNT] = {A0, A1, A2, A3, A4, A5};
const byte PAUSE_BUTTON_PIN = 7;

// Change an entry from 1.0 to -1.0 if that joystick axis is backward.
const float JOYSTICK_DIRECTION[CONTROL_COUNT] = {
  1.0, -1.0, 1.0, -1.0, 1.0, -1.0
};

// Change an entry to -1.0 if a differential servo is mechanically reversed.
const float DIFFERENTIAL_SERVO_DIRECTION[2] = {1.0, 1.0};

const float PHYSICAL_SERVO_RANGE = 270.0;
const float DIRECT_MIN[DIRECT_JOINT_COUNT] = {0.0, 0.0, 90.0, 0.0};
const float DIRECT_MAX[DIRECT_JOINT_COUNT] = {270.0, 112.5, 270.0, 270.0};

const float DIFFERENTIAL_CENTER = 135.0;

// abs(axis A) + abs(axis B) cannot exceed this value. With a 135-degree
// center, 110 leaves a 25-degree safety margin at both servo endpoints.
const float DIFFERENTIAL_MAX_COMBINED = 110.0;

const unsigned long MOVE_TIME_MS = 4000;
const unsigned long PAUSE_TIME_MS = 500;
const unsigned long MANUAL_TIMEOUT_MS = 10000;
const unsigned long UPDATE_INTERVAL_MS = 20;
const unsigned long BUTTON_DEBOUNCE_MS = 40;

// Maximum manual speed at full joystick deflection, in physical degrees/sec.
const float MAX_MANUAL_SPEED = 45.0;

// The Uno R4 ADC is configured for 12 bits: readings are from 0 to 4095.
const int ADC_MAX = 4095;
const int JOYSTICK_DEADZONE = 300;

// Each pose is:
// {Joint 1, Joint 2, Joint 3, Joint 4, Diff axis A, Diff axis B}
const float POSES[][CONTROL_COUNT] = {
  //                                 Diff A  Diff B   Resulting S5 / S6
  {135.0, 55.0, 180.0, 135.0,    0.0,    0.0},  // 135° / 135°
  { 90.0, 35.0, 210.0, 100.0,  -40.0,   30.0},  // 125° /  65°
  {120.0, 80.0, 140.0, 180.0,   45.0,   25.0},  // 205° / 155°
  {180.0, 40.0, 205.0, 170.0,   30.0,  -45.0},  // 120° / 210°
  {150.0, 70.0, 155.0,  80.0,  -35.0,  -35.0}   //  65° / 135°
};

const byte POSE_COUNT = sizeof(POSES) / sizeof(POSES[0]);

Servo servos[SERVO_COUNT];
float currentControl[CONTROL_COUNT];
float autoStartControl[CONTROL_COUNT];
int joystickCenter[CONTROL_COUNT];

bool manualMode = false;
bool autoPaused = false;
bool autoMoving = true;
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
byte autoTargetPose = 1;
unsigned long autoPhaseStartedMs = 0;
unsigned long lastManualInputMs = 0;
unsigned long lastUpdateMs = 0;
unsigned long lastButtonChangeMs = 0;

// Explicit declarations keep the sketch clear and avoid relying on automatic
// function-prototype generation by the Arduino build system.
void calibrateJoysticks();
bool readJoysticks(float axisValues[]);
void updateManualMotion(const float axisValues[], float elapsedSeconds);
void constrainDifferential();
void updatePauseButton(unsigned long now);
void beginAutoMove(byte targetPose, unsigned long now);
void updateAutomaticMotion(unsigned long now);
void writeAllServoOutputs();
void writePhysicalServo(byte servoIndex, float physicalAngle);

void setup() {
  analogReadResolution(12);
  pinMode(PAUSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  for (byte servo = 0; servo < SERVO_COUNT; servo++) {
    servos[servo].attach(SERVO_PINS[servo]);
  }

  for (byte control = 0; control < CONTROL_COUNT; control++) {
    currentControl[control] = POSES[0][control];
    autoStartControl[control] = currentControl[control];
  }
  writeAllServoOutputs();

  // Keep all three sticks centered during this half-second calibration.
  calibrateJoysticks();

  autoPhaseStartedMs = millis();
  lastUpdateMs = autoPhaseStartedMs;
}

void loop() {
  unsigned long now = millis();
  updatePauseButton(now);

  if (now - lastUpdateMs < UPDATE_INTERVAL_MS) {
    return;
  }

  float elapsedSeconds = (now - lastUpdateMs) / 1000.0;
  lastUpdateMs = now;

  float axisValues[CONTROL_COUNT];
  bool joystickActive = readJoysticks(axisValues);

  if (joystickActive) {
    manualMode = true;
    lastManualInputMs = now;
    updateManualMotion(axisValues, elapsedSeconds);
    return;
  }

  if (manualMode) {
    if (now - lastManualInputMs >= MANUAL_TIMEOUT_MS) {
      manualMode = false;
      if (!autoPaused) {
        beginAutoMove(0, now);
      }
    }
    return;
  }

  if (autoPaused) {
    return;
  }

  updateAutomaticMotion(now);
}

void calibrateJoysticks() {
  const int SAMPLE_COUNT = 100;
  long totals[CONTROL_COUNT] = {0, 0, 0, 0, 0, 0};

  for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
    for (byte axis = 0; axis < CONTROL_COUNT; axis++) {
      totals[axis] += analogRead(JOYSTICK_PINS[axis]);
    }
    delay(5);
  }

  for (byte axis = 0; axis < CONTROL_COUNT; axis++) {
    joystickCenter[axis] = totals[axis] / SAMPLE_COUNT;
  }
}

bool readJoysticks(float axisValues[]) {
  bool anyActive = false;

  for (byte axis = 0; axis < CONTROL_COUNT; axis++) {
    int offset = analogRead(JOYSTICK_PINS[axis]) - joystickCenter[axis];
    int magnitude = abs(offset);

    if (magnitude <= JOYSTICK_DEADZONE) {
      axisValues[axis] = 0.0;
      continue;
    }

    anyActive = true;
    int availableTravel = offset > 0
                            ? ADC_MAX - joystickCenter[axis]
                            : joystickCenter[axis];
    int usableTravel = availableTravel - JOYSTICK_DEADZONE;
    if (usableTravel < 1) {
      usableTravel = 1;
    }

    float normalized = (float)(magnitude - JOYSTICK_DEADZONE)
                       / (float)usableTravel;
    normalized = constrain(normalized, 0.0, 1.0);

    axisValues[axis] = (offset > 0 ? normalized : -normalized)
                       * JOYSTICK_DIRECTION[axis];
  }

  return anyActive;
}

void updateManualMotion(const float axisValues[], float elapsedSeconds) {
  for (byte control = 0; control < CONTROL_COUNT; control++) {
    currentControl[control] += axisValues[control]
                               * MAX_MANUAL_SPEED
                               * elapsedSeconds;
  }

  for (byte joint = 0; joint < DIRECT_JOINT_COUNT; joint++) {
    currentControl[joint] = constrain(
      currentControl[joint],
      DIRECT_MIN[joint],
      DIRECT_MAX[joint]
    );
  }

  constrainDifferential();
  writeAllServoOutputs();
}

void constrainDifferential() {
  float combined = fabsf(currentControl[DIFF_AXIS_A])
                   + fabsf(currentControl[DIFF_AXIS_B]);

  if (combined > DIFFERENTIAL_MAX_COMBINED) {
    float scale = DIFFERENTIAL_MAX_COMBINED / combined;
    currentControl[DIFF_AXIS_A] *= scale;
    currentControl[DIFF_AXIS_B] *= scale;
  }
}

void updatePauseButton(unsigned long now) {
  bool reading = digitalRead(PAUSE_BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeMs = now;
  }

  if (now - lastButtonChangeMs < BUTTON_DEBOUNCE_MS ||
      reading == stableButtonState) {
    return;
  }

  stableButtonState = reading;
  if (stableButtonState != LOW) {
    return;
  }

  autoPaused = !autoPaused;
  digitalWrite(LED_BUILTIN, autoPaused ? HIGH : LOW);

  if (!autoPaused && !manualMode) {
    beginAutoMove(0, now);
  }
}

void beginAutoMove(byte targetPose, unsigned long now) {
  for (byte control = 0; control < CONTROL_COUNT; control++) {
    autoStartControl[control] = currentControl[control];
  }

  autoTargetPose = targetPose;
  autoMoving = true;
  autoPhaseStartedMs = now;
}

void updateAutomaticMotion(unsigned long now) {
  if (!autoMoving) {
    if (now - autoPhaseStartedMs >= PAUSE_TIME_MS) {
      byte nextPose = (autoTargetPose + 1) % POSE_COUNT;
      beginAutoMove(nextPose, now);
    }
    return;
  }

  unsigned long elapsed = now - autoPhaseStartedMs;
  float progress = constrain(
    (float)elapsed / (float)MOVE_TIME_MS,
    0.0,
    1.0
  );
  float eased = progress * progress * (3.0 - 2.0 * progress);

  for (byte control = 0; control < CONTROL_COUNT; control++) {
    currentControl[control] = autoStartControl[control]
                              + (POSES[autoTargetPose][control]
                                 - autoStartControl[control]) * eased;
  }

  constrainDifferential();
  writeAllServoOutputs();

  if (progress >= 1.0) {
    autoMoving = false;
    autoPhaseStartedMs = now;
  }
}

void writeAllServoOutputs() {
  for (byte joint = 0; joint < DIRECT_JOINT_COUNT; joint++) {
    writePhysicalServo(joint, currentControl[joint]);
  }

  float differentialA = currentControl[DIFF_AXIS_A];
  float differentialB = currentControl[DIFF_AXIS_B];

  float servo5Angle = DIFFERENTIAL_CENTER
                      + DIFFERENTIAL_SERVO_DIRECTION[0]
                        * (differentialA + differentialB);
  float servo6Angle = DIFFERENTIAL_CENTER
                      + DIFFERENTIAL_SERVO_DIRECTION[1]
                        * (differentialA - differentialB);

  writePhysicalServo(4, servo5Angle);
  writePhysicalServo(5, servo6Angle);
}

void writePhysicalServo(byte servoIndex, float physicalAngle) {
  physicalAngle = constrain(physicalAngle, 0.0, PHYSICAL_SERVO_RANGE);
  int servoCommand = round(physicalAngle * 180.0 / PHYSICAL_SERVO_RANGE);
  servoCommand = constrain(servoCommand, 0, 180);
  servos[servoIndex].write(servoCommand);
}
