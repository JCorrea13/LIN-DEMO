#include "Flexio_Lin_Ip.h"

static Flexio_Lin_Ip_StateStructType linFxioSlaveState;

const Flexio_Lin_Ip_UserConfigType Flexio_Lin_Ip_pxHwConfigPB_0 =
{
    .Instance = 0U,
    .TxShifterId = 0U,
    .TxTimerId = 0U,
    .TxPin = 27U,
    .RxShifterId = 1U,
    .RxTimerId = 1U,
    .RxPin = 28U,
    .FlexioHwInstance = 0U,
    .MasterBreakLength = 13U,
    .SlaveSyncBreakLength = 13U,
    .TimerClkSrc = 5U,
    .Baudratedivider = 48U,
    .NodeFunction = FLEXIO_LIN_IP_MASTER,
    .TransferType = FLEXIO_LIN_IP_USING_INTERRUPTS,
    .Callback = NULL_PTR,
    .StateStruct = &linFxioSlaveState,
    .Baudrate = 9600U,
    .WakeupByte = 0x00U,
    .BaseWakeupByteDetectInverted = 0xFFU
};
