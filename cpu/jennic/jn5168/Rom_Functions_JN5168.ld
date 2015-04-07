/*****************************************************************************
 *
 * MODULE:             Rom_Functions_JN5148.ld
 *
 * DESCRIPTION:        Linker command file for JN5148 ROM based functions
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
 * Copyright Jennic Ltd. 2009 All rights reserved
 *
 ****************************************************************************/

EXTERN(_vACI_IntHandlerAes)

EXTERN(_vAHI_WakeTimerStart)
EXTERN(_vAHI_WakeTimerStop)
EXTERN(_vAHI_WakeTimerEnable)
EXTERN(_u32AHI_WakeTimerCalibrate)

EXTERN(_MAC_vHandleMlmeReqRsp)
EXTERN(_MAC_vHandleMcpsReqRsp)

EXTERN(_vMicroIntEnableOnly)
EXTERN(_vMicroIntRestoreState)
