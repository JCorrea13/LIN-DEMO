#include "Mcal.h"
#include "S32K312.h"
#include "S32K312_FLEXIO.h"
#include "Std_Types.h"

void Flexio_Mcl_Ip_ClearShifterStatus(FLEXIO_Type *baseAddr, uint8 shifter)
{
    baseAddr->SHIFTSTAT = (uint32_t)(1UL << shifter);
}

void Flexio_Mcl_Ip_ClearShifterErrorStatus(FLEXIO_Type *baseAddr, uint8 shifter)
{
    baseAddr->SHIFTERR = (uint32_t)(1UL << shifter);
}

boolean Flexio_Mcl_Ip_GetShifterStatus(const FLEXIO_Type *baseAddr, uint8 shifter)
{
    return ((baseAddr->SHIFTSTAT & (uint32_t)(1UL << shifter)) != 0UL) ? TRUE : FALSE;
}

void Flexio_Mcl_Ip_SetShifterErrorInterrupt(FLEXIO_Type *baseAddr,
                                             uint8 interruptMask,
                                             boolean enable)
{
    if (enable == TRUE) {
        baseAddr->SHIFTEIEN |= (uint32_t)interruptMask;
    } else {
        baseAddr->SHIFTEIEN &= (uint32_t)~interruptMask;
    }
}

void Flexio_Mcl_Ip_SetShifterInterrupt(FLEXIO_Type *baseAddr,
                                       uint8 interruptMask,
                                       boolean enable)
{
    if (enable == TRUE) {
        baseAddr->SHIFTSIEN |= (uint32_t)interruptMask;
    } else {
        baseAddr->SHIFTSIEN &= (uint32_t)~interruptMask;
    }
}

void Flexio_Mcl_Ip_SetTimerInterrupt(FLEXIO_Type *baseAddr,
                                     uint8 interruptMask,
                                     boolean enable)
{
    if (enable == TRUE) {
        baseAddr->TIMIEN |= (uint32_t)interruptMask;
    } else {
        baseAddr->TIMIEN &= (uint32_t)~interruptMask;
    }
}
