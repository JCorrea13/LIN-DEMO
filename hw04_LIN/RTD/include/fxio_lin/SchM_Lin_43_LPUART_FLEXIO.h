#ifndef SCHM_LIN_43_LPUART_FLEXIO_H
#define SCHM_LIN_43_LPUART_FLEXIO_H

#define SCHM_LIN_43_LPUART_FLEXIO_VENDOR_ID 43
#define SCHM_LIN_43_LPUART_FLEXIO_AR_RELEASE_MAJOR_VERSION 4
#define SCHM_LIN_43_LPUART_FLEXIO_AR_RELEASE_MINOR_VERSION 9

static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_11(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_11(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_13(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_13(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_15(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_15(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_16(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_16(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_17(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_17(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_18(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_18(void) { __asm volatile("cpsie i"); }
static inline void SchM_Enter_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_19(void) { __asm volatile("cpsid i"); }
static inline void SchM_Exit_Lin_43_LPUART_FLEXIO_LIN_EXCLUSIVE_AREA_19(void) { __asm volatile("cpsie i"); }

#endif
