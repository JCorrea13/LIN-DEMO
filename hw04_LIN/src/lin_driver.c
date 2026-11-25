#include "lin_driver.h"

#include "Clock_Ip.h"
#include "IntCtrl_Ip.h"
#include "Lpuart_Uart_Ip.h"
#include "Lpuart_Uart_Ip_HwAccess.h"
#include "OsIf.h"
#include "Siul2_Port_Ip.h"
#include "Siul2_Port_Ip_Cfg.h"
#include "S32K312_LPUART.h"
#include "S32K312_FLEXIO.h"
#include "IntCtrl_Ip.h"
#include "fxio_lin/Flexio_Mcl_Ip_HwAccess.h"

#define LIN_UART_INSTANCE   5U
#define LIN_SYNC_BYTE       0x55U
#define LIN_TX_TIMEOUT_ITERATIONS 1000000U
#define FLEXIO_TX_TIMER_ENABLE() \
    IP_FLEXIO->TIMCTL[0U] = \
        (IP_FLEXIO->TIMCTL[0U] & ~(FLEXIO_TIMCTL_TIMOD_MASK)) | \
        FLEXIO_TIMCTL_TIMOD(1U)
#define FLEXIO_TX_TIMER_DISABLE() \
    IP_FLEXIO->TIMCTL[0U] = \
        (IP_FLEXIO->TIMCTL[0U] & ~(FLEXIO_TIMCTL_TIMOD_MASK)) | \
        FLEXIO_TIMCTL_TIMOD(0U)
#define FLEXIO_TX_8BIT_SETUP() \
    do { \
        IP_FLEXIO->TIMCMP[0U] = \
            (IP_FLEXIO->TIMCMP[0U] & ~(0x0000FF00UL)) | \
            FLEXIO_LIN_IP_TIMCMP_BITS_VALUE_U32(8U); \
    } while (0)

uint8 LIN_RxByte;

static Lin_MasterCallbacksType linMasterCallbacks;
static Lin_SlaveCallbacksType linSlaveCallbacks;
static uint8 linResponse[LIN_MAX_DATA_BYTES + 1U];
static uint8 linExpectedLength;
static uint8 linCurrentId;
static volatile uint8 linRxIndex;
static volatile boolean linMasterReceiveActive;
static uint8 linFxioResponse[LIN_MAX_DATA_BYTES + 1U];
typedef enum
{
    LIN_SLAVE_RX_SYNC = 0U,
    LIN_SLAVE_RX_PID
} Lin_SlaveRxStateType;
static Lin_SlaveRxStateType linSlaveRxState;
static volatile boolean linMasterSw3Pressed;
static volatile uint32 linFxioIrqCount;
static volatile uint32 linFxioLastShiftStatus;
static volatile uint32 linFxioLastShiftError;
static volatile uint32 linFxioLastEvent;
static volatile uint32 linFxioLastPid;
static volatile uint32 linLpuartErrorCount;
static volatile uint32 linLpuartLastStatus;
static volatile Lin_StatusType linLpuartLastTxStatus;
typedef enum
{
    LIN_MASTER_TX_IDLE = 0U,
    LIN_MASTER_TX_BREAK,
    LIN_MASTER_TX_SYNC,
    LIN_MASTER_TX_PID,
    LIN_MASTER_TX_DONE
} Lin_MasterTxStateType;
static Lin_MasterTxStateType linMasterTxState;
static uint8 linMasterTxPid;
void FLEXIO_Handler(void);

void Lin_Sw3IrqHandler(void)
{
    if ((IP_SIUL2->DISR0 & SIUL2_DISR0_EIF13_MASK) != 0U) {
        IP_SIUL2->DISR0 = SIUL2_DISR0_EIF13_MASK;
        linMasterSw3Pressed = TRUE;
    }
}

