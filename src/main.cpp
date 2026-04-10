#include <Arduino.h>
#include <RTOS.h>
#include <Wire.h>

#include "../include/pinDefinitions.h"
#include "../lib/humanInteraction/humanInteraction.h"

#include "../.pio/libdeps/heltec_wifi_lora_32_V3/Sensirion I2C SCD4x/src/SensirionI2cScd4x.h"

#define MICROSECONDS_TO_SECONDS_FACTOR	1000000ULL

#ifdef SCD41_NO_ERROR
	#undef SCD41_NO_ERROR
#endif
#define SCD41_NO_ERROR 0

// We have chosen this sampling intervals as a good enough compromise between system reactivity and energy saving performance
// given the power usage data from Sensirion's SCD41 datasheet.
// When CO2 concentration is low to medium (we aim at 5 minute intervals)
#define SCD41_RELAXED_TARGET_SAMPLING_INTERVAL_SECONDS	300
// When CO2 concentration is very high	   (we aim at 2 minute intervals)
#define SCD41_ALERT_TARGET_SAMPLING_INTERVAL_SECONDS	120

// TODO this values below are hardcoded now, needs change
#define VERY_HIGH_CO2_THRESHOLD 4000
#define HIGH_CO2_THRESHOLD		1200
#define MEDIUM_CO2_THRESHOLD	800
#define HIGH_TEMPERATURE	26
#define LOW_TEMPERATURE		18
#define HIGH_RH				60
#define LOW_RH				40

#define MAX_STORED_MEASUREMENTS	12

// Size of a measure struct in ram is 32 bits
struct sensorMeasure{
	uint16_t 	co2;
	float 		temp;
	float 		rh;
};

// Here are declared all variables that need to maintain their data after deep sleep.
RTC_DATA_ATTR uint8_t power_on_after_full_shutdown 	= 1;

RTC_DATA_ATTR uint32_t current_sampling_interval 	= SCD41_RELAXED_TARGET_SAMPLING_INTERVAL_SECONDS;

RTC_DATA_ATTR struct sensorMeasure measurements[MAX_STORED_MEASUREMENTS];
RTC_DATA_ATTR uint8_t nCurrStoredMeasures = 0;

RTC_DATA_ATTR uint8_t previous_measurement_resulted_in_alarm = 0;


