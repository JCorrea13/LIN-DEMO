#include "PIT_TIMER.h"
#include "S32K312.h"
#include "IntCtrl_Ip.h"

#define PIT_CLOCK	30000000UL

static TimerInterruptCallback PIT0Titmer1Callback = NULL;
static TimerInterruptCallback PIT0LinTimeoutCallback = NULL;

void initTimer(void)
{
	IP_PIT_0->MCR = 0; // Enable Timer
}

void PIT0_Handler(void)
{
	if ((IP_PIT_0->TIMER[0].TFLG & PIT_TFLG_TIF_MASK) != 0U) {
		IP_PIT_0->TIMER[0].TFLG = PIT_TFLG_TIF_MASK;
		IP_PIT_0->TIMER[0].TCTRL = 0U;

		if (PIT0LinTimeoutCallback != NULL) {
			TimerInterruptCallback callback = PIT0LinTimeoutCallback;
			PIT0LinTimeoutCallback = NULL;
			callback();
		}
	}

	if ((IP_PIT_0->TIMER[1].TFLG & PIT_TFLG_TIF_MASK) != 0U) {
		IP_PIT_0->TIMER[1].TFLG = PIT_TFLG_TIF_MASK;

		if (PIT0Titmer1Callback != NULL) {
			PIT0Titmer1Callback();
		}
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

void startLinTransactionTimeout(uint32_t ms, TimerInterruptCallback callback)
{
	if (ms == 0U) {
		ms = 1U;
	}

	IP_PIT_0->MCR = 0U;
	IP_PIT_0->TIMER[0].TCTRL = 0U;
	IP_PIT_0->TIMER[0].TFLG = PIT_TFLG_TIF_MASK;
	IP_PIT_0->TIMER[0].LDVAL = ((PIT_CLOCK / 1000UL) * ms) - 1UL;
	PIT0LinTimeoutCallback = callback;

	IntCtrl_Ip_InstallHandler(PIT0_IRQn, PIT0_Handler, NULL_PTR);
	IntCtrl_Ip_ClearPending(PIT0_IRQn);
	IntCtrl_Ip_EnableIrq(PIT0_IRQn);
	IP_PIT_0->TIMER[0].TCTRL = PIT_TCTRL_TEN(1U) | PIT_TCTRL_TIE(1U);
}

void stopLinTransactionTimeout(void)
{
	IP_PIT_0->TIMER[0].TCTRL = 0U;
	IP_PIT_0->TIMER[0].TFLG = PIT_TFLG_TIF_MASK;
	PIT0LinTimeoutCallback = NULL;
}