void Lin_MasterSw3Init(void)
{
    Siul2_Port_Ip_SetInputBuffer(PORTB_H_HALF,
                                 10U,
                                 TRUE,
                                 0U,
                                 PORT_INPUT_MUX_NO_INIT);

    IP_SIUL2->IREER0 &= ~SIUL2_IREER0_IREE13_MASK;
    IP_SIUL2->IFEER0 |= SIUL2_IFEER0_IFEE13_MASK;
    IP_SIUL2->IMCR[541U - 512U] = SIUL2_MSCR_SSS(2U);
    IP_SIUL2->DIRSR0 &= ~SIUL2_DIRSR0_DIRSR13_MASK;
    IP_SIUL2->DISR0 = SIUL2_DISR0_EIF13_MASK;
    IP_SIUL2->DIRER0 |= SIUL2_DIRER0_EIRE13_MASK;

    IntCtrl_Ip_InstallHandler(SIUL_1_IRQn,
                              Lin_Sw3IrqHandler,
                              NULL_PTR);
    IntCtrl_Ip_EnableIrq(SIUL_1_IRQn);
}

boolean Lin_MasterSw3ConsumePress(void)
{
    if (linMasterSw3Pressed != TRUE) {
        return FALSE;
    }

    linMasterSw3Pressed = FALSE;
    return TRUE;
}

static boolean Lin_WaitForTxFlag(uint32 flag)
{
    uint32 timeout = LIN_TX_TIMEOUT_ITERATIONS;

    while (timeout > 0U) {
        if ((IP_LPUART_5->STAT & flag) != 0U) {
            return TRUE;
        }
        timeout--;
    }

    return FALSE;
}

static Lin_StatusType Lin_SendBytes(const uint8 *data, uint8 length)
{
    LPUART_Type *uart = IP_LPUART_5;
    uint8 i;

    if ((data == NULL_PTR) || (length == 0U)) {
        return LIN_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0U; i < length; i++) {
        if (Lin_WaitForTxFlag(LPUART_STAT_TDRE_MASK) != TRUE) {
            return LIN_STATUS_UART_ERROR;
        }
        *(volatile uint8 *)&uart->DATA = data[i];
    }

    if (Lin_WaitForTxFlag(LPUART_STAT_TC_MASK) != TRUE) {
        return LIN_STATUS_UART_ERROR;
    }

    return LIN_STATUS_OK;
}

static boolean Lin_MasterWaitTimerComplete(void)
{
    volatile uint32 timeout = 1000000U;
    while (timeout > 0U) {
        if ((IP_FLEXIO->TIMSTAT & (1UL << 0U)) != 0U) {
            IP_FLEXIO->TIMSTAT = (1UL << 0U);
            return TRUE;
        }
        timeout--;
    }
    return FALSE;
}

static Lin_StatusType Lin_SendSynchBreak(void)
{
    LPUART_Type *uart = IP_LPUART_5;
    uint32 ctrl;

    ctrl = uart->CTRL;
    uart->CTRL = ctrl | LPUART_CTRL_SBK_MASK;
    uart->CTRL = ctrl & ~LPUART_CTRL_SBK_MASK;

    return LIN_STATUS_OK;
}
static uint8 Lin_getLengthFromId(uint8 id)
{
    uint8 length;
    switch (id) {
        case 0U: length = 2U; break;
        case 1U: length = 2U; break;
        case 2U: length = 4U; break;
        case 3U: length = 8U; break;
        default:
            length = 0U;
            break;
    }
    return length;
}

uint8 Lin_AddLengthAndParityToHeader(uint8 id, uint8 responseLength)
{
    switch(responseLength) {
        case 2U: id |= 0x00U; break;
        case 4U: id |= 0x20U; break;
        case 8U: id |= 0x30U; break;
        default: id |= 0x10U; break; // this is 2U also
    }

    uint8 identifier = id & 0x3FU;
    uint8 p0 = (uint8)(((identifier >> 0U) ^ (identifier >> 1U) ^
                        (identifier >> 2U) ^ (identifier >> 4U)) & 1U);
    uint8 p1 = (uint8)(~(((identifier >> 1U) ^ (identifier >> 3U) ^
                          (identifier >> 4U) ^ (identifier >> 5U))) & 1U);
    return (uint8)(identifier | (p0 << 6U) | ((p1 & 1U) << 7U));
}

