#include <ButtonIncrementPair.h>
#include <Arduino.h>

#define INCREMENT 10

void processButtonStep(ButtonIncrementPair *pair)
{
    for (size_t i = 0; i < 2; ++i)
    {
        bool currentState = digitalRead(pair->buttons[i]);
        if (currentState && !pair->buttonWasPressed[i])
        {
            int increment = (i == 0) ? INCREMENT : -INCREMENT; // Adjust increment based on button
            pair->currentAngle = constrain(pair->currentAngle + increment, 0, 180);
        }
        pair->buttonWasPressed[i] = currentState;
    }
}