#ifndef ENCODING_H // To make sure you don't declare the function more than once by including the header multiple times.
#define ENCODING_H

int encodeCurrentServos(int angle1, int angle2, int angle3, int angle4);
void decodeCurrentServos(int rep, int output[]);

#endif