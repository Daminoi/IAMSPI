#include "Prediction.h"

// --- Configuration ---
#define SLOPE_THRESHOLD 5.0 // Threshold to take action if air quality degrades too quickly
int N = nCurrStoredMeasures;

// Calculates the OLS regression
static PredictionResult calculateRegression(float nextX) {
    float sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;

    for (int i = 0; i < N; i++)
    {
        sumX += measurements[i].co2;
        sumY += measurementIndex;
        sumXY += measurements[i].co2 * measurementIndex;
        sumXX += measurements[i].co2 * measurements[i].co2;
    }

    PredictionResult res = {0.0f, 0.0f, 0.0f};
    if (N < 2)
        return res; // Need at least two points to calculate a trend line

    float denominator = (N * sumXX) - (sumX * sumX);
    if (denominator != 0)
    {
        res.slope = ((N * sumXY) - (sumX * sumY)) / denominator;
        res.intercept = (sumY - (res.slope * sumX)) / N;
        res.nextValue = (res.slope * nextX) + res.intercept;
    }
    return res;
}

// --- FreeRTOS Task ---
void processingTask(sensorMeasure *measures) {
    PredictionResult trend = calculateRegression((float)WINDOW_SIZE);

    Serial.printf("Current: %.2f | Calculated Slope: %.4f | Predicted Next: %.2f\n",
                  measurements[WINDOW_SIZE - 1].co2, trend.slope, trend.nextValue);
    Serial.printf(">pred_co2:%d:%u|\n", measurementIndex++, trend.nextValue);

    // Act based on the trend evaluation
    if (nCurrStoredMeasures >= WINDOW_SIZE && trend.slope > SLOPE_THRESHOLD) {
        Serial.println("WARNING: Air quality degrading rapidly! Activating air filter...");
        // digitalWrite(GPIO_NUM_2, HIGH); // Turn on peripheral actuator (fan, alert LED, etc.)
        delay(1000); // Allow physical hardware action visibility before dropping power
    }

    // Teleplot Logging (Real vs Predicted)
    String currentPacket = "Air_Real:" + String(N) + ":" + String(measurements[N - 1].co2) + "|xy\n";
    String predictPacket = "Air_Trend:" + String(WINDOW_SIZE - 1) + ":" + String(trend.nextValue) + "|xy\n";

    vTaskDelete(NULL); // Fallback safety catch
}