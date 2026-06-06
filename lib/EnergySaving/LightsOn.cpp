#include "LightsOn.h"

// Constants for the pins where sensors are plugged into.
// const int ledPin = 2;

// Filtering and occupancy constants.
const float alpha = 0.30f;            // exponential moving average weight
// A higher alpha (e.g., 0.5) makes the filter respond faster to changes, giving more weight to recent readings.
// A lower alpha (e.g., 0.1) makes it respond slower, averaging out noise but delaying detection of sudden changes like lights turning off.
// const int sampleIntervalMs = 500;     // read sensor every 500 ms
const int lightOnThresholdDelta = 150; // threshold above baseline for "lights on"
const int lightOffThresholdDelta = 50; // lower threshold for hysteresis

RTC_DATA_ATTR uint16_t lightInit = 0; // initial baseline reading
RTC_DATA_ATTR float lightFiltered = 0; // initial baseline reading

bool lightsOn = false;      // current interpreted state
// unsigned long lastSampleMs = 0;

// Gets Initial Baseline for photoresistor readings as an avg over 3 500ms intervals.
void LDR::setIlluminationBaseline() {
    if (lightInit != 0) return;
  	uint16_t sum = 0;

  	for (int i = 0; i < 3; i++) {
    	sum += readLDR();
    	delay(500);
  	}

  	lightInit = sum / 3;

  	lightFiltered = lightInit;

  	Serial.print("Initial baseline: ");
  	Serial.println(lightInit);
}

bool LDR::getIllumination() {
  	// Let the filter process a quick burst of 4-5 readings to catch up to reality
  	uint16_t rawValue = 0;
  	for(int i = 0; i < 5; i++) {
      	rawValue = readLDR();
      	// Apply exponential smoothing to reduce jitter.
      	lightFiltered = alpha * rawValue + (1.0f - alpha) * lightFiltered;
      	delay(50); // Small delay to let the sensor settle
  	}

  	// Use hysteresis logic so state only changes on sustained values.
  	lightsOn = lightFiltered > lightInit + lightOnThresholdDelta;


  	Serial.printf("LIGHT: raw= %f, filtered= %f, state= %s\n", (float)rawValue, lightFiltered, lightsOn ? "ON" : "OFF");
  	Serial.printf("LightInit: %f, Threshold: %f\n", (float)lightInit, (float)lightInit + lightOnThresholdDelta);
  	
	return lightsOn;
}

uint16_t LDR::readLDR () {
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

// Connect one end of the photoresistor to 3.3V
// Connect the other end of the photoresistor to GPIO7 / ADC6
// At that same GPIO36 node, connect a fixed resistor to GND

// That means:

// analogRead(36) reads the voltage at the junction
// more light → lower LDR resistance → higher voltage
// less light → higher LDR resistance → lower voltage
