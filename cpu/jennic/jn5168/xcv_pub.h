/****************************************************************************
 *
 * MODULE:             ZED-XCV
 *
 * COMPONENT:          $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Components/Xcv/Tags/JN516x_SDK_Generic_R_Gate_RC1a/Include/xcv_pub.h $
 *
 * REVISION:           $Revision: 41368 $
 *
 * DATED:              $Date: 2012-04-30 16:09:45 +0100 (Mon, 30 Apr 2012) $
 *
 * STATUS:             $Id: xcv_pub.h 41368 2012-04-30 15:09:45Z nxp29749 $
 *
 * AUTHOR:             rcc
 *
 * DESCRIPTION:
 * ZED Transceiver interface for single threaded direct memory access
 *
 * LAST MODIFIED BY:   $Author: nxp29749 $
 *
 *
 ****************************************************************************
 *
 *  (c) Copyright 2005, Jennic Limited
 *
 ****************************************************************************/

/**
 * @defgroup grp_XCV_STMM Transceiver infrastructure
 * @brief Transceiver infrastructure module.
 *
 * The Transceiver infrastructure module abstracts the interface to the Transceiver. This version
 * of the module is defined as a memory-mapped peripheral.
 *
 * The access routines are not explicitly shown here as they are used solely by the MAC layer and should
 * not need to be used by higher layers.
 */

#ifndef _xcv_pub_h_
#define _xcv_pub_h_