void setup() {
	setCpuFrequencyMhz(240);

	pinMode(GREEN_LED, OUTPUT);
	pinMode(YELLOW_LED, OUTPUT);
	pinMode(RED_LED, OUTPUT);

	pinMode(STATUS_RLED, OUTPUT);
	pinMode(STATUS_GLED, OUTPUT);
	pinMode(STATUS_BLED, OUTPUT);

	pinMode(BUZZERINO, OUTPUT);
	
	// This puts the GPIO VE_ENABLE, that is GPIO 36 in Heltec V3.2, in OUTPUT mode so that is can later be used to power the SCD41
	// LET'S *NOT* USE THIS PIN FOR ANYTHING ELSE, OR WE MAY DAMAGE THE SCD41
	pinMode(VE_ENABLE, OUTPUT);

	// Serial is only used for debugging
	Serial.begin(115200);
	setLEDStatusRED();
	while(!Serial){			// Wait for the serial connection to complete, if the status LED remains RED we can immediately tell something is wrong with the serial
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	setLEDStatusOFF();

	if(power_on_after_full_shutdown == 1)
	{
		// Self check and initialization procedure after a complete power off event.

		startup_welcome();

		// Activating power to the SCD41, this GPIO SHOULD keep this state after Deep Sleep
		digitalWrite(VE_ENABLE, LOW);	// We set it low so that it outputs 3.3V, inverted logic


		Serial.println("First power on after full shutdown completed.");
	}
	else{
		Serial.println("Waking up from Deep Sleep");
	}

	Wire.begin(SDA_GPIO, SCL_GPIO);	// Has to be repeated after every wake up from deep sleep, enables I2C on the specified GPIOs

	SensirionI2cScd4x scd41Sensor;
	int16_t scd41Error;
	char errorMessage[64];
	
	scd41Sensor.begin(Wire, SCD41_I2C_ADDR_62);
	
	if(power_on_after_full_shutdown == 1)
	{
		uint64_t scd41SerialNumber;
		
		setLEDStatusRED();
		scd41Error = scd41Sensor.wakeUp();
		if(scd41Error != SCD41_NO_ERROR)
		{
			Serial.println("Error while trying to wake up the SCD41!");
			unrecoverableErrorStatus();
		}
		setLEDStatusOFF();

		// Asking the SCD41 to provide its serial number is one way to check if it is in idle state
		setLEDStatusRED();
		scd41Error = scd41Sensor.getSerialNumber(scd41SerialNumber);
		if (scd41Error != SCD41_NO_ERROR)
		{
			Serial.println("Failure to obtain the SCD41 serial number!");
			errorToString(scd41Error, errorMessage, sizeof(errorMessage));
			Serial.println(errorMessage);
			unrecoverableErrorStatus();
		}
		setLEDStatusOFF();

		Serial.print("SCD41's serial number: 0x");
    	Serial.print((uint32_t)(scd41SerialNumber >> 32), HEX);
    	Serial.println((uint32_t)(scd41SerialNumber & 0xFFFFFFFF), HEX);

		Serial.println("Power on procedure completed with SUCCESS!");

		setLEDStatusGREEN();
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		setLEDStatusOFF();
	}

	// Sampling (Ask the sensor to sample -> Wait for the data -> Get the data -> Save it in RTC RAM)

	uint16_t lastCo2;
	float lastTemp;
	float lastRh;

	uint8_t alertOrPreAlertConditionsMet = 0;

	if(power_on_after_full_shutdown == 0){
		scd41Sensor.wakeUp();
	}

	scd41Error = scd41Sensor.measureSingleShot();
	if (scd41Error != SCD41_NO_ERROR)
	{
		Serial.println("Failure to ask the sensor for a single shot measurement!");
		errorToString(scd41Error, errorMessage, sizeof(errorMessage));
		Serial.println(errorMessage);
		unrecoverableErrorStatus();
	}

	setLEDStatusGREEN();	// The green status LED will stay on while the board is in light sleep, only for debugging purposes
	/*
		In the datasheet it is reported that the time necessary to take a sample is 5000 ms (5 seconds) maximum.
		Therefore we can save power by going in LIGHT SLEEP mode while waiting for the SCD41 to take a sample.
	*/
	esp_sleep_enable_timer_wakeup(5 * MICROSECONDS_TO_SECONDS_FACTOR);
	Serial.println("Entering Light Sleep while waiting for measurement");
	esp_light_sleep_start();

	setLEDStatusOFF();

	setLEDStatusBLUE();
	scd41Error = scd41Sensor.readMeasurement(lastCo2, lastTemp, lastRh);
	if (scd41Error != SCD41_NO_ERROR)
	{
		Serial.println("Error while reading the values from a measurement, maybe the sensor wasn't ready.");
		errorToString(scd41Error, errorMessage, sizeof(errorMessage));
		Serial.println(errorMessage);

		setLEDStatusGREEN();	// The green status LED will stay on while the board is in light sleep, only for debugging purposes
		esp_sleep_enable_timer_wakeup(3 * MICROSECONDS_TO_SECONDS_FACTOR);
		Serial.println("Entering Light Sleep while waiting once more for the sensor's data");
		esp_light_sleep_start();
		setLEDStatusOFF();

		scd41Error = scd41Sensor.readMeasurement(lastCo2, lastTemp, lastRh);
		if (scd41Error != SCD41_NO_ERROR)
		{
			Serial.println("Second attempt at reading the measurement failed!");
			errorToString(scd41Error, errorMessage, sizeof(errorMessage));
			Serial.println(errorMessage);
			unrecoverableErrorStatus();
		}
	}
	setLEDStatusOFF();

	scd41Sensor.powerDown();

	// For debugging
	Serial.print(lastCo2);
	Serial.print("\t");
	Serial.print(lastTemp);
	Serial.print("\t");
	Serial.println(lastRh);
	
	measurements[nCurrStoredMeasures].co2 	= lastCo2;
	measurements[nCurrStoredMeasures].temp 	= lastTemp;
	measurements[nCurrStoredMeasures].rh 	= lastRh;
	nCurrStoredMeasures++;

	// Real on-device computing part, the air quality indicator must be determined and pre-alert and alert conditions need to be verified

	// TODO

	/* 
		Simple implementation following, NEEDS TO BE COMPLETED WITH A PROPER ONE
	*/

	if(lastCo2 >= VERY_HIGH_CO2_THRESHOLD)
	{
		alertOrPreAlertConditionsMet == 1;
		setAqiRED();
		//playAlarmChime();
	}
	else if(lastCo2 >= HIGH_CO2_THRESHOLD){
		setAqiRED();
		playPreAlertChime();	// TODO Has to be done differently, this way the computing task has to halt to play the melody!
	}
	else if(lastCo2 >= MEDIUM_CO2_THRESHOLD || lastTemp >= HIGH_TEMPERATURE || lastRh >= HIGH_RH || lastRh <= LOW_RH || lastTemp <= LOW_TEMPERATURE){
		setAqiYELLOW();
	}
	else{
		setAqiGREEN();
	}

	// The following LIGHT SLEEP is only needed to save energy while keeping the aqi LED on
	// TODO needs improvement (we can probably avoid this and perform other things ehile the aqi led stays lit)
	setLEDStatusGREEN();	// The green status LED will stay on while the board is in light sleep, only for debugging purposes
	esp_sleep_enable_timer_wakeup(3 * MICROSECONDS_TO_SECONDS_FACTOR);
	Serial.println("Entering Light Sleep while keeping the aqi LED on");
	esp_light_sleep_start();
	setLEDStatusOFF();

	setAqiOFF();
	// TODO sampling frequency can now be adjusted, depending on the computed air quality level




	if(nCurrStoredMeasures >= (MAX_STORED_MEASUREMENTS) || alertOrPreAlertConditionsMet == 1){
		
		/*
			ACTIVATE LORA AND TRANSMIT
		*/

		nCurrStoredMeasures = 0;
	}


	// The board has completed its job, so it prepares the deep sleep timer and enters deep sleep mode

	/*
		TODO right now it will only deep sleep for a minute EVERY TIME!
		This has to be changed in the final version; the sleep timer needs to take into account the sampling interval 
		and the time spent in the computation and LoRa transmission
	*/
	power_on_after_full_shutdown = 0;
	
	Serial.println("Entering Deep Sleep Mode");


	esp_deep_sleep(60 * MICROSECONDS_TO_SECONDS_FACTOR);

}

void loop() {
	vTaskDelete(NULL);	// Kills the Loop task that we don't use and would otherwise waste resources
}