uint8 Lin_Checksum(const uint8 *data, uint8 length)
{
    uint8 sum = 0U;
    uint8 i;

    for (i = 0U; i < length; i++) {
        sum = (uint8)(sum + data[i]);
    }
    return (uint8)~sum;
}

static void Lin_Notify(Lin_StatusType status)
{
    if (linMasterCallbacks.transaction != NULL_PTR) {
        linMasterCallbacks.transaction(status, linCurrentId, linResponse,
                                       linExpectedLength,
                                       linMasterCallbacks.userData);
    }
}

static void Lin_InitCommon(void)
{
    OsIf_Init(NULL_PTR);
    (void)Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
    (void)IntCtrl_Ip_Init(&IntCtrlConfig_0);
}

static void Lin_HandleReceivedByte(uint8 rxByte)
{
    if (linSlaveRxState == LIN_SLAVE_RX_SYNC) {
        if (rxByte == LIN_SYNC_BYTE) {
            linSlaveRxState = LIN_SLAVE_RX_PID;
        }
        return;
    }

    if (linSlaveRxState == LIN_SLAVE_RX_PID) {
        uint8 pid = rxByte;
        uint8 id = pid & 0x3FU;
        uint8 parity = pid >> 6U;
        uint8 expectedP0 =
            (uint8)(((id >> 0U) ^ (id >> 1U) ^
                     (id >> 2U) ^ (id >> 4U)) & 1U);
        uint8 expectedP1 =
            (uint8)(~((id >> 1U) ^ (id >> 3U) ^
                      (id >> 4U) ^ (id >> 5U)) & 1U);

        if ((((parity & 1U) == expectedP0) &&
             (((parity >> 1U) & 1U) == expectedP1)) &&
            (linSlaveCallbacks.slaveRequest != NULL_PTR)) {
            uint8 length =
                Lin_getLengthFromId((pid >> 4U) & 0x03U);

            linSlaveCallbacks.slaveRequest(id & 0x0F, linFxioResponse,
                                           &length,
                                           linSlaveCallbacks.userData);
            if (length > 0U) {
                linFxioResponse[length] =
                    Lin_Checksum(linFxioResponse, length);
                linLpuartLastTxStatus =
                    Lin_SendBytes(linFxioResponse, length + 1U);
            }
        }

        linSlaveRxState = LIN_SLAVE_RX_SYNC;
    }
}

static void Lin_MasterStopResponseReception(Lin_StatusType status);

static void Lin_MasterHandleReceivedByte(uint8 rxByte)
{
    if (linRxIndex < linExpectedLength) {
        linResponse[linRxIndex] = rxByte;
        linRxIndex++;
        return;
    }

    Lin_MasterStopResponseReception(
        (rxByte == Lin_Checksum(linResponse, linExpectedLength)) ?
        LIN_STATUS_OK : LIN_STATUS_CHECKSUM_ERROR);
}

static void Lin_MasterStopResponseReception(Lin_StatusType status)
{
    IP_FLEXIO->SHIFTSIEN &= ~(1UL << 1U);
    IP_FLEXIO->SHIFTEIEN &= ~(1UL << 1U);
    IP_FLEXIO->TIMCTL[1U] &= ~FLEXIO_TIMCTL_TIMOD_MASK;
    IP_FLEXIO->SHIFTSTAT = (1UL << 1U);
    IP_FLEXIO->SHIFTERR = (1UL << 1U);
    linMasterReceiveActive = FALSE;
    //IntCtrl_Ip_DisableIrq(FLEXIO_IRQn);
    Lin_Notify(status);
}

