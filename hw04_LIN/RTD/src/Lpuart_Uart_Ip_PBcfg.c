#include "Lpuart_Uart_Ip.h"

static Lpuart_Uart_Ip_StateStructureType lpuart5State;

const Lpuart_Uart_Ip_UserConfigType Lpuart_Uart_Ip_xHwConfigPB_5 =
{
    9765U,
    192U,
    16U,
    LPUART_UART_IP_PARITY_DISABLED,
    LPUART_UART_IP_ONE_STOP_BIT,
    LPUART_UART_IP_8_BITS_PER_CHAR,
    LPUART_UART_IP_USING_INTERRUPTS,
    NULL_PTR,
    NULL_PTR,
    &lpuart5State
};
