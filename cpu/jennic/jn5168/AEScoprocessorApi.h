/*****************************************************************************
 *
 * MODULE:              Application Hardware API
 *
 * COMPONENT:           $RCSfile: AEScoprocessorApi.h,v $
 *
 * VERSION:             $Name: JN5148_0035_ROM_CP0 $
 *
 * REVISION:            $Revision: 1.11 $
 *
 * DATED:               $Date: 2008/09/04 17:23:18 $
 *
 * STATUS:              $State: Exp $
 *
 * AUTHOR:              Wayne Ellis
 *
 * DESCRIPTION:
 * AES coprocessor API bit masks, defines and function prototypes.
 *
 * LAST MODIFIED BY:    $Author: dclar $
 *                      $Modtime: $
 *
 ****************************************************************************
 *
 *  (c) Copyright JENNIC Ltd 2006
 *
 ****************************************************************************/

#ifndef  ACI_H_INCLUDED
#define  ACI_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "PeripheralRegs.h"
#include "AppHardwareApi.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* CCM parameter limits */
#define M_MAX       (16)
#define ALENGTH_MAX (127)
#define MLENGTH_MAX (127)

/* command register error bit masks */
#define XCV_REG_AES_ACL_CMD_ERROR_MASK  (2)
#define XCV_REG_AES_ACL_CMD_REJECT_MASK (4)
#define XCV_REG_AES_ACL_CMD_MUTEX_MASK  (8)

//int_src = {ccmda_valid, checksum_valid, mutex, rejected, error, com}
#define XCV_REG_AES_INT_CCMDA_VALID_MASK  (16)
#define XCV_REG_AES_INT_CHKSM_VALID_MASK  (8)
#define XCV_REG_AES_INT_MUTEX_MASK        (4)
#define XCV_REG_AES_INT_CMD_REJECTED_MASK (2)
#define XCV_REG_AES_INT_CMD_COM_MASK      (1)
/* all interrupts */
#define XCV_REG_AES_INT_REG_MASK          (31)

/* status bits bitmasks */
#define XCV_REG_AES_STATUS_AUTH_COMPLETE_MASK (8)
#define XCV_REG_AES_STATUS_CHKSM_VALID_MASK   (4)
#define XCV_REG_AES_STATUS_CCMDA_VALID_MASK   (2)
#define XCV_REG_AES_STATUS_CCMDA_RESULT_MASK  (1)

/* AES Encryption Modes */
#define XCV_REG_AES_SET_MODE_ECB          (0)
#define XCV_REG_AES_SET_MODE_CTR          (2)
#define XCV_REG_AES_SET_MODE_CCM          (3)
#define XCV_REG_AES_SET_MODE_CCM_D        (4)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/* interrupt enable and status register structure */
typedef struct {
    bool_t bCcmdaValid;
    bool_t bChecksumValid;
    bool_t bMutex;
    bool_t bCmdRejected;
    bool_t bCmdError;
    bool_t bCmdComplete;
    } tsInterruptStruct;

/* status register structure */
typedef struct {
    bool_t bAuthComplete;
    bool_t bChecksumValid;
    bool_t bCcmdaValid;
    bool_t bCcmdaResult;
    } tsStatusStruct;

/* command register structure */
typedef struct {
    bool_t bMutex;
    bool_t bRejected;
    bool_t bCmdError;
    bool_t bCmdValid;
    } tsCommandStruct;

/* 128-bit data and config data structure */
typedef struct {
    uint32 u32register0;
    uint32 u32register1;
    uint32 u32register2;
    uint32 u32register3;
    } tsReg128;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC bool_t bACI_setMode(uint8 u8AESmode);
