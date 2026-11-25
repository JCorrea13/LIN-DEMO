#include "Mcal.h"
#include "lin_driver.h"
#include "Clock_Ip.h"
#include "Siul2_Port_Ip.h"
#include "Siul2_Port_Ip_Cfg.h"
#include "S32K312_LPUART.h"
#include <stdio.h>

volatile int exit_code = 0;
#define MASTER_TO_SLAVE_ID 0x01U
#define SLAVE_ID 0x01U
#define SLAVE_RESPONSE_LENGTH 8U
#define LIN_NODE_MASTER 0U
#define LIN_NODE_SLAVE 1U
#define LIN_NODE_ROLE LIN_NODE_MASTER

static volatile Lin_StatusType slaveTransactionStatus;
static volatile uint8 slaveResponseData[SLAVE_RESPONSE_LENGTH];
static volatile uint8 slaveResponseLength;
static volatile boolean slaveResponseReceived;
static volatile boolean masterResponseLogPending;
static volatile Lin_StatusType masterResponseLogStatus;
static volatile uint8 masterResponseLogId;
static volatile uint32 fxioIrqCount;
static volatile uint32 fxioShifterStatus;
static volatile uint32 fxioShifterError;
static volatile uint32 fxioRxPinState;
static volatile uint32 fxioLastEvent;
static volatile uint32 fxioLastPid;

static void LinUart6Init(void)
{
    LPUART_Type *uart = IP_LPUART_6;

    Siul2_Port_Ip_SetOutputBuffer(PORTA_H_HALF,
                                  0U,
                                  TRUE,
                                  PORT_MUX_ALT5);
    Clock_Ip_EnableModuleClock(LPUART6_CLK);
    MCAL_DATA_SYNC_BARRIER();
    MCAL_INSTRUCTION_SYNC_BARRIER();

    uart->CTRL = 0U;
    uart->BAUD = (15U << 24U) | 195U;
    uart->STAT = 0U;
    uart->CTRL |= LPUART_CTRL_TE_MASK;
}

static void LinUart6Print(const char *msg)
{
    LPUART_Type *uart = IP_LPUART_6;
    uint32 i;

    if (msg == NULL_PTR) {
        return;
    }

    for (i = 0U; msg[i] != '\0'; i++) {
        while ((uart->STAT & LPUART_STAT_TDRE_MASK) == 0U) {
        }
        *(volatile uint8 *)&uart->DATA = (uint8)msg[i];
    }

    while ((uart->STAT & LPUART_STAT_TC_MASK) == 0U) {
    }
}

static void LinUart6PrintResponse(uint8 id,
                                  Lin_StatusType status,
                                  const uint8 *data,
                                  uint8 length)
{
    char line[96U];
    uint8 i;
    int prefixLength;
    uint8 position;
    const char *statusText;

    switch (status) {
        case LIN_STATUS_OK:
            statusText = "OK";
            break;
        case LIN_STATUS_CHECKSUM_ERROR:
            statusText = "CHECKSUM_ERROR";
            break;
        case LIN_STATUS_UART_ERROR:
            statusText = "UART_ERROR";
            break;
        default:
            statusText = "ERROR";
            break;
    }

    prefixLength = snprintf(line, sizeof(line),
                            "[LIN RX] Slave ID=0x%02X len=%u status=%s data=",
                            (unsigned int)id,
                            (unsigned int)length,
                            statusText);
    if ((prefixLength < 0) || ((size_t)prefixLength >= sizeof(line))) {
        return;
    }
    position = (uint8)prefixLength;

    if ((status == LIN_STATUS_OK) ||
        (status == LIN_STATUS_CHECKSUM_ERROR)) {
        for (i = 0U; (i < length) && (i < SLAVE_RESPONSE_LENGTH); i++) {
            line[position++] = ((data[i] >= 0x20U) &&
                                (data[i] <= 0x7EU))
                                   ? (char)data[i]
                                   : '.';
        }
        line[position++] = '\r';
        line[position++] = '\n';
    } else {
        line[position++] = '-';
        line[position++] = '\r';
        line[position++] = '\n';
    }
    line[position] = '\0';
    LinUart6Print(line);
}

