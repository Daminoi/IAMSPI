#ifndef PREDICTION_H
#define PREDICTION_H

#include "Data.h"

struct PredictionResult {
        float slope;
        float intercept;
        float nextValue;
};

static PredictionResult calculateRegression(float nextX);
void processingTask(sensorMeasure *measures);

#endif /* PREDICTION_H */
