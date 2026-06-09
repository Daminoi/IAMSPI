#ifndef HUMANINTERACTION_H
#define HUMANINTERACTION_H

#include <Arduino.h>
#include <SSD1306Wire.h>
#include "pinDefinitions.h"

void initBuzzerGPIO();
void initLEDsGPIO();
void initButtonGPIO();
void initDisplay(SSD1306Wire* display);

void playStartUpChime();
void playPreAlertChime();

void startupSelfTestLedBuzzer();

void setLEDStatusRED();
void setLEDStatusGREEN();
void setLEDStatusBLUE();
void setLEDStatusOFF();

void unrecoverableErrorStatus();

void setAqiRED();
void setAqiYELLOW();
void setAqiGREEN();
void setAqiOFF();

void activateButtonPower();
void disableButtonPower();
uint8_t checkButtonClickState();
uint8_t checkButtonLongPress(uint16_t millisBtwChecks, uint16_t nChecks);
uint8_t checkButtonAtLeastOnePress(uint16_t millisBtwChecks, uint16_t nChecks);

void powerOnDisplay(SSD1306Wire* display);
void powerOffDisplay(SSD1306Wire* display);
void displayClsAndPrintln(SSD1306Wire* display, const char* str);
void displayCls(SSD1306Wire* display);

#endif /* HUMANINTERACTION_H */