static void Lin_Lpuart5IrqHandler(void)
{
    LPUART_Type *uart = IP_LPUART_5;
    uint32 status = uart->STAT;
    uint32 errorFlags = status & (LPUART_STAT_FE_MASK |
                                  LPUART_STAT_NF_MASK |
                                  LPUART_STAT_OR_MASK |
                                  LPUART_STAT_PF_MASK);
    boolean dataReady = ((status & LPUART_STAT_RDRF_MASK) != 0U);
    uint8 rxByte = 0U;

    linLpuartLastStatus = status;

    if (dataReady) {
        rxByte = (uint8)uart->DATA;
        LIN_RxByte = rxByte;
    }

    if (errorFlags != 0U) {
        uart->STAT = errorFlags;
        linSlaveRxState = LIN_SLAVE_RX_SYNC;
        linLpuartErrorCount++;
        return;
    }

    if (dataReady) {
        Lin_HandleReceivedByte(rxByte);
    }
}

void Lin_MasterInit(uint32 baudrate,
                    const Lin_MasterCallbacksType *callbacks)
{
    uint32 temp;

    Lin_InitCommon();
    linMasterReceiveActive = FALSE;

    Siul2_Port_Ip_SetInputBuffer(PORTB_L_HALF,
                                 9U,
                                 TRUE,
                                 180U,
                                 PORT_INPUT_MUX_ALT1);
    Siul2_Port_Ip_SetOutputBuffer(PORTB_L_HALF,
                                  10U,
                                  TRUE,
                                  PORT_MUX_ALT7);

    IntCtrl_Ip_DisableIrq(FLEXIO_IRQn);
    IntCtrl_Ip_InstallHandler(FLEXIO_IRQn,
                              FLEXIO_Handler,
                              NULL_PTR);

    Clock_Ip_EnableModuleClock(FLEXIO0_CLK);
    IP_FLEXIO->CTRL = FLEXIO_CTRL_FLEXEN_MASK;
    IP_FLEXIO->TIMCTL[0U] = 0U;
    IP_FLEXIO->TIMCTL[1U] = 0U;
    IP_FLEXIO->SHIFTCTL[0U] = 0U;
    IP_FLEXIO->SHIFTCTL[1U] = 0U;
    IP_FLEXIO->SHIFTSTAT = 0xFFU;
    IP_FLEXIO->SHIFTERR = 0xFFU;

    IP_FLEXIO->SHIFTCFG[0U] =
        FLEXIO_SHIFTCFG_SSTART(FLEXIO_SHIFTER_START_BIT_0) |
        FLEXIO_SHIFTCFG_SSTOP(FLEXIO_SHIFTER_STOP_BIT_1) |
        FLEXIO_SHIFTCFG_INSRC(FLEXIO_SHIFTER_SOURCE_PIN);

    IP_FLEXIO->SHIFTCTL[0U] =
        FLEXIO_SHIFTCTL_TIMSEL(0U) |
        FLEXIO_SHIFTCTL_PINCFG(FLEXIO_PIN_CONFIG_OUTPUT) |
        FLEXIO_SHIFTCTL_PINSEL(27U) |
        FLEXIO_SHIFTCTL_SMOD(FLEXIO_SHIFTER_MODE_TRANSMIT);

    IP_FLEXIO->SHIFTCFG[1U] =
        FLEXIO_SHIFTCFG_SSTART(FLEXIO_SHIFTER_START_BIT_0) |
        FLEXIO_SHIFTCFG_SSTOP(FLEXIO_SHIFTER_STOP_BIT_1) |
        FLEXIO_SHIFTCFG_INSRC(FLEXIO_SHIFTER_SOURCE_PIN);

    IP_FLEXIO->SHIFTCTL[1U] =
        FLEXIO_SHIFTCTL_TIMSEL(1U) |
        FLEXIO_SHIFTCTL_PINCFG(FLEXIO_PIN_CONFIG_DISABLED) |
        FLEXIO_SHIFTCTL_PINSEL(28U) |
        FLEXIO_SHIFTCTL_SMOD(FLEXIO_SHIFTER_MODE_RECEIVE) |
        FLEXIO_SHIFTCTL_TIMPOL(FLEXIO_TIMER_POLARITY_NEGEDGE);

    temp = FLEXIO_TIMCFG_TSTART(FLEXIO_TIMER_START_BIT_ENABLED) |
           FLEXIO_TIMCFG_TSTOP(FLEXIO_TIMER_STOP_BIT_TIM_DIS) |
           FLEXIO_TIMCFG_TIMENA(FLEXIO_TIMER_ENABLE_TRG_HIGH) |
           FLEXIO_TIMCFG_TIMDIS(FLEXIO_TIMER_DISABLE_TIM_CMP) |
           FLEXIO_TIMCFG_TIMDEC(5U) |
           FLEXIO_TIMCFG_TIMOUT(FLEXIO_TIMER_INITOUT_ONE_RESET);
    IP_FLEXIO->TIMCFG[0U] = temp;

    IP_FLEXIO->TIMCMP[0U] =
        FLEXIO_TIMCMP_CMP(((uint32)15U << 8U) | 23U);

    IP_FLEXIO->TIMCTL[0U] =
        FLEXIO_TIMCTL_TRGSEL(1U) |
        FLEXIO_TIMCTL_TRGPOL(FLEXIO_TRIGGER_POLARITY_LOW) |
        FLEXIO_TIMCTL_TRGSRC(FLEXIO_TRIGGER_SOURCE_INTERNAL) |
        FLEXIO_TIMCTL_PINCFG(FLEXIO_PIN_CONFIG_DISABLED) |
        FLEXIO_TIMCTL_PINSEL(27U) |
        FLEXIO_TIMCTL_PINPOL(FLEXIO_PIN_POLARITY_LOW) |
        FLEXIO_TIMCTL_TIMOD(0U);

    IP_FLEXIO->TIMCMP[1U] =
        FLEXIO_TIMCMP_CMP(((uint32)15U << 8U) | 23U);
    IP_FLEXIO->TIMCFG[1U] =
        FLEXIO_TIMCFG_TSTART(FLEXIO_TIMER_START_BIT_ENABLED) |
        FLEXIO_TIMCFG_TSTOP(FLEXIO_TIMER_STOP_BIT_TIM_DIS) |
        FLEXIO_TIMCFG_TIMENA(FLEXIO_TIMER_ENABLE_PIN_POSEDGE) |
        FLEXIO_TIMCFG_TIMDIS(FLEXIO_TIMER_DISABLE_TIM_CMP) |
        FLEXIO_TIMCFG_TIMRST(FLEXIO_TIMER_RESET_PIN_RISING) |
        FLEXIO_TIMCFG_TIMDEC(5U) |
        FLEXIO_TIMCFG_TIMOUT(FLEXIO_TIMER_INITOUT_ONE_RESET);
    IP_FLEXIO->TIMCTL[1U] =
        FLEXIO_TIMCTL_TRGSEL(0U) |
        FLEXIO_TIMCTL_TRGPOL(FLEXIO_TRIGGER_POLARITY_HIGH) |
        FLEXIO_TIMCTL_TRGSRC(FLEXIO_TRIGGER_SOURCE_EXTERNAL) |
        FLEXIO_TIMCTL_PINCFG(FLEXIO_PIN_CONFIG_DISABLED) |
        FLEXIO_TIMCTL_PINSEL(28U) |
        FLEXIO_TIMCTL_PINPOL(FLEXIO_PIN_POLARITY_LOW) |
        FLEXIO_TIMCTL_TIMOD(0U);

    IP_FLEXIO->SHIFTSIEN = 0U;
    IP_FLEXIO->SHIFTEIEN = 0U;
    IP_FLEXIO->TIMIEN = 0U;
    IP_FLEXIO->SHIFTSTAT = 0xFFU;
    IP_FLEXIO->TIMSTAT = 0xFFU;
    linMasterTxState = LIN_MASTER_TX_IDLE;

    if (callbacks != NULL_PTR) {
        linMasterCallbacks = *callbacks;
    } else {
        linMasterCallbacks.transaction = NULL_PTR;
        linMasterCallbacks.userData = NULL_PTR;
    }
}

