#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Globals.h>

// Configuration & Constants
#define U_S_TO_S_FACTOR 1000000ULL
#define DEEP_SLEEP_SECONDS 300 

// Thresholds
const uint16_t CO2_VERY_HIGH = 4000;
const uint16_t CO2_HIGH      = 1200;
const uint16_t CO2_MEDIUM    = 800;
const uint16_t CO2_ERROR     = 50;
const float TEMP_HIGH        = 26;             
const float TEMP_LOW         = 18;
const float TEMP_ERROR       = 2;
const float RH_HIGH          = 60;
const float RH_LOW           = 40;
const float RH_ERROR         = 5;

#endif /* CONFIGURATION_H */
