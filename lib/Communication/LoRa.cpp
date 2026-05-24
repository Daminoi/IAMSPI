#include "Data.h"
#include "LoRa.h"
#include <heltec_unofficial.h>
#include <LoRaWAN_ESP32.h>

// Global pointer for the RadioLib LoRaWAN node stack
LoRaWANNode* node;
bool LoRaManager::IsReadyForTransmission = false;

void loRaSleepCallback(RadioLibTime_t ms);

void LoRaManager::begin(uint8_t* appEui, uint8_t* devEui, uint8_t* appKey) {
    // 1. Initialize the Heltec v3 hardware radio
    heltec_setup();
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Hardware initialization failed! Code: %d\n", state);
        return;
    }

    // 2. Clone keys into a dynamic memory structure for the FreeRTOS thread
    LoRaKeys* savedKeys = new LoRaKeys();
    memcpy(savedKeys->joinEui, appEui, 8);
    memcpy(savedKeys->devEui, devEui, 8);
    memcpy(savedKeys->appKey, appKey, 16);
    
    LoRaManager::IsReadyForTransmission = false;

    // 3. Spawn the independent background FreeRTOS worker task
    xTaskCreatePinnedToCore(
        loraTaskWorker,     // Task loop function
        "LoRaWorker",       // Name tag
        4096,               // Stack size 
        (void*)savedKeys,   // Pass our copied keys structural block
        2,                  // Priority rank
        NULL,               // Task Handle
        1                   // Execution Core (Core 1)
    );
}

void LoRaManager::loraTaskWorker(void *pvParameters) {
    // Extract our configuration keys back out of the parameter pointer
    LoRaKeys* keys = (LoRaKeys*)pvParameters;
    int counter = 0;

    node = persist.manage(&radio);
    node->setSleepFunction(loRaSleepCallback);

    Serial.println("[LoRa] Node State: " + String(node->isActivated()));
    
    // Join Network via the updated RadioLib 2-Step OTAA Handshake
    if (!node->isActivated()) {
        Serial.println("[LoRa Task] Configuring TTN OTAA credentials...");
        
        // Step 1: Assign keys to the LoRaWAN layer stack memory
        node->beginOTAA(
            *((uint64_t*)keys->joinEui), 
            *((uint64_t*)keys->devEui), 
            keys->appKey,
            keys->appKey
        );

        Serial.println("[LoRa Task] Broadcasting Join Request packet...");
        // Step 2: Trigger the actual network join broadcast sequence
        int16_t state = node->activateOTAA();
        delay (5000);
        if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.println("[LoRa Task] Successfully Joined TTN!");
        } else {
            Serial.printf("[LoRa Task] OTAA Join Failed! Error Code: %d\n", state);
            delete keys;       // Free dynamic memory safety net
            vTaskDelete(NULL); // Terminate thread if handshake crashes out
        }
    }

    // Free up dynamic memory allocation as connection parameters are fully registered
    delete keys;

    // --- The Main Continuous LoRa Transmission Loop ---
    if (!IsReadyForTransmission) return;
    while (true) {
        for (int i = 0; i < WINDOW_SIZE; i++) {
            uint16_t actualCO2   = measurements[i].co2;         // e.g., 850 ppm
            float actualTemp     = measurements[i].temp;        // e.g., 23.54 °C
            float actualRH       = measurements[i].rh;          // e.g., 45.21 %
            float predictedCO2   = 912.40f;                     // e.g., 912.40 ppm

            // 2. Compress the floats into 16-bit integers by saving the decimals
            // Multiplying temperature by 100 turns 23.54 into 2354 (fits perfectly in 2 bytes)
            int16_t encodedTemp = (int16_t)(actualTemp * 100.0f); 
            uint16_t encodedRH  = (uint16_t)(actualRH * 100.0f);  // 45.21 -> 4521
            uint16_t encodedPred= (uint16_t)predictedCO2;         // 912.40 -> 912 (CO2 decimals aren't critical)

            // 3. Allocate an 8-byte payload array (2 bytes per metric x 4 metrics)
            uint8_t payload[8];

            // Bytes 0-1: Actual CO2
            payload[0] = (actualCO2 >> 8) & 0xFF;
            payload[1] = actualCO2 & 0xFF;

            // Bytes 2-3: Predicted CO2
            payload[2] = (encodedPred >> 8) & 0xFF;
            payload[3] = encodedPred & 0xFF;

            // Bytes 4-5: Temperature
            payload[4] = (encodedTemp >> 8) & 0xFF;
            payload[5] = encodedTemp & 0xFF;

            // Bytes 6-7: Humidity
            payload[6] = (encodedRH >> 8) & 0xFF;
            payload[7] = encodedRH & 0xFF;
            
            Serial.printf("[LoRa Task] Uploading frame data #%d...\n", counter);
        
            // RadioLib update: sendReceive handles uplink transmissions, 
            // choosing Port 1, and expects a temporary output string placeholder for downlinks.
            String strDownlinkResponse = "";
            int16_t state = node->sendReceive(payload, sizeof(payload), 1, strDownlinkResponse);

            if (state == RADIOLIB_ERR_NONE)
                Serial.println("[LoRa Task] Uplink broadcast complete!");
            else
                Serial.printf("[LoRa Task] Transmission skipped/dropped. Error: %d\n", state);
        }

        counter++;

        // Safely pauses this individual task context for 60 seconds
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void loRaSleepCallback(RadioLibTime_t ms) {
    // Converts the raw millisecond timing window to FreeRTOS ticks safely
    vTaskDelay(pdMS_TO_TICKS(ms));
}