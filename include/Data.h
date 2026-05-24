#ifndef DATA_H
#define DATA_H

#include <Arduino.h>

struct sensorMeasure {
    uint16_t co2;
    float temp;
    float rh;
};

struct PredictionResult {
        float slope;
        float intercept;
        float nextValue;
};

#define WINDOW_SIZE 12

extern RTC_DATA_ATTR struct sensorMeasure measurements[WINDOW_SIZE];
extern RTC_DATA_ATTR uint8_t nCurrStoredMeasures;
extern RTC_DATA_ATTR uint8_t measurementIndex;

#endif /* DATA_H */
