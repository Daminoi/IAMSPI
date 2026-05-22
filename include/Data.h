#include <Arduino.h>

struct sensorMeasure {
    uint16_t co2;
    float temp;
    float rh;
};


#define WINDOW_SIZE 12
RTC_DATA_ATTR struct sensorMeasure measurements[WINDOW_SIZE];
RTC_DATA_ATTR uint8_t nCurrStoredMeasures = 0;
RTC_DATA_ATTR uint8_t measurementIndex = 0;