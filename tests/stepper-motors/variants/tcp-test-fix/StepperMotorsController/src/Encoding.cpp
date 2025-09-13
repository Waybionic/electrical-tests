#include "Encoding.h"

int encodeCurrentServos(int angle1, int angle2, int angle3, int angle4)
{
    int encoded = 0;
    encoded |= (angle1 & 0xFF) << 24;
    encoded |= (angle2 & 0xFF) << 16;
    encoded |= (angle3 & 0xFF) << 8;
    encoded |= (angle4 & 0xFF);
    return encoded;
}

void decodeCurrentServos(int rep, int output[])
{
    output[0] = (rep >> 24) & 0xFF; // Extract angle1
    output[1] = (rep >> 16) & 0xFF; // Extract angle2
    output[2] = (rep >> 8) & 0xFF;  // Extract angle3
    output[3] = rep & 0xFF;         // Extract angle4
}
