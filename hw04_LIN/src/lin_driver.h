#ifndef LIN_DRIVER_H
#define LIN_DRIVER_H

#include "Std_Types.h"

#define LIN_MAX_DATA_BYTES 8U
#define LIN_MASTER_TIMEOUT_MS 100U

typedef enum
{
    LIN_STATUS_OK = 0U,
    LIN_STATUS_INVALID_ARGUMENT,
    LIN_STATUS_BUSY,
    LIN_STATUS_UART_ERROR,
    LIN_STATUS_PARITY_ERROR,
    LIN_STATUS_CHECKSUM_ERROR,
    LIN_STATUS_TIMEOUT
} Lin_StatusType;

typedef void (*Lin_TransactionCallback)(Lin_StatusType status,
                                        uint8 id,
                                        const uint8 *data,
                                        uint8 length,
                                        void *userData);
typedef void (*Lin_SlaveRequestCallback)(uint8 id,
                                         uint8 *data,
                                         uint8 *length,
                                         void *userData);

typedef struct
{
    Lin_TransactionCallback transaction;
    void *userData;
} Lin_MasterCallbacksType;

typedef struct
{
    Lin_SlaveRequestCallback slaveRequest;
    void *userData;
} Lin_SlaveCallbacksType;

void Lin_MasterInit(uint32 baudrate, const Lin_MasterCallbacksType *callbacks);
void Lin_SlaveInit(const Lin_SlaveCallbacksType *callbacks);
void Lin_MasterSw3Init(void);
boolean Lin_MasterSw3ConsumePress(void);
Lin_StatusType Lin_MasterSendFrame(uint8 id,
                                   uint8 responseLength);
void Lin_SlaveListen(void);
uint8 Lin_AddParityToHeader(uint8 id, uint8 length);
uint8 Lin_Checksum(const uint8 *data, uint8 length);

#endif
