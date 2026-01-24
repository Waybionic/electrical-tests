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
    this->lastUpdateMillis = millis();
}

uint8_t JoystickReader::setUpdatedCurrentAngle(double increment)
{
    double newAngle = currentAngle + increment;
    currentAngle = constrain(newAngle, 0.0, (double)maxAngle);

    return (uint8_t)round(currentAngle);
}

double JoystickReader::getIncrementFromJoystick(unsigned long milliseconds)
{
    const int joystickValue = analogRead(joyStickAnalogPin);
    const int rawDelta = joystickValue - baseValue;
    const unsigned long deltaMillis = (milliseconds - lastUpdateMillis);
    lastUpdateMillis = milliseconds;

    // Dead zone in ADC counts
    if (abs(rawDelta) < MINIMUM_DETECTABLE_ANGLE)
    {
        return 0.0;
    }

    const double deltaSeconds = (deltaMillis) / 1000.0;
    int clamped = constrain(rawDelta, -JOYSTICK_MAX, JOYSTICK_MAX);

    // Apply direction flip if needed
    clamped *= signMul;

    const double scaledClamp = (double)clamped / (double)JOYSTICK_MAX; // Scale to -1.0 to 1.0
    const double rate = scaledClamp * MAX_INCREMENT_PER_S;

    // Convert rate to angle increment given elapsed time
    return rate * deltaSeconds;
}