Lin_StatusType Lin_MasterSendFrame(uint8 id,
                                   uint8 responseLength)
{
    uint8 pid;

    if (linMasterReceiveActive || linMasterTxState != LIN_MASTER_TX_IDLE) {
        return LIN_STATUS_BUSY;
    }
    if ((id > 0xFU) || (responseLength > LIN_MAX_DATA_BYTES ||
                        (responseLength % 2U) != 0U)) {
        return LIN_STATUS_INVALID_ARGUMENT;
    }
    linCurrentId = id;
    linExpectedLength = responseLength;
    linMasterReceiveActive = FALSE;
    linRxIndex = 0U;
    pid = Lin_AddLengthAndParityToHeader(id, responseLength);
    linFxioLastPid = pid;

    IntCtrl_Ip_DisableIrq(FLEXIO_IRQn);
    IP_FLEXIO->SHIFTSIEN = 0U;
    IP_FLEXIO->SHIFTEIEN = 0U;
    IP_FLEXIO->TIMIEN = 0U;
    IP_FLEXIO->SHIFTSTAT = (1UL << 0U);
    IP_FLEXIO->SHIFTERR = (1UL << 0U);
    IP_FLEXIO->TIMSTAT = (1UL << 0U);
    IP_FLEXIO->TIMCTL[0U] =
        (IP_FLEXIO->TIMCTL[0U] & ~(FLEXIO_TIMCTL_TIMOD_MASK)) |
        FLEXIO_TIMCTL_TIMOD(0U);
    IP_FLEXIO->TIMCMP[0U] =
        FLEXIO_TIMCMP_CMP(((uint32)23U << 8U) | 23U);

    linMasterTxPid = pid;
    linMasterTxState = LIN_MASTER_TX_BREAK;
    IP_FLEXIO->TIMCTL[0U] =
        (IP_FLEXIO->TIMCTL[0U] & ~(FLEXIO_TIMCTL_TIMOD_MASK)) |
        FLEXIO_TIMCTL_TIMOD(1U);
    IP_FLEXIO->TIMIEN = (1UL << 0U);
    IP_FLEXIO->SHIFTBUF[0U] = 0U;
    IntCtrl_Ip_EnableIrq(FLEXIO_IRQn);
    return LIN_STATUS_OK;
}