static void LinTransactionDone(Lin_StatusType status,
                               uint8 id,
                               const uint8 *data,
                               uint8 length,
                               void *userData)
{
    uint8 i;

    (void)userData;
    slaveTransactionStatus = status;
    slaveResponseLength = length;
    slaveResponseReceived = FALSE;
    masterResponseLogStatus = status;
    masterResponseLogId = id;
    masterResponseLogPending = TRUE;

    if (((status == LIN_STATUS_OK) ||
         (status == LIN_STATUS_CHECKSUM_ERROR)) &&
        (length <= SLAVE_RESPONSE_LENGTH)) {
        for (i = 0U; i < length; i++) {
            slaveResponseData[i] = data[i];
        }
        slaveResponseReceived = 
            ((status == LIN_STATUS_OK) && (id == MASTER_TO_SLAVE_ID));
    }
}

static void LinPrepareSlaveResponse(uint8 id,
                                    uint8 *data,
                                    uint8 *length,
                                    void *userData)
{
    (void)userData;
    if (id == SLAVE_ID) { // TODO: add support for multiple IDs if needed
                            // make that each slave turn on a different LED
    						// for certain amount of time
                            // led color is the response to be send
        
            data[0] = 'b';
            data[1] = 'l';
            data[2] = 'u';
            data[3] = 'e';
            data[4] = 'e';
            data[5] = 'e';
            data[6] = 'e';
            data[7] = 'e';
        
    } else {
        *length = 0U;
    }
}

int main(void)
{
    const Lin_MasterCallbacksType masterCallbacks =
    {
        .transaction = LinTransactionDone,
        .userData = NULL_PTR
    };

    const Lin_SlaveCallbacksType slaveCallbacks =
    {
        .slaveRequest = LinPrepareSlaveResponse,
        .userData = NULL_PTR
    };

#if (LIN_NODE_ROLE == LIN_NODE_MASTER)
    Lin_MasterInit(9600U, &masterCallbacks);
    Lin_MasterSw3Init();
    LinUart6Init();
#elif (LIN_NODE_ROLE == LIN_NODE_SLAVE)
    Lin_SlaveInit(&slaveCallbacks);
#else
#error "LIN_NODE_ROLE must be LIN_NODE_MASTER or LIN_NODE_SLAVE"
#endif

    for (;;) {
#if (LIN_NODE_ROLE == LIN_NODE_MASTER)
        if (masterResponseLogPending == TRUE) {
            uint8 responseData[SLAVE_RESPONSE_LENGTH];
            uint8 responseLength = slaveResponseLength;
            uint8 responseId = masterResponseLogId;
            Lin_StatusType responseStatus = masterResponseLogStatus;
            uint8 i;

            masterResponseLogPending = FALSE;
            if (responseLength > SLAVE_RESPONSE_LENGTH) {
                responseLength = SLAVE_RESPONSE_LENGTH;
            }
            for (i = 0U; i < responseLength; i++) {
                responseData[i] = slaveResponseData[i];
            }
            LinUart6PrintResponse(responseId,
                                  responseStatus,
                                  responseData,
                                  responseLength);
        }

        if (Lin_MasterSw3ConsumePress()) { // SW3 button pressed, send a LIN frame to the slave
            // TODO: Add logic to send to different slave IDs 
            // on each press. Create a counter based on the number of slaves and increment it
            // on each press, wrapping around to 0 when it reaches the number of slaves. 
            // Use this counter to determine the slave ID to send to.
            // MASTER_TO_SLAVE_ID is currently hardcoded to 0x01U, but it should be updated based on the counter.
            Lin_StatusType sendStatus =
                Lin_MasterSendFrame(MASTER_TO_SLAVE_ID, SLAVE_RESPONSE_LENGTH);
            char msg[64U];

            if (sendStatus == LIN_STATUS_OK) {
                (void)snprintf(msg, sizeof(msg),
                               "[LIN TX] Solicitud enviada a slave ID=0x%02X len=8\r\n",
                               (unsigned int)MASTER_TO_SLAVE_ID);
            } 
            LinUart6Print(msg);
        }
#endif

        if (exit_code != 0) {
            break;
        }
    }
    return exit_code;
}
