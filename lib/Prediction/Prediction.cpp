#include "Prediction.h"
#include "Configuration.h"

// --- Configuration ---
// Threshold to take action if air quality degrades too quickly
// A 5% rise in CO2 
#define SLOPE_THRESHOLD 0.05
int N = nCurrStoredMeasures;

// Calculates the OLS regression
PredictionResult Regression::calculateRegression(float nextX) {
    int N = nCurrStoredMeasures;
    PredictionResult res = {0.0f, 0.0f, 0.0f};
    if (N < 2) return res;

    float slope = 0;
    float intercept = 0;
    float avgX = 0, avgY = 0;

    float minVal = measurements[0].co2;
    float maxVal = measurements[0].co2;

    for (int i = 0; i < N; i++) {
        float currentX = (float)i;
        float currentY = (float)measurements[i].co2;
        avgX += currentX;
        avgY += currentY;
        if (measurements[i].co2 < minVal) minVal = measurements[i].co2;
        if (measurements[i].co2 > maxVal) maxVal = measurements[i].co2;
    }

    // When the CO2 reading flatlines, denom is very small making the slope very high.
    // This causes spikes in the predicted CO2 readings even from the smallest changes, which is not desirable.
    // To mitigate this issue, if the total data spread is less than 5 ppm, we treat it as a flatline.
    if ((maxVal - minVal) < 5.0f || (minVal - maxVal) < 5.0f) {
        res.slope = 0.0f;
        res.intercept = measurements[N-1].co2;
        res.nextValue = measurements[N-1].co2; // Prediction matches current stable value
        return res;
    }

    // Otherwise, we use linear regression as normal
    avgX /= N;
    avgY /= N;
    float numerator = 0;
    float denominator = 0;
    for (int i = 0; i < N - 1; i += 2) {
        float currentX = (float)i;
        float currentY = (float)measurements[i].co2;
        numerator += (currentX - avgX) * (currentY - avgY);
        denominator += (currentX - avgX) * (currentX - avgX);
    }
    if (denominator == 0) denominator += 1;
    slope = numerator / denominator;
    intercept = avgY - (slope * avgX);

    res.slope = slope;
    res.intercept = intercept;
    res.nextValue = (slope * nextX) + intercept;
    Serial.printf ("Slope: %.4f, Intercept: %.2f, Next Predicted CO2: %.2f\n", slope, intercept, res.nextValue);
    return res;
}

// --- FreeRTOS Task ---
void Regression::processingTask(uint8_t msrmntIndex) {
    PredictionResult trend = calculateRegression((float)msrmntIndex);

    Serial.printf("Current: %.2f | Calculated Slope: %.4f | Predicted Next: %.2f\n",
                  measurements[WINDOW_SIZE - 1].co2, trend.slope, trend.nextValue);
    Serial.printf(">pred_co2:%d:%.2f|\n", msrmntIndex, trend.nextValue);

    // Act based on the trend evaluation
    if (nCurrStoredMeasures >= WINDOW_SIZE && trend.nextValue > CO2_HIGH) {
        Serial.println("WARNING: Air quality degrading rapidly! Activating air filter...");
        // digitalWrite(GPIO_NUM_2, HIGH); // Turn on peripheral actuator (fan, alert LED, etc.)
        delay(1000); // Allow physical hardware action visibility before dropping power
    }

    vTaskDelete(NULL);
}