// Slave functions
void Lin_SlaveInit(const Lin_SlaveCallbacksType *callbacks)
{
    Lin_InitCommon();
    linMasterReceiveActive = FALSE;
    IntCtrl_Ip_DisableIrq(LPUART5_IRQn);

    if (callbacks != NULL_PTR) {
        linSlaveCallbacks = *callbacks;
    } else {
        linSlaveCallbacks.slaveRequest = NULL_PTR;
        linSlaveCallbacks.userData = NULL_PTR;
    }

    linSlaveRxState = LIN_SLAVE_RX_SYNC;
    Lpuart_Uart_Ip_Init(LIN_UART_INSTANCE,
                        &Lpuart_Uart_Ip_xHwConfigPB_5);

    Lin_SlaveListen();
    IntCtrl_Ip_InstallHandler(LPUART5_IRQn,
                              Lin_Lpuart5IrqHandler,
                              NULL_PTR);
    IntCtrl_Ip_ClearPending(LPUART5_IRQn);
    IP_LPUART_5->CTRL |= LPUART_CTRL_RE_MASK |
                         LPUART_CTRL_TE_MASK |
                         LPUART_CTRL_RIE_MASK |
                         LPUART_CTRL_FEIE_MASK |
                         LPUART_CTRL_NEIE_MASK |
                         LPUART_CTRL_ORIE_MASK;
    IntCtrl_Ip_EnableIrq(LPUART5_IRQn);
}

