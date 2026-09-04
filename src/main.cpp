#include <SSD1306Wire.h>

#include "Globals.h"
#include "humanInteraction.h"
#include "LightsOn.h"
#include "Prediction.h"
#include "LoRa.h"
#include "Data.h"

#include "embeddedCredentials.h"

#define SCD41_NO_ERROR 0

// Persistent Memory (persists after Deep Sleep but not after a full power down-power up cicle)
RTC_DATA_ATTR uint8_t is_first_boot = 1;
RTC_DATA_ATTR uint8_t pre_alert = 0;
RTC_DATA_ATTR uint8_t light_On = 0;
RTC_DATA_ATTR uint8_t isFirstReading = 1;

RTC_DATA_ATTR struct sensorMeasure measurements[WINDOW_SIZE];
RTC_DATA_ATTR uint8_t nCurrStoredMeasures;
RTC_DATA_ATTR uint8_t measurementIndex;

SSD1306Wire* OLEDdisplay;
SensirionI2cScd4x scd41;    // Global Sensor Object
int16_t scd41Error;
char errorMessage[64];
int lastPrinted = 0;
const float cO2Alpha = 0.15;

SemaphoreHandle_t dataMutex = NULL;
TaskHandle_t loRaTaskHandle = NULL;


void runSystemSequence();
void checkLevels(sensorMeasure data);
void goToDeepSleep();
int getSCD41Reading (sensorMeasure &data);
void wakeSCDAfterDeepSleep();

void setup() {

    // 1. Initialize Hardware
    pinMode(VE_ENABLE, OUTPUT);
    digitalWrite(VE_ENABLE, LOW);
    
    Wire1.begin(SDA_GPIO, SCL_GPIO);
    
    OLEDdisplay = new SSD1306Wire(0x3C, OLED_SDA, OLED_SCL, GEOMETRY_128_64, I2C_ONE);
    
    initDisplay(OLEDdisplay);
    initBuzzerGPIO();
    initLEDsGPIO();
    initButtonGPIO();
    
    pinMode(LDR_POWER, OUTPUT);
    digitalWrite(LDR_INPUT, LOW); // low by default
    
    if(is_first_boot == 1) 
        startupSelfTestLedBuzzer();
    
    // Serial initialization
    Serial.begin(115200);
	setLEDStatusRED();
    delay(1000);
    setLEDStatusOFF();
    Serial.flush();

    // LDR calibration after a full power cicle
    if (is_first_boot == 1) {
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Booting up after full power cicle: performing light sensor calibration...");
        #endif
        displayClsAndPrintln(OLEDdisplay, "LDR calibration:\n keep the LDR covered\n while pressing the button");

        activateButtonPower();
        while(checkButtonAtLeastOnePress(250, 100) == 0)
        {
            Serial.println("Keep the LDR covered while clicking the button to perform the calibration ...");
            playStartUpChime(); // so that we don't forget to do this calibration step
        }
        disableButtonPower();

        LDR::setIlluminationBaseline();

        displayClsAndPrintln(OLEDdisplay, "Calibration completed!\nUncover the LDR");
        delay(4000);
        powerOffDisplay(OLEDdisplay);

        is_first_boot = 0;
    }

    wakeSCDAfterDeepSleep();

    setLEDStatusGREEN();

    dataMutex = xSemaphoreCreateMutex();
    
    #ifdef DEBUG_MODE_ACTIVE
    if (dataMutex == NULL) Serial.println("Failed to create Data Mutex.");
    #endif

    // 1. Communication task
    #ifdef DEBUG_USE_EMBEDDED_CREDENTIALS
    LoRaManager::begin(testJoinEui, testDevEui, testAppKey);
    #else

    #endif

    // 2. Start the main logic sequence as a Task
    xTaskCreate(
        [](void * o) { runSystemSequence(); },
        "MainSeq",
        4096,
        NULL,
        1,
        &loRaTaskHandle
    );

    // 3. Prediction Task
    xTaskCreatePinnedToCore(
        [](void * o) { Regression::processingTask(measurementIndex); },
        "ProcTask", 
        4096,
        NULL,
        3,
        NULL,
        1
    );
}

void loop() {
    vTaskDelete(NULL); 
}

// Main logic flow
void runSystemSequence() {
    sensorMeasure currentData;
    
    light_On = LDR::getIllumination();
    if (!light_On) {
        // If we woke up and the light is still off, it means we are in a low-light environment and we should skip the sensor reading and go back to sleep immediately.
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Low light detected on wakeup, going back to sleep...");
        #endif
        goToDeepSleep();
        return;
    }
    
    for (int i = 0; i < 3; i++) {
        // Phase 1: Sensing
        int error = getSCD41Reading(currentData);
        float filteredCO2 = 0.0f;

        if (isFirstReading) {
            isFirstReading = 0;
            filteredCO2 = currentData.co2;
        }
        else {
            // LPF for smoothing CO2 values
            // temp and humidity are quiet smooth already. 
            currentData.co2 = (cO2Alpha * currentData.co2) + ((1.0f - cO2Alpha) * filteredCO2);
        }
        
        if (currentData.co2 && currentData.temp && currentData.rh) {
            // Phase 2: Logic & UI
            checkLevels(currentData);
            // Phase 3: Data Storage and Transmission (If buffer full or Alert)
            if (nCurrStoredMeasures < WINDOW_SIZE) {
                measurements[nCurrStoredMeasures++] = currentData;
                #ifdef DEBUG_MODE_ACTIVE
                Serial.println("Data stored successfully.");
                #endif
            }
            else if (nCurrStoredMeasures >= WINDOW_SIZE) {
                #ifdef DEBUG_MODE_ACTIVE
                Serial.println("Triggering LoRa Uplink...");
                #endif
                for (int i = 0; i < WINDOW_SIZE; i++) {
                    LoRaManager::measurementsCpy[i] = measurements[i];
                }
                LoRaManager::IsReadyForTransmission = true;
                nCurrStoredMeasures = 0;
                measurements[nCurrStoredMeasures++] = currentData;
            } else if (pre_alert == 1) {
                #ifdef DEBUG_MODE_ACTIVE
                Serial.println("Triggering LoRa Uplink...");
                #endif
                #ifndef DEBUG_MODE_ACTIVE
                LoRaManager::IsReadyForTransmission = true;
                #endif  
                pre_alert = 0;
            }
            break;
        }
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Reading failed, retrying...");
        #endif
    }
    
    while (!loRaTaskHandle || LoRaManager::IsReadyForTransmission) vTaskDelay(pdMS_TO_TICKS(500));
    // Phase 5: Shutdown
    #ifdef DEBUG_MODE_ACTIVE
    Serial.println("Sequence complete, going to sleep...");
    #endif
    goToDeepSleep();
}

