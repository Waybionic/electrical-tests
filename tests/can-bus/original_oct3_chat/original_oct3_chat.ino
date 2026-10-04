#include "AA_MCP2515.h"

// =====================================================
// CAN SETUP
// =====================================================

const CANBitrate::Config CAN_BITRATE =
    CANBitrate::Config_8MHz_500kbps;

const uint8_t CAN_CS  = 10;
const int8_t  CAN_INT = 2;

CANConfig config(CAN_BITRATE, CAN_CS, CAN_INT);
CANController CAN(config);


// =====================================================
// MOTOR IDS
// =====================================================

const uint16_t J1 = 1;
const uint16_t J2 = 2;
const uint16_t J3 = 3;


// =====================================================
// CHECKSUM
// =====================================================

uint8_t checksum(uint16_t id, uint8_t *data, uint8_t len)
{
    uint16_t sum = id;

    for (uint8_t i = 0; i < len; i++)
        sum += data[i];

    return sum & 0xFF;
}


// =====================================================
// SEND COMMAND
// =====================================================

void sendMKS(uint16_t id, uint8_t *data, uint8_t len)
{
    CANFrame frame(id, data, len);

    CANController::IOResult result = CAN.write(frame);

    if (result != CANController::IOResult::OK)
    {
        Serial.print("CAN TX FAILED for ID ");
        Serial.println(id);
    }
}


// =====================================================
// READ RESPONSES
// =====================================================

void readCAN()
{
    CANFrame rx;

    while (CAN.read(rx) == CANController::IOResult::OK)
    {
        rx.print("MKS RX");
    }
}


// =====================================================
// ENABLE MOTOR
// =====================================================

void enableMotor(uint16_t id)
{
    uint8_t data[3];

    data[0] = 0xF3;
    data[1] = 0x01;
    data[2] = checksum(id, data, 2);

    sendMKS(id, data, 3);
}


// =====================================================
// DISABLE MOTOR
// =====================================================

void disableMotor(uint16_t id)
{
    uint8_t data[3];

    data[0] = 0xF3;
    data[1] = 0x00;
    data[2] = checksum(id, data, 2);

    sendMKS(id, data, 3);
}


// =====================================================
// SPEED MODE
//
// speed: 0 - 3000
// accel: 0 - 255
//
// ccw = false -> CW
// ccw = true  -> CCW
// =====================================================

void moveSpeed(uint16_t id,
               uint16_t speed,
               uint8_t accel,
               bool ccw)
{
    if (speed > 3000)
        speed = 3000;

    uint8_t data[5];

    data[0] = 0xF6;

    data[1] =
        (ccw ? 0x80 : 0x00) |
        ((speed >> 8) & 0x0F);

    data[2] = speed & 0xFF;

    data[3] = accel;

    data[4] = checksum(id, data, 4);

    sendMKS(id, data, 5);

    Serial.print("Motor ");
    Serial.print(id);
    Serial.print(" speed = ");
    Serial.println(speed);
}


// =====================================================
// STOP MOTOR
// =====================================================

void stopMotor(uint16_t id)
{
    uint8_t data[5];

    data[0] = 0xF6;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x02;

    data[4] = checksum(id, data, 4);

    sendMKS(id, data, 5);
}


// =====================================================
// EMERGENCY STOP
// =====================================================

void emergencyStop(uint16_t id)
{
    uint8_t data[2];

    data[0] = 0xF7;
    data[1] = checksum(id, data, 1);

    sendMKS(id, data, 2);
}


// =====================================================
// RELATIVE POSITION MOVE
// =====================================================

void moveRelative(uint16_t id,
                  uint32_t pulses,
                  uint16_t speed,
                  uint8_t accel,
                  bool ccw)
{
    if (speed > 3000)
        speed = 3000;

    if (pulses > 0xFFFFFF)
        pulses = 0xFFFFFF;

    uint8_t data[8];

    data[0] = 0xFD;

    data[1] =
        (ccw ? 0x80 : 0x00) |
        ((speed >> 8) & 0x0F);

    data[2] = speed & 0xFF;

    data[3] = accel;

    data[4] = (pulses >> 16) & 0xFF;
    data[5] = (pulses >> 8)  & 0xFF;
    data[6] = pulses & 0xFF;

    data[7] = checksum(id, data, 7);

    sendMKS(id, data, 8);

    Serial.print("Relative move ID ");
    Serial.print(id);
    Serial.print(" pulses = ");
    Serial.println(pulses);
}


