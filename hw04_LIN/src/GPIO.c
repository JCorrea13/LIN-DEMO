#include "GPIO.h"
#include "S32K312.h"
#include "Siul2_Port_Ip_Cfg.h"
#include "S32K312_SIUL2.h"

// Red Led Functions
void initRedLed(void)
{
	IP_SIUL2->MSCR[29] = SIUL2_MSCR_OBE(1) | SIUL2_MSCR_SSS(0);
}

void turnOnRedLed(void)
{
	IP_SIUL2->GPDO29 = 0U;
}

void turnOffRedLed(void)
{
	IP_SIUL2->GPDO29 = 1U;
}

void toggleRedLed(void)
{
	IP_SIUL2->GPDO29 ^= 1U;
}

// Green Led Functions
void initGreenLed(void)
{
	IP_SIUL2->MSCR[30] = SIUL2_MSCR_OBE(1) | SIUL2_MSCR_SSS(0);
}

void turnOnGreenLed(void)
{
	IP_SIUL2->GPDO30 = 0U;
}

void turnOffGreenLed(void)
{
	IP_SIUL2->GPDO30 = 1U;
}

void toggleGreenLed(void)
{
	IP_SIUL2->GPDO30 ^= 1U;
}

// Blue Led Functions
void initBlueLed(void)
{
	IP_SIUL2->MSCR[31] = SIUL2_MSCR_OBE(1) | SIUL2_MSCR_SSS(0);
}

void turnOnBlueLed(void)
{
	IP_SIUL2->GPDO31 = 0U;
}

void turnOffBlueLed(void)
{
	IP_SIUL2->GPDO31 = 1U;
}

void toggleBlueLed(void)
{
	IP_SIUL2->GPDO31 ^= 1U;
}
