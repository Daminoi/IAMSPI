#include "Data.h"
#include "LoRa.h"
#include <heltec_unofficial.h>
#include <LoRaWAN_ESP32.h>

RTC_DATA_ATTR uint8_t lwSessionStore[712];

LoRaWANNode* node;
sensorMeasure LoRaManager::measurementsCpy[WINDOW_SIZE];
bool LoRaManager::IsReadyForTransmission = false;

void loRaSleepCallback(RadioLibTime_t ms);

void LoRaManager::begin(uint8_t* appEui, uint8_t* devEui, uint8_t* appKey) {
    heltec_setup();
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Hardware initialization failed! Code: %d\n", state);
        return;
    }

    LoRaKeys* savedKeys = new LoRaKeys();
    memcpy(savedKeys->joinEui, appEui, 8);
    memcpy(savedKeys->devEui, devEui, 8);
    memcpy(savedKeys->appKey, appKey, 16);
    
    LoRaManager::IsReadyForTransmission = false;

    // Allocate 8192 stack size to ensure the RadioLib cryptographic operations have enough headroom
    xTaskCreatePinnedToCore(
        loraTaskWorker,     
        "LoRaWorker",       
        8192,               
        (void*)savedKeys,   
        2,                  
        NULL,               
        1                   
    );
}

void LoRaManager::loraTaskWorker(void *pvParameters) {
    LoRaKeys* keys = (LoRaKeys*)pvParameters;

    node = persist.manage(&radio, lwSessionStore);
    node->setSleepFunction(loRaSleepCallback);

    Serial.println("[LoRa] Node State: " + String(node->isActivated()));
    
    if (!node->isActivated()) {
        Serial.println("[LoRa Task] Configuring TTN OTAA credentials...");
        
        node->beginOTAA(
            *((uint64_t*)keys->joinEui), 
            *((uint64_t*)keys->devEui), 
            keys->appKey,
            keys->appKey
        );

        Serial.println("[LoRa Task] Broadcasting Join Request packet...");
        
        // This function blocks internally until the RX windows close or join succeeds!
        int16_t state = node->activateOTAA();
        
        if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.println("[LoRa Task] Successfully Joined TTN!");
            persist.saveSession(node);
        } else {
            Serial.printf("[LoRa Task] OTAA Join Failed! Error Code: %d\n", state);
            delete keys;       
            vTaskDelete(NULL); 
            return;
        }
    }

    delete keys;

    // --- The Main Continuous LoRa Transmission Loop ---
    while (true) {
        // If your main loop hasn't signaled data is ready, sleep the task briefly to yield CPU
        if (!IsReadyForTransmission) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        for (int i = 0; i < WINDOW_SIZE; i++) {
            uint16_t actualCO2   = measurementsCpy[i].co2;         
            float actualTemp     = measurementsCpy[i].temp;        
            float actualRH       = measurementsCpy[i].rh;          
            float predictedCO2   = 912.40f;                      

            int16_t encodedTemp = (int16_t)(actualTemp * 100.0f);
            uint16_t encodedRH  = (uint16_t)(actualRH * 100.0f);
            uint16_t encodedPred= (uint16_t)predictedCO2;

            uint8_t payload[8];
            payload[0] = (actualCO2 >> 8) & 0xFF;
            payload[1] = actualCO2 & 0xFF;
            payload[2] = (encodedPred >> 8) & 0xFF;
            payload[3] = encodedPred & 0xFF;
            payload[4] = (encodedTemp >> 8) & 0xFF;
            payload[5] = encodedTemp & 0xFF;
            payload[6] = (encodedRH >> 8) & 0xFF;
            payload[7] = encodedRH & 0xFF;
            
            Serial.printf("[LoRa Task] Uploading frame data #%d...\n", i);
        
            String strDownlinkResponse = "";
            int16_t state = node->sendReceive(payload, sizeof(payload), 1, strDownlinkResponse);

            if (state == RADIOLIB_ERR_NONE)
                Serial.println("[LoRa Task] Uplink broadcast complete!");
            else
                Serial.printf("[LoRa Task] Transmission skipped/dropped. Error: %d\n", state);
        }

        // Signal back to main loop that transmission sequence finished
        IsReadyForTransmission = false;

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void loRaSleepCallback(RadioLibTime_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}