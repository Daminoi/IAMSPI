#ifndef PREDICTION_H
#define PREDICTION_H

#include "Data.h"

class Regression {
public:
    static PredictionResult calculateRegression(float nextX);
    static void processingTask();

private:
    static uint16_t readLDR ();
};

#endif /* PREDICTION_H */
