#ifndef PREDICTION_H
#define PREDICTION_H

#include "Data.h"

class Regression {
public:
    static PredictionResult calculateRegression(float nextX);
    static void processingTask(uint8_t msrmntIndex);

private:
    static uint16_t readLDR ();
};

#endif /* PREDICTION_H */
