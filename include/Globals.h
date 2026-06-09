#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <SensirionI2cScd4x.h>

#include "Configuration.h"
#include "pinDefinitions.h"

// DEBUGGING DEFS
// comment the following line to disable all features that are ONLY meant for debugging purposes
#define DEBUG_MODE_ACTIVE 1

// comment the following line to *NOT* use the HARDCODED CREDENTIALS
#define DEBUG_USE_EMBEDDED_CREDENTIALS 1

#endif /* GLOBALS_H */