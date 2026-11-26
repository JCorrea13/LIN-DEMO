#include "PIT_TIMER.h"
#include "S32K312.h"
#include "IntCtrl_Ip.h"

#define PIT_CLOCK	48000000UL

static TimerInterruptCallback PIT0Titmer1Callback = NULL;

void initTimer(void)
{
	IP_PIT_0->MCR = 0; // Enable Timer
}

void PIT0_Handler(void)
{
    IP_PIT_0->TIMER[1].TFLG = PIT_TFLG_TIF_MASK;	//Clear flag

    if (PIT0Titmer1Callback != NULL)
    {
    	PIT0Titmer1Callback();
    }
}

void setInterruptCallbackForTimer(uint32_t ms, TimerInterruptCallback callback)
{
	IP_PIT_0->TIMER[1].LDVAL = (PIT_CLOCK / 1000UL) * ms - 1UL; // Set period

	PIT0Titmer1Callback = callback; // Assign Interrupt Callback

	// Enable the PIT interrupt and start the timer
	IP_PIT_0->TIMER[1].TCTRL = PIT_TCTRL_TEN(1)+ PIT_TCTRL_TIE(1);
	IntCtrl_Ip_InstallHandler(PIT0_IRQn, PIT0_Handler, NULL_PTR);
	IntCtrl_Ip_EnableIrq(PIT0_IRQn);
}
