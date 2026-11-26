/**
 * @file GPIO.h
 * @brief Public interface to use the GPIO on the S32K312MINI-EVB.
 */

#if !defined(GIPIO_)
#define GIPIO_

typedef void (*InputInterruptCallback)(void);

// Red Led Functions
/**
 * Initializes the red led.
 */
void initRedLed(void);

/**
 * Turn on the red led.
 */
void turnOnRedLed(void);

/**
 * Turn off the red led.
 */
void turnOffRedLed(void);

/**
 * Toggle the red led.
 */
void toggleRedLed(void);

// Green Led Functions
/**
 * Initializes the Green led.
 */
void initGreenLed(void);

/**
 * Turn on the Green led.
 */
void turnOnGreenLed(void);

/**
 * Turn off the Green led.
 */
void turnOffGreenLed(void);

/**
 * Toggle the Green led.
 */
void toggleGreenLed(void);

// Blue Led Functions
/**
 * Initializes the Blue led.
 */
void initBlueLed(void);

/**
 * Turn on the Blue led.
 */
void turnOnBlueLed(void);

/**
 * Turn off the Blue led.
 */
void turnOffBlueLed(void);

/**
 * Toggle the Blue led.
 */
void toggleBlueLed(void);

#endif
