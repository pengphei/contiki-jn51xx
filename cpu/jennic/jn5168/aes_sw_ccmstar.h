/*****************************************************************************
 *
 * MODULE:             AES_SW
 *
 * COMPONENT:          AESSW_CCM.h
 *
 * AUTHOR:             Mark Shea
 *
 * DESCRIPTION:        Software Implementation of CCM Star.
 *
 * $HeadURL: http://svn/sware/Projects/AES_SW/Trunk/Include/aessw_ccm.h $
 *
 * $Revision: 14694 $
 *
 * $LastChangedBy: rcc $
 *
 * $LastChangedDate: 2009-06-19 13:43:25 +0100 (Fri, 19 Jun 2009) $
 *
 * $Id: aessw_ccm.h 14694 2009-06-19 12:43:25Z rcc $
 *
 *****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd. 2008 All rights reserved
 *
 ****************************************************************************/

#ifndef  AESSW_CCMSTAR__H_INCLUDED
#define  AESSW_CCMSTAR__H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "aessw_ccm.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/* CCM* Encode/Decode API Calls */
/* full CCM operation with authentication and Encryption/Decryption */
PUBLIC bool_t bAES_CCMstar
(
  AESSW_Block_u *puKeyData,
  bool_t bLoadKey,
  uint8 u8AESmode,
  uint8 u8M,
  uint8 u8alength,
  uint8 u8mlength,
  AESSW_Block_u *puNonce,
  uint8 *pau8authenticationData,
  uint8 *pau8inputData,
  uint8 *pau8outputData,
  uint8 *pau8checksumData,
  bool_t *pbChecksumVerify
);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* AESSW_CCMSTAR__H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
