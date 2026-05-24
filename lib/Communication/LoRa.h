#ifndef LORA_H
#define LORA_H

#include <Arduino.h>

class LoRaManager {
public:
    // Initializes the radio and kicks off the background FreeRTOS task
    static void begin(uint8_t* appEui, uint8_t* devEui, uint8_t* appKey);

private:
    // The core FreeRTOS background task function
    static void loraTaskWorker(void *pvParameters);
    
    // Struct to internally hold keys across to the FreeRTOS worker scope
    struct LoRaKeys {
        uint8_t joinEui[8];
        uint8_t devEui[8];
        uint8_t appKey[16];
    };
};

#endif // LORA_H