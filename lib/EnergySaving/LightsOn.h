#ifndef LIGHTSON_H
#define LIGHTSON_H

#include <Arduino.h>
#include <Wire.h>

#include "Configuration.h"
#include "PinDefinitions.h"

void setIlluminationBaseline();
bool getIllumination();
uint16_t readLDR ();

#endif /* LIGHTSON_H */
