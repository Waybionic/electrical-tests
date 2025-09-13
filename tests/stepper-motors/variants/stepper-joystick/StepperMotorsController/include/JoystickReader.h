#include "Arduino.h"

const int JOYSTICK_MAX = 521;
const int MAX_INCREMENT = 200;
const int MINIMUM_DETECTABLE_ANGLE = 10;
const int UPDATE_DELAY_MILLIS = 10; // Delay for joystick updates

class JoystickReader
{
private:
    uint8_t joyStickAnalogPin;
    double currentAngle;
    int maxAngle;
    int signMul;
    int lastJoystickValue = 0; // last read value from joystick X
    int baseValue = 0;

    // Get the increment based on the joystick position
    // Returns a value between -MAX_INCREMENT and MAX_INCREMENT
    // If the change is less than MINIMUM_DETECTABLE_ANGLE, it ignores the change
    double getIncrementFromJoystick();

public:
    JoystickReader(int startingAngle, int maxAngle, bool flipped);

    // Set up the joystick servo with the given servo and joystick pin
    // Initializes the servo to the straight angle and reads the base value from the joystick
    void setUp(uint8_t joyStickAnalogPin);

    // Update the servo angle based on the increment
    // Ensures the angle is constrained between 0 and 180 degrees
    int getUpdatedCurrentAngle();
};