#include "LoRa.h"
#include <heltec_unofficial.h>
#include <LoRaWAN_ESP32.h>

// Global pointer for the RadioLib LoRaWAN node stack
LoRaWANNode* node;

void loRaSleepCallback(RadioLibTime_t ms);

void LoRaManager::begin(uint8_t* appEui, uint8_t* devEui, uint8_t* appKey) {
    Serial.begin(115200);
	while(!Serial)			// Wait for the serial connection to complete, if the status LED remains RED we can immediately tell something is wrong with the serial
		vTaskDelay(200 / portTICK_PERIOD_MS);
    Serial.flush();

    // 1. Initialize the Heltec v3 hardware radio
    Serial.println("[LoRa] Initializing Heltec LoRa hardware...");
    heltec_setup();
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Hardware initialization failed! Code: %d\n", state);
        return;
    }
    Serial.println("[LoRa] Hardware Initialised...");

    // 2. Clone keys into a dynamic memory structure for the FreeRTOS thread
    LoRaKeys* savedKeys = new LoRaKeys();
    memcpy(savedKeys->joinEui, appEui, 8);
    memcpy(savedKeys->devEui, devEui, 8);
    memcpy(savedKeys->appKey, appKey, 16);

    Serial.println("[LoRa] Initializing Task...");

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
    while (true) {
        uint8_t payload[2];
        payload[0] = highByte(counter);
        payload[1] = lowByte(counter);

        Serial.printf("[LoRa Task] Uploading frame data #%d...\n", counter);
        
        // RadioLib update: sendReceive handles uplink transmissions, 
        // choosing Port 1, and expects a temporary output string placeholder for downlinks.
        String strDownlinkResponse = "";
        int16_t state = node->sendReceive(payload, sizeof(payload), 1, strDownlinkResponse);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("[LoRa Task] Uplink broadcast complete!");
            
            // Visual feedback update for the onboard OLED display screen
            heltec_display_power(true);
            display.clear();
            display.drawString(0, 0, "LoRa Task Active");
            display.drawString(0, 15, "Uplink Frame: " + String(counter));
            display.display();
        } else {
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