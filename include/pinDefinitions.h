#ifndef PINDEFINITIONS_H
#define PINDEFINITIONS_H

/*
    @important DO NOT ALTER THIS DEFINITIONS!
    THEY COULD DAMAGE THE BOARD EASILY OR COMPLETELY BREAK THE PROJECT!
    OTHER FUNCTIONS RELY ON THE FOLLOWING DEFINITIONS!!!
*/

// @important GPIO36 CANNOT be used as we already use it to enable Ve, the output 3.3V power

// GPIO pin dedicated to the green discrete led that represents good air quality
#define GREEN_LED	47
// GPIO pin dedicated to the yellow discrete led that represents mediocre air quality
#define YELLOW_LED	48
// GPIO pin dedicated to the red discrete led that represents bad air quality
#define RED_LED		45

// GPIO pin dedicated to the red   led of the multicolor led that indicates the system's status
#define STATUS_RLED 3
// GPIO pin dedicated to the green led of the multicolor led that indicates the system's status
#define STATUS_GLED 4
// GPIO pin dedicated to the blue  led of the multicolor led that indicates the system's status
#define STATUS_BLED 5

#define BUZZERINO	46

// for clarity, we redefine the pins we want to use for I2C
#define SDA_GPIO    41
// for clarity, we redefine the pins we want to use for I2C
#define SCL_GPIO    42

#define VE_ENABLE   36


#endif /* PINDEFINITIONS_H */