#if((!defined CHIP_RELEASE_1) || (!defined CHIP_RELEASE_2))
PUBLIC bool_t bACI_issueReset(void);
#endif
PUBLIC bool_t bACI_WriteKey(tsReg128 *psKeyData);
PUBLIC bool_t bACI_WriteInputData(uint8 *pau8InputData);
PUBLIC bool_t bACI_SetCCstarM(uint8 u8M);
PUBLIC bool_t bACI_SetCCstarAlength(uint8 u8alength);
PUBLIC bool_t bACI_SetCCstarMlength(uint8 u8mlength);
PUBLIC bool_t bACI_SetCCstarNonce(tsReg128 *psNonce);
PUBLIC bool_t bACI_InitilisePreLoadedCCMstar(void);
PUBLIC bool_t bACI_ReadChecksumData(bool_t bWait, uint8 *pau8ChecksumData, uint8 u8M);
PUBLIC bool_t bACI_ReadChecksumValidationResult(bool_t bWait);
PUBLIC bool_t bACI_ReadOutputData(uint8 *pau8OutputData, uint8 u8BytesRead);
PUBLIC bool_t bACI_isReady(void);
PUBLIC bool_t bACI_isBusy(void);
PUBLIC bool_t bACI_commandAccepted(void);
PUBLIC void vACI_ReadCommandRegister(tsCommandStruct *psCommandStruct);
PUBLIC void vACI_CmdWaitBusy(void);
PUBLIC void vACI_ReadInterruptStatus(tsInterruptStruct *psInterruptStruct);
PUBLIC void vACI_ReadInterruptEnable(tsInterruptStruct *psInterruptStruct);
PUBLIC void vACI_SetInterruptEnable(tsInterruptStruct *psInterruptStruct);
PUBLIC void vACI_ClearInterruptEnable(tsInterruptStruct *psInterruptStruct);
PUBLIC void vACI_ClearInterruptStatus(tsInterruptStruct *psInterruptStruct);
PUBLIC void vACI_ReadStatusRegister(tsStatusStruct *psStatusStruct);
PUBLIC void vACI_IntHandlerAes(void);
PUBLIC void vACI_SetInterruptCallback(PR_HWINT_APPCALLBACK prTimer1Callback);

/* Added by CJG -> */
PUBLIC bool_t bIssueACIcommand(uint8 u8Command, tsReg128 *psCommandData);
/* <- */

/* ECB Encoding API Calls */
PUBLIC bool_t bACI_ECBencodeStripe(tsReg128 *psKeyData, bool_t bLoadKey, tsReg128 *psInputData, tsReg128 *psOutputData);
PUBLIC bool_t bACI_ECBencodeBuffer(tsReg128 *psKeyData, bool_t bLoadKey, uint32 u32stripes, uint8 *pau8inputData, uint8 *pau8outputData, uint32 *pu32stripesComplete);

/* CCM* Encode/Decode API Calls */
/* full CCM operation with authentication and Encryption/Decryption */
PUBLIC bool_t bACI_CCMstar(/*@null@*/tsReg128 *psKeyData, bool_t bLoadKey,
                           uint8 u8AESmode, uint8 u8M, uint8 u8alength, uint8 u8mlength, tsReg128 *psNonce,
                           /*@null@*/uint8 *pau8authenticationData,
                           /*@null@*/uint8 *pau8inputData, /*@null@*/uint8 *pau8outputData, /*@null@*/uint8 *pau8checksumData, /*@null@*/bool_t *pbChecksumVerify);
/* CCM* Data Authentication only */
PUBLIC bool_t bACI_CCMstarAuthenticateData(/*@null@*/tsReg128 *psKeyData, bool_t bLoadKey,
                                           uint8 u8AESmode, uint8 u8M, uint8 u8alength, tsReg128 *psNonce,
                                           uint8 *pau8authenticationData, /*@null@*/uint8 *pau8checksumData, /*@null@*/bool_t *pbChecksumVerify);
/* CCM* Data Encode Decode Only */
PUBLIC bool_t bACI_CCMstarEncodeData(/*@null@*/tsReg128 *psKeyData, bool_t bLoadKey, uint8 u8mlength, tsReg128 *psNonce, uint8 *pau8inputData, uint8 *pau8outputData);

#if((!defined CHIP_RELEASE_1) && (!defined CHIP_RELEASE_2))
/* IEEE CTR Data Encode Decode Only */
PUBLIC bool_t bACI_IEEECTRencodeData(/*@null@*/tsReg128 *psKeyData, bool_t bLoadKey, uint8 u8mlength, tsReg128 *psNonce, uint8 *pau8inputData, uint8 *pau8outputData);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ACI_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