// =====================================================
// QUERY MOTOR STATUS
// =====================================================

void queryMotor(uint16_t id)
{
    uint8_t data[2];

    data[0] = 0xF1;
    data[1] = checksum(id, data, 1);

    sendMKS(id, data, 2);
}


// =====================================================
// ALL-MOTOR HELPERS
// =====================================================

void enableAll()
{
    enableMotor(J1);
    delay(10);

    enableMotor(J2);
    delay(10);

    enableMotor(J3);
}


void stopAll()
{
    stopMotor(J1);
    delay(10);

    stopMotor(J2);
    delay(10);

    stopMotor(J3);

    Serial.println("ALL MOTORS STOPPED");
}


void eStopAll()
{
    emergencyStop(J1);
    delay(5);

    emergencyStop(J2);
    delay(5);

    emergencyStop(J3);

    Serial.println("!!! EMERGENCY STOP !!!");
}


// =====================================================
// SERIAL COMMAND HELP
// =====================================================

void printHelp()
{
    Serial.println();
    Serial.println("========== WAYBIONIC CAN TEST ==========");
    Serial.println();
    Serial.println("1 = J1 forward");
    Serial.println("2 = J2 forward");
    Serial.println("3 = J3 forward");
    Serial.println();
    Serial.println("4 = J1 reverse");
    Serial.println("5 = J2 reverse");
    Serial.println("6 = J3 reverse");
    Serial.println();
    Serial.println("a = all motors forward");
    Serial.println("r = relative-position demo");
    Serial.println("s = stop all");
    Serial.println("e = EMERGENCY STOP");
    Serial.println("q = query all motors");
    Serial.println("x = disable all motors");
    Serial.println("h = print this menu");
    Serial.println();
    Serial.println("========================================");
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(1500);

    Serial.println();
    Serial.println("Starting Waybionic CAN Controller...");

    while (CAN.begin(CANController::Mode::Normal)
           != CANController::OK)
    {
        Serial.println("CAN initialization FAILED");
        delay(1000);
    }

    Serial.println("CAN initialization OK");

    delay(500);

    enableAll();

    Serial.println("Motors enabled.");

    printHelp();
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
    readCAN();

    if (Serial.available())
    {
        char command = Serial.read();

        switch (command)
        {

            case '1':

                Serial.println("J1 forward");

                moveSpeed(J1, 200, 20, false);

                break;


            case '2':

                Serial.println("J2 forward");

                moveSpeed(J2, 200, 20, false);

                break;


            case '3':

                Serial.println("J3 forward");

                moveSpeed(J3, 200, 20, false);

                break;


            case '4':

                Serial.println("J1 reverse");

                moveSpeed(J1, 200, 20, true);

                break;


            case '5':

                Serial.println("J2 reverse");

                moveSpeed(J2, 200, 20, true);

                break;


            case '6':

                Serial.println("J3 reverse");

                moveSpeed(J3, 200, 20, true);

                break;


            case 'a':

                Serial.println("ALL MOTORS FORWARD");

                moveSpeed(J1, 200, 20, false);
                delay(5);

                moveSpeed(J2, 200, 20, false);
                delay(5);

                moveSpeed(J3, 200, 20, false);

                break;


            case 'r':

                Serial.println("Relative position demo");

                moveRelative(J1, 1000, 300, 20, false);

                delay(20);

                moveRelative(J2, 1000, 300, 20, false);

                delay(20);

                moveRelative(J3, 1000, 300, 20, false);

                break;


            case 's':

                stopAll();

                break;


            case 'e':

                eStopAll();

                break;


            case 'q':

                Serial.println("Querying motors...");

                queryMotor(J1);
                delay(20);

                queryMotor(J2);
                delay(20);

                queryMotor(J3);

                break;


            case 'x':

                disableMotor(J1);
                disableMotor(J2);
                disableMotor(J3);

                Serial.println("Motors disabled.");

                break;


            case 'h':

                printHelp();

                break;
        }
    }
}
