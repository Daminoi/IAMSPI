#include "humanInteraction.h"

void initBuzzerGPIO()
{
	pinMode(BUZZERINO, 		OUTPUT);
}

void initLEDsGPIO()
{
	pinMode(RED_LED, 		OUTPUT);
	pinMode(YELLOW_LED, 	OUTPUT);
	pinMode(GREEN_LED, 		OUTPUT);
	pinMode(STATUS_RLED, 	OUTPUT);
	pinMode(STATUS_GLED,	OUTPUT);
	pinMode(STATUS_BLED, 	OUTPUT);
}

void initButtonGPIO()
{
	pinMode(BTN_SENSE,		INPUT);
}

void playStartUpChime()
{
	digitalWrite(BUZZERINO, HIGH);
	delay(75);
	digitalWrite(BUZZERINO, LOW);
	
	delay(175);

	digitalWrite(BUZZERINO, HIGH);
	delay(75);
	digitalWrite(BUZZERINO, LOW);
	
	delay(175);

	digitalWrite(BUZZERINO, HIGH);
	delay(75);
	digitalWrite(BUZZERINO, LOW);
	
	delay(175);

	digitalWrite(BUZZERINO, HIGH);
	delay(75);
	digitalWrite(BUZZERINO, LOW);
	
	delay(300);

	digitalWrite(BUZZERINO, HIGH);
	delay(400);
	digitalWrite(BUZZERINO, LOW);
}

void playPreAlertChime()
{
    digitalWrite(BUZZERINO, HIGH);
	delay(15);
	digitalWrite(BUZZERINO, LOW);
	delay(100);

	digitalWrite(BUZZERINO, HIGH);
	delay(25);
	digitalWrite(BUZZERINO, LOW);
	delay(100);

	digitalWrite(BUZZERINO, HIGH);
	delay(50);
	digitalWrite(BUZZERINO, LOW);

    delay(400);

	digitalWrite(BUZZERINO, HIGH);
	delay(15);
	digitalWrite(BUZZERINO, LOW);
	delay(100);

	digitalWrite(BUZZERINO, HIGH);
	delay(25);
	digitalWrite(BUZZERINO, LOW);
	delay(100);

	digitalWrite(BUZZERINO, HIGH);
	delay(50);
	digitalWrite(BUZZERINO, LOW);
}

void startupSelfTestLedBuzzer()
{
	delay(500);

	digitalWrite(STATUS_GLED, HIGH);
	delay(300);
	digitalWrite(STATUS_GLED, LOW);
	digitalWrite(STATUS_RLED, HIGH);
	delay(300);
	digitalWrite(STATUS_RLED, LOW);
	digitalWrite(STATUS_BLED, HIGH);
	delay(300);
	digitalWrite(STATUS_BLED, LOW);
	digitalWrite(STATUS_RLED, HIGH);
	delay(300);
	digitalWrite(STATUS_RLED, LOW);
	digitalWrite(STATUS_GLED, HIGH);
	delay(300);
	digitalWrite(STATUS_GLED, LOW);

	delay(500);

	digitalWrite(GREEN_LED, HIGH);
	delay(250);
	digitalWrite(YELLOW_LED, HIGH);
	delay(250);
	digitalWrite(RED_LED, HIGH);
	
	delay(500);

	digitalWrite(GREEN_LED, LOW);
	delay(250);
	digitalWrite(YELLOW_LED, LOW);
	delay(250);
	digitalWrite(RED_LED, LOW);

	delay(500);

	playStartUpChime();
}

void setStatusLED (uint8_t green, uint8_t blue, uint8_t red){
	digitalWrite(STATUS_GLED, green);
	digitalWrite(STATUS_BLED, blue);
	digitalWrite(STATUS_RLED, red);
}

void setLEDStatusRED()
{
	setStatusLED (LOW, LOW, HIGH);
}

void setLEDStatusGREEN()
{
	setStatusLED (HIGH, LOW, LOW);
}

void setLEDStatusBLUE()
{
	setStatusLED (LOW, HIGH, LOW);
}

void setLEDStatusOFF()
{
	setStatusLED (LOW, LOW, LOW);
}

void setAQI (uint8_t green, uint8_t yellow, uint8_t red){
	digitalWrite(GREEN_LED, green);
	digitalWrite(YELLOW_LED, yellow);
	digitalWrite(RED_LED, red);
}

void setAqiRED()
{
	setAQI(LOW, LOW, HIGH);
}

void setAqiYELLOW()
{
	setAQI(LOW, HIGH, LOW);
}

void setAqiGREEN()
{
	setAQI(HIGH, LOW, LOW);
}

void setAqiOFF()
{
	setAQI(LOW, LOW, LOW);
}

// The irrecoverable error status can be immediately distinguished because the status led goes back red to blue to red forever.
// Debugging feature!
void unrecoverableErrorStatus()
{
	setLEDStatusOFF();

	for(;;){
		digitalWrite(STATUS_RLED, HIGH);
		delay(500);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		delay(500);
		digitalWrite(STATUS_BLED, LOW);

		digitalWrite(STATUS_RLED, HIGH);
		delay(200);
		digitalWrite(STATUS_RLED, LOW);
		delay(100);
		digitalWrite(STATUS_RLED, HIGH);
		delay(200);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		delay(200);
		digitalWrite(STATUS_BLED, LOW);
		delay(100);
		digitalWrite(STATUS_BLED, HIGH);
		delay(200);
		digitalWrite(STATUS_BLED, LOW);
	}
}

void activateButtonSensing()
{
	digitalWrite(BTN_PWR, HIGH);
}

// The button doesn't let current flow when it is not pressed. It is still good to disable it when not in use.
void disableButtonSensing()
{
	digitalWrite(BTN_PWR, LOW);
}

uint8_t checkButtonClickState()
{
	// The button is wired in such a way that analogRead = 0 approximately when it is not clicked,
	// when the button is pressed down, then analogRead = HIGH (4095) approximately and a small current should
	// flow through the 10k resistor. The resistor is needed to make sure that the GPIO used as sense is reliably 0
	// when the button is not pressed.
	if(analogRead(BTN_SENSE) >= 2000)
		return 1;
	else
		return 0;
}

// returns 1 if and only if the button state is "pressed" for nChecks consecutive times, each executed with millisBtwChecks interval.
// returns 0 otherwise.
// This will halt the current thread calling this function until the probing is over.
uint8_t checkButtonLongPress(uint16_t millisBtwChecks, uint16_t nChecks)
{
	for(uint16_t i=0; i < nChecks; ++i)
	{
		if(checkButtonClickState() == 0)
			return 0;
		delay(millisBtwChecks);
	}

	return 1;
}

// returns 1 if and only if the button state is "pressed" at least once in nChecks consecutive probes, each executed with millisBtwChecks interval.
// returns 0 otherwise.
// This will halt the current thread calling this function until the probing is over.
uint8_t checkButtonAtLeastOnePress(uint16_t millisBtwChecks, uint16_t nChecks)
{
	for(uint16_t i=0; i < nChecks; ++i)
	{
		if(checkButtonClickState() == 1)
			return 1;
		delay(millisBtwChecks);
	}

	return 0;
}
