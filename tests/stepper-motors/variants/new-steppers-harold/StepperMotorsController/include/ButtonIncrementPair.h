#include <Arduino.h>

struct ButtonIncrementPair
{
    int currentAngle;
    const uint8_t *buttons;
    bool *buttonWasPressed;
};

// Function to process button steps and update the current angle
// This function should be called in the loop to check button states
void processButtonStep(ButtonIncrementPair *pair);