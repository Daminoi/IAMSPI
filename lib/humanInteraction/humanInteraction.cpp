#include "Globals.h"

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

void startup_welcome()
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

void setLED (int green, int blue, int red){
	digitalWrite(STATUS_GLED, green);
	digitalWrite(STATUS_BLED, blue);
	digitalWrite(STATUS_RLED, red);
}

void setLEDStatusRED()
{
	setLED (LOW, LOW, HIGH);
}

void setLEDStatusGREEN()
{
	setLED (HIGH, LOW, LOW);
}

void setLEDStatusBLUE()
{
	setLED (LOW, HIGH, LOW);
}

void setLEDStatusOFF()
{
	setLED (LOW, LOW, LOW);
}

void setAQI (int green, int yellow, int red){
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
void unrecoverableErrorStatus()
{
	setLEDStatusOFF();

	for(;;){
		digitalWrite(STATUS_RLED, HIGH);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_BLED, LOW);

		digitalWrite(STATUS_RLED, HIGH);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_RLED, LOW);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_RLED, HIGH);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_RLED, LOW);

		digitalWrite(STATUS_BLED, HIGH);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_BLED, LOW);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		digitalWrite(STATUS_BLED, HIGH);
		vTaskDelay(200);
		digitalWrite(STATUS_BLED, LOW);
	}
}