#ifdef __cplusplus
extern "C" {
#endif

/***********************/
/**** INCLUDE FILES ****/
/***********************/

#include "jendefs.h"
#include "xcv_reg.h"

/************************/
/**** MACROS/DEFINES ****/
/************************/

/* Sanity compile time check */
#if defined(USE_ACTION_CBS) && defined(LEAN_N_MEAN)
#error "#define USE_ACTION_CBS cannot be used with #define LEAN_N_MEAN"
#endif

/**
 * @ingroup grp_XCV_STMM
 * @name PDU Size
 */
#define XCV_PDU_SIZE            128  /* 127 actually, but round up */

/**
 * @name Elemental Read/Write
 * @ingroup grp_XCV_STMM
 * @{
 */
#define XCV_READ_L(a)           (*((volatile uint32 *)(a)))
#define XCV_WRITE_L(b, a)       (*((volatile uint32 *)(a)) = (b))
#define XCV_RMW_L(b, m, a)      (XCV_WRITE_L((((m) & (b)) | (~(m) & XCV_READ_L(a))), (a)))

/*@}*/

/**************************/
/**** TYPE DEFINITIONS ****/
/**************************/

#ifdef USE_ACTION_CBS

/**
 * @ingroup grp_XCV_STMM
 * Register Access Callback types
 *
 * Indicates the operation being performed when called.
 */
typedef enum
{
    XCV_ACTION_CB_READ_REG,     /**< Read register called */
    XCV_ACTION_CB_WRITE_REG,    /**< Write register called */
    XCV_ACTION_CB_RMW_REG,      /**< Read-Modify-Write register called */
    NUM_XCV_ACTION_CB           /**< Endstop */
} XCV_ActionCB_e;

/**
 * @ingroup grp_XCV
 * Register Access Callback function
 *
 * Callback function called when Transceiver registers are accessed; used
 * to simulate Transceiver operation.
 * @note
 * Parameter is command/register structure passed in. void * is used to avoid
 * awkward circular referencing. Return is wildcard - depends on context
 */
typedef uint32 (*XCV_ActionCB_t)(void *pvXCV, uint32 u32AccOp, uint32 u32Data);

#endif /* USE_ACTION_CBS */

typedef void (*XCV_BbcIntCB_t)(void *pvClt, XCV_BbcEvt_e);
typedef void (*XCV_PhyIntCB_t)(void *pvClt, XCV_PhyEvt_e);

/****************/
/**** Device ****/
/****************/

/**
 * @ingroup grp_XCV_STMM
 * Transceiver initialisation parameters
 *
 * Parameters passed to the constructor concerned with Transceiver initialisation.
 */
typedef struct tagXCV_InitParam_s
{
    volatile uint32 *pu32Base; /**< Base address of Transceiver registers */
} XCV_InitParam_s;

/**
 * @ingroup grp_XCV_STMM
 * Transceiver object
 *
 * The class of Transceiver object
 */
typedef struct tagXCV_s
{
#ifndef LEAN_N_MEAN
    volatile uint32 *pu32Base;  /**< Card's memory base address - 0 if disabled */
#endif
    uint16          u16IER;     /**< IER shadow */
#ifdef USE_ACTION_CBS
    void            *pvClt;     /**< Associated Client object */
    XCV_ActionCB_t  aprActionCB[NUM_XCV_ACTION_CB]; /**< Action callbacks */
#ifdef SIMULATOR

    void *pvAncillary;

#endif

#endif
} XCV_s;

/****************************/
/**** EXPORTED VARIABLES ****/
/****************************/

/****************************/
/**** EXPORTED FUNCTIONS ****/
/****************************/

/**** Transceiver Instantiation ****/

/* Construct */
PUBLIC void
XCV_vCtor(XCV_s *psXcv,
          XCV_InitParam_s *psInitParam);

/**** XCV Device Access ****/

#ifdef USE_ACTION_CBS
/* Register action callback */
PUBLIC void
XCV_vRegisterActionCB(XCV_s *psXcv,
                      XCV_ActionCB_e eCBType,
                      XCV_ActionCB_t prCB);
#endif

#ifdef LEAN_N_MEAN

#if (defined SINGLE_CONTEXT) && (defined EMBEDDED)
#ifdef OPTIMISE_REG_STRUCT
/* r16 is set to 0x02008000, all register offsets are from a base of 
   XCV_BASE_ADDR, so offset to use from r16 is as follows: */
#define R16_OFFSET (0x02008000 - XCV_BASE_ADDR)

#define XCV_u32DevReadReg(psXcv, eOffset)       \
     ({ uint32 __result, __arg = (((eOffset) << 2) - R16_OFFSET);   \
        asm volatile ("bg.lwz %[result], %[offset](r16);": [result] "=r" (__result): [offset] "i" (__arg));  \
        __result; })

#define XCV_u32DevReadRegNC(psXcv, eOffset)       \
     ({ uint32 __result, __u32Addr = (((eOffset) << 2) - R16_OFFSET);   \
        asm volatile("bt.add %0, r16;" : "+r" (__u32Addr) : ); \
        asm volatile ("bn.lwz %[result], 0(%[addr]);": [result] "=r" (__result): [addr] "r" (__u32Addr));  \
        __result; })

#define XCV_vDevWriteReg32(psXcv, eOffset, u32Data) \
    asm volatile("bg.sw %[offset](r16), %[value];" : : [offset] "i" (((eOffset) << 2) - R16_OFFSET), [value] "r" (u32Data) )

#define XCV_vDevWriteReg32NC(psXcv, eOffset, u32Data) \
    { uint32 __u32Addr = (((eOffset) << 2) - R16_OFFSET); \
      asm volatile("bt.add %0, r16;" : "+r" (__u32Addr) : ); \
      asm volatile("bn.sw 0(%[addr]), %[value];" : : [addr] "r" (__u32Addr), [value] "r" (u32Data) ); }

#else
#define XCV_u32DevReadReg(psXcv, eOffset)           (XCV_READ_L((uint32*)(XCV_BASE_ADDR + ((eOffset)<<2))))
#define XCV_vDevWriteReg32(psXcv, eOffset, u32Data) (XCV_WRITE_L((u32Data), (uint32*)(XCV_BASE_ADDR + ((eOffset)<<2))))
#endif
#else

#define XCV_u32DevReadReg(psXcv, eOffset)           (XCV_READ_L((psXcv)->pu32Base + (eOffset)))
#define XCV_vDevWriteReg32(psXcv, eOffset, u32Data) (XCV_WRITE_L((u32Data), (psXcv)->pu32Base + (eOffset)))

#endif /* (defined SINGLE_CONTEXT) && (defined EMBEDDED) */

#define XCV_vDevRMWReg32(psXcv, eOffset, u32Mask, u32Data) XCV_vDevWriteReg32(psXcv, eOffset, (((u32Mask) & (u32Data)) | (~(u32Mask) & (XCV_u32DevReadReg(psXcv, eOffset)))))
#define XCV_vDevRMWReg32NC(psXcv, eOffset, u32Mask, u32Data) XCV_vDevWriteReg32NC(psXcv, eOffset, (((u32Mask) & (u32Data)) | (~(u32Mask) & (XCV_u32DevReadRegNC(psXcv, eOffset)))))

#else /* LEAN_N_MEAN */

/* Device Read Register */
PUBLIC uint32
XCV_u32DevReadReg(XCV_s *psXcv,
                  XCV_Reg_e eOffset);


/* Device Write Register */
PUBLIC void
XCV_vDevWriteReg32(XCV_s *psXcv,
                   XCV_Reg_e eOffset,
                   uint32 u32Data);

/* Device Read-Modify-Write Register */
PUBLIC void
XCV_vDevRMWReg32(XCV_s *psXcv,
                 XCV_Reg_e eOffset,
                 uint32 u32Mask,
                 uint32 u32Data);

#endif /* !LEAN_N_MEAN */

/* 16 and 8 bit accesses use 32 bit access functions */
#define XCV_u16DevReadReg(psXcv, eOffset)               ((uint16)XCV_u32DevReadReg(psXcv,eOffset))
#define XCV_u8DevReadReg(psXcv, eOffset)                ((uint8)XCV_u32DevReadReg(psXcv,eOffset))
#define XCV_vDevWriteReg16(psXcv, eOffset, u16Data)     XCV_vDevWriteReg32(psXcv,eOffset,(uint32)(u16Data))
#define XCV_vDevWriteReg8(psXcv, eOffset, u8Data)       XCV_vDevWriteReg32(psXcv,eOffset,(uint32)(u8Data))
#define XCV_vDevRMWReg16(psXcv,eOffset,u16Mask,u16Data) XCV_vDevRMWReg32(psXcv,eOffset,(uint32)u16Mask,(uint32)u16Data)
#define XCV_vDevRMWReg8(psXcv,eOffset,u8Mask,u8Data)    XCV_vDevRMWReg32(psXcv,eOffset,(uint32)u8Mask,(uint32)u8Data)

#define XCV_u16DevReadRegNC(psXcv, eOffset)               ((uint16)XCV_u32DevReadRegNC(psXcv,eOffset))
#define XCV_u8DevReadRegNC(psXcv, eOffset)                ((uint8)XCV_u32DevReadRegNC(psXcv,eOffset))
#define XCV_vDevWriteReg16NC(psXcv, eOffset, u16Data)     XCV_vDevWriteReg32NC(psXcv,eOffset,(uint32)(u16Data))
#define XCV_vDevWriteReg8NC(psXcv, eOffset, u8Data)       XCV_vDevWriteReg32NC(psXcv,eOffset,(uint32)(u8Data))
#define XCV_vDevRMWReg16NC(psXcv,eOffset,u16Mask,u16Data) XCV_vDevRMWReg32NC(psXcv,eOffset,(uint32)u16Mask,(uint32)u16Data)
#define XCV_vDevRMWReg8NC(psXcv,eOffset,u8Mask,u8Data)    XCV_vDevRMWReg32NC(psXcv,eOffset,(uint32)u8Mask,(uint32)u8Data)

/* IER write value */
PUBLIC void
XCV_vDevWriteIER(XCV_s *psXcv,
                 uint16 u16Data);

#ifdef ROM_PATCH_ZED003

/* If patching ZED003, refer to function in ROM */
/* IER modify-write value */
typedef void (*tprXCV_vDevRMW_IER)(XCV_s *, uint16, uint16);
#define XCV_vDevRMW_IER ((tprXCV_vDevRMW_IER) 0xf0f0)

#else

/* IER modify-write value */
PUBLIC void
XCV_vDevRMW_IER(XCV_s *psXcv,
                uint16 u16Mask,
                uint16 u16Data);
#endif

#ifdef __cplusplus
};
#endif

#endif /* _xcv_pub_h_ */

/* End of file $RCSfile: xcv_pub.h,v $ *******************************************/
