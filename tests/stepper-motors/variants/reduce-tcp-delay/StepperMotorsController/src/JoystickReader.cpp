#include <Arduino.h>
#include <JoystickReader.h>

JoystickReader::JoystickReader(int startingAngle, int maxAngle, bool flipped)
{
    this->currentAngle = startingAngle;
    this->maxAngle = maxAngle;
    this->signMul = flipped ? -1 : 1; // Set the sign multiplier based on the flipped state
}

void JoystickReader::setUp(uint8_t joyStickAnalogPin)
{
    this->joyStickAnalogPin = joyStickAnalogPin;
    this->baseValue = analogRead(joyStickAnalogPin);
    this->lastJoystickValue = baseValue; // Initialize lastJoystickValue to the base value
}

int JoystickReader::getUpdatedCurrentAngle()
{
    double increment = getIncrementFromJoystick();
    double newAngle = currentAngle + increment;
    currentAngle = constrain(newAngle, 0, maxAngle);
    return floor(currentAngle);
}

double JoystickReader::getIncrementFromJoystick()
{
    int joystickValue = analogRead(joyStickAnalogPin);
    if (abs(joystickValue - baseValue) < MINIMUM_DETECTABLE_ANGLE)
    {
        return 0; // Ignore small changes
    }
    double increment = (joystickValue - baseValue) * signMul; // Apply the sign multiplier
    double div = 1000;
    double seconds = (UPDATE_DELAY_MILLIS / div);
    return (double)map(increment, -JOYSTICK_MAX, JOYSTICK_MAX, -MAX_INCREMENT, MAX_INCREMENT) * seconds;
}