#include <JoystickPair.h>

void JoystickPair::updateBothJoysticksMax(unsigned long currentMillis, uint8_t *startIdx)
{
    double increment1 = joystick1->getIncrementFromJoystick(currentMillis);
    double increment2 = joystick2->getIncrementFromJoystick(currentMillis);

    if (abs(increment1) > abs(increment2))
    {
        increment2 = 0;
    }
    else
    {
        increment1 = 0;
    }

    uint8_t angle1 = joystick1->setUpdatedCurrentAngle(increment1);
    uint8_t angle2 = joystick2->setUpdatedCurrentAngle(increment2);

    startIdx[0] = angle1;
    startIdx[1] = angle2;
}