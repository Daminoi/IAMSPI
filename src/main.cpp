#include <Globals.h>

struct sensorMeasure {
    uint16_t co2;
    float temp;
    float rh;
};

// --- Persistent Memory (RTC RAM) ---
RTC_DATA_ATTR uint8_t is_first_boot = 1;
RTC_DATA_ATTR uint8_t pre_alert = 0;
RTC_DATA_ATTR struct sensorMeasure measurements[12];
RTC_DATA_ATTR uint8_t nCurrStoredMeasures = 0;


#ifdef SCD41_NO_ERROR
	#undef SCD41_NO_ERROR
#endif
#define SCD41_NO_ERROR 0


// Global Sensor Object
SensirionI2cScd4x scd41;

// --- Function Declarations ---
void runSystemSequence();
void handleSensing(sensorMeasure &data);
void handleLogic(sensorMeasure data);
void goToDeepSleep();
void getSCD41Reading (sensorMeasure &data);
void wakeSCDAfterDeepSleep();

void setup() {
    Serial.begin(115200);
	setLEDStatusRED();
	while(!Serial){			// Wait for the serial connection to complete, if the status LED remains RED we can immediately tell something is wrong with the serial
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	setLEDStatusOFF();
    
    // 1. Initialize Hardware
    pinMode(VE_ENABLE, OUTPUT);
    digitalWrite(VE_ENABLE, LOW); // Power sensor
    Wire.begin(SDA_GPIO, SCL_GPIO);
    scd41.begin(Wire, SCD41_I2C_ADDR_62);

    if (is_first_boot) {
        Serial.println("Initial Boot: Performing Self-Check...");
        // Add startup_welcome() here
        is_first_boot = 0;
        wakeSCDAfterDeepSleep ();
    }
    else scd41.wakeUp();

    getSCD41Reading(measurements[0]); // Get initial reading to ensure sensor is working
    setLEDStatusGREEN(); 	// The green status LED will stay on while the board is in light sleep, only for debugging purposes

    // 2. Start the main logic sequence as a Task
    xTaskCreate(
        [](void * o) { runSystemSequence(); },
        "MainSeq",
        4096,
        NULL,
        1,
        NULL
    );
}

void loop() {
    vTaskDelete(NULL); 
}

// --- Main Logic Flow ---
void runSystemSequence() {
    sensorMeasure currentData;

    // Phase 1: Sensing
    handleSensing(currentData);

    // Phase 2: Logic & UI
    handleLogic(currentData);

    // Phase 3: Data Storage and Transmission (If buffer full or Alert)
    if (nCurrStoredMeasures < 12)
        measurements[nCurrStoredMeasures++] = currentData;
    else if (nCurrStoredMeasures >= 12) {
        Serial.println("Triggering LoRa Uplink...");
        // triggerLoRaSend(); 
        nCurrStoredMeasures = 0;
        measurements[nCurrStoredMeasures++] = currentData;
    } else if (pre_alert == 1) {
        Serial.println("Triggering LoRa Uplink...");
        // triggerLoRaSend(); 
        pre_alert = 0;
    }

    // Phase 5: Shutdown
    goToDeepSleep();
}

// sense photoresistor, if light level is sufficient put esp32 in light sleep and proceed to read SCD41
void handleSensing(sensorMeasure &data) {
    getSCD41Reading(data);
}

// Turns on scd42, requests data. Board is in light sleep while waiting for data.
void getSCD41Reading (sensorMeasure &data) {
    scd41.measureSingleShot();

    // In the datasheet it is reported that the time necessary to take a sample is 5000 ms (5 seconds) maximum.
    // Therefore we can save power by going in LIGHT SLEEP mode while waiting for the SCD41 to take a sample.
    Serial.println("Sensor measuring (Light Sleep)...");
    esp_sleep_enable_timer_wakeup(5 * U_S_TO_S_FACTOR);
    esp_light_sleep_start();

    if (scd41.readMeasurement(data.co2, data.temp, data.rh) != SCD41_NO_ERROR) {
        Serial.println("Sensor Error!\nTrying again ...");
        esp_sleep_enable_timer_wakeup(5 * U_S_TO_S_FACTOR);
        esp_light_sleep_start();
        if (scd41.readMeasurement(data.co2, data.temp, data.rh) != SCD41_NO_ERROR) {
            Serial.println("Sensor Error!");
            setLEDStatusRED();
        }
    }
    Serial.printf ("CO2: %d ppm, Temp: %.2f °C, RH: %.2f %%\n", data.co2, data.temp, data.rh);
    scd41.powerDown();
}

void handleLogic(sensorMeasure data) {
    measurements[nCurrStoredMeasures].co2 	= data.co2;
	measurements[nCurrStoredMeasures].temp 	= data.temp;
	measurements[nCurrStoredMeasures].rh 	= data.rh;
	nCurrStoredMeasures++;
    if (data.co2 - CO2_VERY_HIGH >= CO2_ERROR) {
        pre_alert = 1;
        setAqiRED();
        // Use a non-blocking chime or a separate task for buzzer
    } else if (data.co2 - CO2_MEDIUM >= CO2_ERROR || data.temp - TEMP_HIGH >= TEMP_ERROR || data.rh - RH_HIGH >= RH_ERROR) {
        setAqiYELLOW();
    } else {
        setAqiGREEN();
    }
    
    // Hold LED for user to see
    vTaskDelay(pdMS_TO_TICKS(2000));
    setAqiOFF();
}

void wakeSCDAfterDeepSleep() {
	SensirionI2cScd4x scd41Sensor;
	int16_t scd41Error;
	char errorMessage[64];
	
	scd41Sensor.begin(Wire, SCD41_I2C_ADDR_62);
	
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

// puts esp32 to deep sleep
void goToDeepSleep() {
    Serial.println("Entering Deep Sleep...");
    esp_deep_sleep(DEEP_SLEEP_SECONDS * U_S_TO_S_FACTOR);
}