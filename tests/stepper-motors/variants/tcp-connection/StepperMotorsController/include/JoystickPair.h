#include "Arduino.h"
#include "JoystickReader.h"

class JoystickPair
{
private:
    JoystickReader *joystick1;
    JoystickReader *joystick2;

public:
    JoystickPair(JoystickReader *joystick1, JoystickReader *joystick2)
    {
        this->joystick1 = joystick1;
        this->joystick2 = joystick2;
    }

    // Update both joysticks and return their current angles
    // Favors the joystick with the larger increment, setting the other to zero
    void updateBothJoysticksMax(unsigned long currentMillis, uint8_t *startIdx);
};