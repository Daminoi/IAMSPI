#include "LightsOn.h"

// Constants for the pins where sensors are plugged into.
const int ledPin = 2;

// Filtering and occupancy constants.
const float alpha = 0.30f;            // exponential moving average weight
// A higher alpha (e.g., 0.5) makes the filter respond faster to changes, giving more weight to recent readings.
// A lower alpha (e.g., 0.1) makes it respond slower, averaging out noise but delaying detection of sudden changes like lights turning off.
const int sampleIntervalMs = 500;     // read sensor every 500 ms
const int lightOnThresholdDelta = 150; // threshold above baseline for "lights on"
const int lightOffThresholdDelta = 50; // lower threshold for hysteresis

int lightInit = 0;          // initial baseline reading
float lightFiltered = 0;    // smoothed sensor value
bool lightsOn = false;      // current interpreted state
unsigned long lastSampleMs = 0;

// Gets Initial Baseline for photoresistor readings as an avg over 3 500ms intervals.
void setIlluminationBaseline() {
  uint16_t sum = 0;
  for (int i = 0; i < 3; i++) {
    uint16_t rawValue = readLDR();
    sum += (alpha * rawValue + (1.0f - alpha) * lightFiltered);
    delay(500);
  }
  lightInit = sum / 3;
  lightFiltered = lightInit;

  Serial.print("Initial baseline: ");
  Serial.println(lightInit);
}

bool getIllumination() {
  // unsigned long now = millis();
  // if (now - lastSampleMs < sampleIntervalMs) {
  //   return lightsOn; // Not time to sample yet, return current state
  // }
  // lastSampleMs = now;

  uint16_t rawValue = readLDR();

  // Apply exponential smoothing to reduce jitter.
  lightFiltered = alpha * rawValue + (1.0f - alpha) * lightFiltered;

  // Use hysteresis logic so state only changes on sustained values.
  lightsOn = lightFiltered > lightInit + lightOnThresholdDelta;

  Serial.printf("LIGHT: raw= %f, filtered= %f, state= %s\n", (float)rawValue, lightFiltered, lightsOn ? "ON" : "OFF");
  return lightsOn;
}

uint16_t readLDR () {
  digitalWrite(LDR_POWER, HIGH); // Power the LDR circuit
    delay(100); // Short delay to allow the sensor to stabilize after powering it on
    uint16_t rawValue = analogRead(LDR_INPUT);
    digitalWrite(LDR_POWER, LOW); // Power down the LDR circuit to save energy
    return rawValue;
}
// notes:
// value at 3:30PM is pretty high (on avg 3984)
// with torch: 4095
// inside class: 
//    with lights off: 2502
//    with lights on: 
//        1 Light far off: 2592 - 2608
//        2 Lights closer: 2930 - 3000



// Added an exponential moving average (lightFiltered) to smooth raw ADC readings.
// Added hysteresis using two thresholds:
// lightOnThresholdDelta
// lightOffThresholdDelta
// Added a timed sampling interval (sampleIntervalMs = 150) instead of reading continuously.
// Exponential smoothing reduces jitter from small ADC noise and flicker, without requiring large memory.
// Hysteresis prevents the interpreted state from bouncing when the reading sits near the threshold.
// Timed sampling avoids reacting to every tiny transient and gives the sensor a stable cadence.

// What your circuit should look like
// Photoresistor sensor circuit
// Use a voltage divider with the photoresistor and a fixed resistor.

// Connect one end of the photoresistor to 3.3V
// Connect the other end of the photoresistor to GPIO7 / ADC6
// At that same GPIO36 node, connect a fixed resistor (10kΩ is a good start) to GND

// That means:

// analogRead(36) reads the voltage at the junction
// more light → lower LDR resistance → higher voltage at ADC36
// less light → higher LDR resistance → lower voltage at ADC36


// Important points
// Use 3.3V, not 5V, for the sensor on ESP32 ADC
// Ground must be common for the sensor and LED circuits
// GPIO36 is a proper ADC pin on ESP32
// GPIO39 is not a valid output pin, so change ledPin to a real digital pin in code
// Recommended wiring
// sensorPin = 36
// ledPin = 2 (or another output-capable GPIO)
// 10kΩ fixed resistor between ADC pin and GND
// 3.3V → LDR → ADC pin → resistor → GND