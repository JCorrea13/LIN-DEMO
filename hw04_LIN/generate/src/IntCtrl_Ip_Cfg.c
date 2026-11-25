#include "IntCtrl_Ip_Cfg.h"
#include "Lpuart_Uart_Ip_Irq.h"

extern void FLEXIO_Handler(void);

#define PLATFORM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Platform_MemMap.h"

static const IntCtrl_Ip_IrqConfigType uartIrqConfiguration[] = {
    {LPUART5_IRQn, (boolean)TRUE, 0U, LPUART5_IRQHandler},
    {FLEXIO_IRQn, (boolean)TRUE, 0U, FLEXIO_Handler}
};

const IntCtrl_Ip_CtrlConfigType IntCtrlConfig_0 = {
    2U,
    uartIrqConfiguration
};

#define PLATFORM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Platform_MemMap.h"
