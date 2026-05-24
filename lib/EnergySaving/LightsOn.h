#ifndef LIGHTSON_H
#define LIGHTSON_H

#include <Arduino.h>
#include <Wire.h>

#include "Configuration.h"
#include "pinDefinitions.h"

class LDR {
public:
    // Initializes the Baseline which acts as the value for "Darkness".
    static void setIlluminationBaseline();
    static bool getIllumination();

private:
    static uint16_t readLDR ();
};

#endif /* LIGHTSON_H */