void Lin_SlaveListen(void)
{
    linSlaveRxState = LIN_SLAVE_RX_SYNC;
}

void FLEXIO_Handler(void)
{
    uint32 timerStatus = IP_FLEXIO->TIMSTAT & (1UL << 0U);
    uint32 rxErrorStatus;
    uint32 rxStatus;

    linFxioIrqCount++;
    linFxioLastShiftStatus = timerStatus;
    linFxioLastShiftError = IP_FLEXIO->SHIFTERR;

    if (timerStatus != 0U) {
        IP_FLEXIO->TIMSTAT = (1UL << 0U);

        if (linMasterTxState == LIN_MASTER_TX_BREAK) {
            IP_FLEXIO->TIMCMP[0U] =
                FLEXIO_TIMCMP_CMP(((uint32)15U << 8U) | 23U);
            linMasterTxState = LIN_MASTER_TX_SYNC;
            IP_FLEXIO->SHIFTSTAT = (1UL << 0U);
            IP_FLEXIO->SHIFTBUF[0U] = LIN_SYNC_BYTE;
        } else if (linMasterTxState == LIN_MASTER_TX_SYNC) {
            linMasterTxState = LIN_MASTER_TX_PID;
            IP_FLEXIO->SHIFTSTAT = (1UL << 0U);
            IP_FLEXIO->SHIFTBUF[0U] = linMasterTxPid;
        } else if (linMasterTxState == LIN_MASTER_TX_PID) {
            IP_FLEXIO->TIMIEN = 0U;
            IP_FLEXIO->TIMCTL[0U] &= ~FLEXIO_TIMCTL_TIMOD_MASK;
            linMasterTxState = LIN_MASTER_TX_IDLE;
            linRxIndex = 0U;
            IP_FLEXIO->SHIFTSTAT = (1UL << 1U);
            IP_FLEXIO->SHIFTERR = (1UL << 1U);

            if (linExpectedLength > 0U) {
                linMasterReceiveActive = TRUE;
                IP_FLEXIO->SHIFTSIEN = (1UL << 1U);
                IP_FLEXIO->SHIFTEIEN = (1UL << 1U);
                IP_FLEXIO->TIMCTL[1U] =
                    (IP_FLEXIO->TIMCTL[1U] &
                     ~FLEXIO_TIMCTL_TIMOD_MASK) |
                    FLEXIO_TIMCTL_TIMOD(1U);
                IntCtrl_Ip_EnableIrq(FLEXIO_IRQn);
            } else {
                Lin_MasterStopResponseReception(LIN_STATUS_OK);
                return;
            }
        }
    }

    rxErrorStatus = IP_FLEXIO->SHIFTERR & (1UL << 1U);
    if (rxErrorStatus != 0U) {
        IP_FLEXIO->SHIFTERR = rxErrorStatus;
        linFxioLastShiftError = rxErrorStatus;
        if (linMasterReceiveActive) {
            Lin_MasterStopResponseReception(LIN_STATUS_UART_ERROR);
            return;
        }
    }

    rxStatus = IP_FLEXIO->SHIFTSTAT & (1UL << 1U);
    if ((rxStatus != 0U) && (linMasterReceiveActive == TRUE)) {
        uint8 rxByte = (uint8)(IP_FLEXIO->SHIFTBUFBYS[1U] & 0xFFU);

        IP_FLEXIO->SHIFTSTAT = rxStatus;
        Lin_MasterHandleReceivedByte(rxByte);
    }
}