// Turns on scd42, requests data. Board is in light sleep while waiting for data.
int getSCD41Reading (sensorMeasure &data) {
    scd41.wakeUp(); // TODO: is this required?
    scd41.measureSingleShot();
    
    // In the datasheet it is reported that the time necessary to take a sample is 5000 ms (5 seconds) maximum.
    // Therefore we can save power by going in LIGHT SLEEP mode while waiting for the SCD41 to take a sample.
    #ifdef DEBUG_MODE_ACTIVE
    Serial.println("Sensor measuring (Light Sleep)...");
    #endif
    esp_sleep_enable_timer_wakeup(5 * U_S_TO_S_FACTOR);
    esp_light_sleep_start();
    scd41Error = scd41.readMeasurement(data.co2, data.temp, data.rh);
    if (scd41Error != SCD41_NO_ERROR) {
        errorToString(scd41Error, errorMessage, sizeof(errorMessage));
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Sensor Error!\nTrying again ...");
        Serial.println(errorMessage);
        #endif
        esp_sleep_enable_timer_wakeup(5 * U_S_TO_S_FACTOR);
        esp_light_sleep_start();
        if (scd41.readMeasurement(data.co2, data.temp, data.rh) != SCD41_NO_ERROR) {
            #ifdef DEBUG_MODE_ACTIVE
            Serial.println("Sensor Error!");
            #endif
            setLEDStatusRED();
        }
    }


    #ifdef DEBUG_MODE_ACTIVE
    Serial.printf ("CO2: %d ppm, Temp: %.2f °C, RH: %.2f %%\n", data.co2, data.temp, data.rh);
    Serial.printf (">co2:%d:%u|\n", measurementIndex, data.co2);
    Serial.printf (">temp:%d:%.2f|\n", measurementIndex, data.temp);
    Serial.printf (">humidity:%d:%.2f|\n", measurementIndex,  data.rh);
    Serial.flush();
    #endif

    scd41.powerDown();
    return SCD41_NO_ERROR;
}

void checkLevels(sensorMeasure data) {
    if (data.co2 - CO2_VERY_HIGH >= CO2_ERROR) {
        pre_alert = 1;
        setAqiRED();
    } else if (data.co2 - CO2_MEDIUM >= CO2_ERROR || 
               data.temp - TEMP_HIGH >= TEMP_ERROR ||
               data.rh - RH_HIGH >= RH_ERROR) setAqiYELLOW();
    else setAqiGREEN();
    
    // Hold LED for user to see
    vTaskDelay(pdMS_TO_TICKS(2000));
    setAqiOFF();
}

void wakeSCDAfterDeepSleep() {	
    scd41.begin(Wire1, SCD41_I2C_ADDR_62);

    uint64_t scd41SerialNumber;

    setLEDStatusRED();
    scd41Error = scd41.wakeUp();
    if (scd41Error != SCD41_NO_ERROR) {
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Error while trying to wake up the SCD41!");
        #endif
        unrecoverableErrorStatus();
    }
    setLEDStatusOFF();

	// Asking the SCD41 to provide its serial number is one way to check if it is in idle state
	setLEDStatusRED();
	scd41Error = scd41.getSerialNumber(scd41SerialNumber);
	if (!scd41SerialNumber){
        #ifdef DEBUG_MODE_ACTIVE
        Serial.println("Failure to obtain the SCD41 serial number!");
		errorToString(scd41Error, errorMessage, sizeof(errorMessage));
		Serial.println(errorMessage);
		#endif
        unrecoverableErrorStatus();
	}
	setLEDStatusOFF();

    #ifdef DEBUG_MODE_ACTIVE
    Serial.print("SCD41's serial number: 0x");
    Serial.print((uint32_t)(scd41SerialNumber >> 32), HEX);
    Serial.println((uint32_t)(scd41SerialNumber & 0xFFFFFFFF), HEX);

    Serial.println("Power on procedure completed with SUCCESS!");
    #endif
    
    setLEDStatusGREEN();
    vTaskDelay(pdMS_TO_TICKS(2000));
    setLEDStatusOFF();
}

// puts esp32 to deep sleep
// deep sleep entirely kills freertos tasks.
void goToDeepSleep() {
    #ifdef DEBUG_MODE_ACTIVE
    Serial.println("Entering Deep Sleep...");
    #endif
    esp_deep_sleep(DEEP_SLEEP_SECONDS * U_S_TO_S_FACTOR);
}